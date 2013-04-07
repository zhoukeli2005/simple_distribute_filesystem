#include "s_core_glock.h"

void s_core_mserv_glock(struct s_server * serv, struct s_packet * pkt, void * ud)
{
	struct s_core * core = (struct s_core *)ud;
	struct s_mserver * mserv = s_core_mserv(core);

	int type = s_servg_get_type(serv);
	int id = s_servg_get_id(serv);

	assert(type == S_SERV_TYPE_C);

	unsigned int req_id = s_packet_get_req(pkt);

	struct s_id lock_id;
	unsigned int lock;

	s_packet_read(pkt, &lock_id.x, int, label_error);
	s_packet_read(pkt, &lock_id.y, int, label_error);
	s_packet_read(pkt, &lock, uint, label_error);

//	s_log("id.x:%d, id.y:%d, lock:%x", lock_id.x, lock_id.y, lock);

	struct s_glock_elem * elem = s_hash_set_id(mserv->glock_elems, lock_id);
	if(!elem) {
		s_log("[Warning glock : s_hash_set_id failed!");
		return;
	}
	elem->client_id = id;
	elem->req_id = req_id;
	elem->lock = lock;
	elem->is_waiting = 1;
	
	if(!(mserv->glock & lock)) {
	//	s_log("[LOG] no-conflict! ok");
		elem->is_waiting = 0;
		mserv->glock |= lock;
		s_servg_rpc_ret(serv, req_id, pkt);
		return;
	}

//	s_log("[LOG] conflict! waiting...");

	return;

label_error:
	s_log("[Warning] glock : miss param!");
}

void s_core_mserv_glock_unlock(struct s_server * serv, struct s_packet * pkt, void * ud)
{
	struct s_core * core = (struct s_core *)ud;
	struct s_mserver * mserv = s_core_mserv(core);

	int type = s_servg_get_type(serv);
	int id = s_servg_get_id(serv);

	assert(type == S_SERV_TYPE_C);

	struct s_id lock_id;
	unsigned int lock;

	s_packet_read(pkt, &lock_id.x, int, label_error);
	s_packet_read(pkt, &lock_id.y, int, label_error);

//	s_log("id.x:%d, id.y:%d, lock:%u", lock_id.x, lock_id.y, lock);

	struct s_glock_elem * elem = s_hash_get_id(mserv->glock_elems, lock_id);
	if(!elem) {
		s_log("[Warning glock release : s_hash_get_id failed!");
		return;
	}

	assert(id == elem->client_id);

//	s_log("[LOG] release : %x", elem->lock);

	mserv->glock &= ~(elem->lock);

//	s_log("curr lock:%x", mserv->glock);

	// del elem
	s_hash_del_id(mserv->glock_elems, lock_id);

	// iter to unlock
	int iter = -1;
	struct s_hash_key key;
	struct s_glock_elem * next = s_hash_next(mserv->glock_elems, &iter, &key);
	elem = next;
	while(next) {
		elem = next;
		next = s_hash_next(mserv->glock_elems, &iter, &key);

		if(!elem->is_waiting) {
			continue;
		}
		if((mserv->glock & elem->lock)) {
			continue;
		}

		elem->is_waiting = 0;

	//	s_log("[LOG] unlock, id(%d, %d), lock:%x", key.id.x, key.id.y, elem->lock);
		struct s_server * serv = s_servg_get_active_serv(core->servg, S_SERV_TYPE_C, elem->client_id);

		if(!serv) {
			s_log("[Warning] unlock, client:%d missing!", elem->client_id);
			continue;
		}

		mserv->glock |= elem->lock;

		int sz = s_packet_size_for_id(0);
		struct s_packet * pkt = s_packet_create(sz);
		s_packet_write_int(pkt, key.id.x);
		s_packet_write_int(pkt, key.id.y);
		s_servg_rpc_ret(serv, elem->req_id, pkt);

		s_packet_drop(pkt);
	}
	return;

label_error:
	s_log("[Warning] glock release : miss param!");
}


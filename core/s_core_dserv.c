#include "s_core_dserv.h"
#include "s_core_lock.h"

static int ilock_is_conflict(struct s_core_lock * a, struct s_core_lock * b)
{
	int cur_a = a->next_pos;
	int cur_b = b->next_pos;

	struct s_core_lock_elem * pa = s_core_lock_p(a);
	struct s_core_lock_elem * pb = s_core_lock_p(b);

	while(cur_a < a->nserv && cur_b < b->nserv) {
		if(pa[cur_a].serv_id == pb[cur_b].serv_id) {
			if(pa[cur_a].finish || pb[cur_b].finish) {
				return 0;
			}
		//	s_log("[LOG] conflict : (%d.%d) -- (%d.%d) at (serv_id:%d)",
				//	a->id.x, a->id.y, b->id.x, b->id.y, pa[cur_a].serv_id);
			return 1;
		}
		while(cur_a < a->nserv && pa[cur_a].serv_id < pb[cur_b].serv_id) {
			cur_a++;
		}
		while(cur_b < b->nserv && pa[cur_a].serv_id > pb[cur_b].serv_id) {
			cur_b++;
		}
	}
	return 0;
}

static int ilock_conflict_all(struct s_core * core, struct s_core_lock * lock)
{
	struct s_dserver * dserv = s_core_dserv(core);

	struct s_list * p = NULL;
	s_list_foreach(p, &dserv->lock_sent) {
		struct s_core_lock * tmp = s_list_entry(p, struct s_core_lock, link);
		if(ilock_is_conflict(tmp, lock)) {
			// conflict
			return 1;
		}
	}
	return 0;
}

static void iwrite_data_and_free(struct s_core * core, struct s_id data_id)
{
	struct s_dserver * dserv = s_core_dserv(core);

	struct s_core_wait_data * wd = s_hash_get_id(dserv->waiting_data, data_id);
	if(!wd) {
		s_log("[Warning] write data, get_id NULL!");
		return;
	}

	s_log("[LOG] write size:%d", wd->sz);

	int sz = wd->sz;
	char * buf = wd->buf;

	struct timeval tv;
	gettimeofday(&tv, NULL);

	char fname[256];
	memset(fname, 0, sizeof(fname));
	snprintf(fname, 255, "tmp/%d.%d.%lu", data_id.x, data_id.y, tv.tv_usec);

	s_log("[LOG] write to file:%s", fname);

	int fd = open(fname, O_CREAT | O_WRONLY, S_IRWXU | S_IRWXG | S_IRWXO);
	if(fd < 0) {
		s_log("[Warning] write:open file error!");
		goto label_free;
	}

	srand(tv.tv_usec);
	int i;
	for(i = 0; i < sz; ++i) {
		buf[i] = rand() % 127;
	}

	if(write(fd, buf, sz) < 0) {
		s_log("[Warning] write error!");
	}

	close(fd);

label_free:
	if(buf) {
		s_free(buf);
	}
	s_hash_del_id(dserv->waiting_data, data_id);

	return ;
}

static int ilock_next(struct s_core * core, struct s_core_lock * lock, struct s_packet * pkt)
{
	if(lock->next_pos >= lock->nserv) {
		// end
	//	s_log("[LOG] End of lock, back to client");
		struct s_server * serv = s_servg_get_active_serv(core->servg, S_SERV_TYPE_C, lock->client_id);
		if(!serv) {
			s_log("[Warning] missing client:%d", lock->client_id);
			return 0;
		}
		int sz = s_packet_size_for_id(0);
		pkt = s_packet_easy(S_PKT_TYPE_LOCK_END, sz);
		s_packet_write_int(pkt, lock->id.x);
		s_packet_write_int(pkt, lock->id.y);

		s_servg_send(serv, pkt);

		s_packet_drop(pkt);

		return 0;
	}

	s_packet_set_fun(pkt, S_PKT_TYPE_LOCK_NEXT);
	s_packet_seek(pkt, 0);
	s_packet_write_int(pkt, lock->next_pos);

	struct s_server * serv = s_servg_get_active_serv(core->servg, S_SERV_TYPE_D, lock->next_serv);
	if(!serv) {
		s_log("[Warning] ilock_next: dserv missing!(%d)", lock->next_serv);
		return 0;
	}
	s_servg_send(serv, pkt);

	return 1;
}

static void ilock_unlock(struct s_core * core, struct s_core_lock * lock)
{
	if(lock->next_pos <= 1) {
		return;
	}
	struct s_core_lock_elem * elem = s_core_lock_p(lock);
	struct s_packet * pkt = s_packet_easy(S_PKT_TYPE_LOCK_UNLOCK, s_packet_size_for_id(0));
	s_packet_write_int(pkt, lock->id.x);
	s_packet_write_int(pkt, lock->id.y);

	int i;
	for(i = 0; i < lock->next_pos - 1; ++i) {
		struct s_server * serv = s_servg_get_active_serv(core->servg, S_SERV_TYPE_D, elem[i].serv_id);
		if(serv) {
			s_servg_send(serv, pkt);
		} else {
			s_log("[Warning] ilock_unlock : send back to dserv:%d missing!", elem[i].serv_id);
		}
	}

	s_packet_drop(pkt);
}

static void ilock_take_done(struct s_core * core, struct s_core_lock * lock)
{
	struct s_dserver * dserv = s_core_dserv(core);

	// send back
	ilock_unlock(core, lock);

	// send next
	int ret = ilock_next(core, lock, lock->pkt);

	// write data
	/*
	struct s_core_lock_elem * elem = s_core_lock_p(lock);
	elem[lock->next_pos - 1].finish = 1;
	iwrite_data_and_free(core, elem[lock->next_pos-1].id);
	*/

	// has next -- add to sent list
	if(ret) {
		s_list_insert_tail(&dserv->lock_sent, &lock->link);

		struct s_core_lock ** pp = s_hash_set_id(dserv->locks, lock->id);
		if(!pp) {
			s_log("[Warning] lock: set_id error!");
			return;
		}
		*pp = lock;
		return;
	}

	// finished
	s_list_del(&lock->link);
	s_hash_del_id(dserv->locks, lock->id);

	s_packet_drop(lock->pkt);
	s_free(lock);
}

void s_core_dserv_push_data(struct s_server * serv, struct s_packet * pkt, void * ud)
{
	struct s_core * core = (struct s_core *)ud;
	struct s_dserver * dserv = s_core_dserv(core);

	struct s_id data_id;
	s_packet_read(pkt, &data_id.x, int, label_error);
	s_packet_read(pkt, &data_id.y, int, label_error);

	int sz;
	if(s_packet_read_to_end(pkt, NULL, &sz) < 0) {
		s_log("[Warning] push data, read_to_end:get size error!");
		return;
	}
	s_log("[LOG] push data, id(%d, %d), size:%d", data_id.x, data_id.y, sz);

	char * buf = s_malloc(char, sz);
	if(s_packet_read_to_end(pkt, buf, &sz) < 0) {
		s_log("[Warning] push data, read_to_end error!");
		s_free(buf);
		return;
	}

	struct s_core_wait_data * wd = s_hash_set_id(dserv->waiting_data, data_id);
	if(!wd) {
		s_log("[Warning] push data, set_id error!");
		s_free(buf);
		return;
	}

	wd->sz = sz;
	wd->buf = buf;

	return;

label_error:
	s_log("[Warning] push data error!");
}

void s_core_dserv_write(struct s_server * serv, struct s_packet * pkt, void * ud)
{
	struct s_core * core = (struct s_core *)ud;
	struct s_dserver * dserv = s_core_dserv(core);
	s_used(dserv);

	struct s_id data_id;
	s_packet_read(pkt, &data_id.x, int, label_error);
	s_packet_read(pkt, &data_id.y, int, label_error);

//	s_log("[LOG] write, id(%d, %d)", data_id.x, data_id.y);

//	iwrite_data_and_free(core, data_id);

//	s_log("[LOG] write ok!");

	s_servg_rpc_ret(serv, s_packet_get_req(pkt), pkt);
	return;

label_error:
	s_log("[Warning] write data read pkt error!");
	return;
}

void s_core_dserv_lock(struct s_server * serv, struct s_packet * pkt, void * ud)
{
	struct s_core * core = (struct s_core *)ud;
	struct s_dserver * dserv = s_core_dserv(core);

	int client_id;
	int nserv;
	int curr;

	struct s_id id;

	s_packet_read(pkt, &curr, int, label_error);
	s_packet_read(pkt, &nserv, int, label_error);
	s_packet_read(pkt, &client_id, int, label_error);
	s_packet_read(pkt, &id.x, int, label_error);
	s_packet_read(pkt, &id.y, int, label_error);

	struct s_core_lock * lock = s_core_lock_create(nserv);
	struct s_core_lock_elem * elem = s_core_lock_p(lock);

	s_list_init(&lock->link);

	lock->pkt = pkt;
	s_packet_grab(pkt);

	lock->id = id;
	lock->client_id = client_id;

	lock->next_pos = curr + 1;
	lock->next_serv = -1;
	lock->nserv = nserv;

	int i;
	for(i = 0; i < nserv; ++i) {
		struct s_core_lock_elem * p = &elem[i];
		s_packet_read(pkt, &p->serv_id, int, label_error);
		s_packet_read(pkt, &p->id.x, int, label_error);
		s_packet_read(pkt, &p->id.y, int, label_error);
		p->finish = 0;
	}

	if(unlikely(elem[curr].serv_id != core->id)) {
		s_log("[Warning] %d != %d!curr:%d", elem[curr].serv_id, core->id, curr);
	}
	assert(elem[curr].serv_id == core->id);

	if(lock->next_pos < lock->nserv) {
		lock->next_serv = elem[lock->next_pos].serv_id;
	}

	if(ilock_conflict_all(core, lock)) {
		s_list_insert_tail(&dserv->lock_waiting, &lock->link);
		return;
	}

	// not conflict. send back -- lock next -- write data
	ilock_take_done(core, lock);

	return;

label_error:
	s_log("[Warning] lock start, read pkt error!");
	return;
}

void s_core_dserv_lock_unlock(struct s_server * serv, struct s_packet * pkt, void * ud)
{
	struct s_core * core = ud;
	struct s_dserver * dserv = s_core_dserv(core);

	int serv_id = s_servg_get_id(serv);

	struct s_id id;
	s_packet_read(pkt, &id.x, int, label_error);
	s_packet_read(pkt, &id.y, int, label_error);

//	s_log("[LOG] unlock:%d.%d, from:%d", id.x, id.y, serv_id);

	struct s_core_lock ** pp = s_hash_get_id(dserv->locks, id);
	if(!pp) {
		s_log("[Warning] lock unlock : get_id error(%d.%d)", id.x, id.y);
		return;
	}

	struct s_core_lock * lock = *pp;
	struct s_core_lock_elem * elem = s_core_lock_p(lock);
	int finish = 1;
	int i;
	for(i = 0; i < lock->nserv; ++i) {
		if(elem[i].serv_id == serv_id) {
			elem[i].finish = 1;
			if(!finish) {
				break;
			}
			continue;
		}
		if(!elem[i].finish) {
			finish = 0;
		}
	}

	if(finish) {
		s_hash_del_id(dserv->locks, id);
		s_list_del(&lock->link);
		s_packet_drop(lock->pkt);
		s_free(lock);
	}

	struct s_list * p, * tmp;
label_loop:
	s_list_foreach_safe(p, tmp, &dserv->lock_waiting) {
		lock = s_list_entry(p, struct s_core_lock, link);
		if(!ilock_conflict_all(core, lock)) {
			// non-conflict
			s_list_del(&lock->link);

			ilock_take_done(core, lock);

			goto label_loop;
		}
	}

	return;

label_error:
	s_log("[Warning] lock -unlock, read error!");
	return;
}


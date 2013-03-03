#ifndef s_file_reader_h_
#define s_file_reader_h_

struct s_file_reader;

struct s_file_reader *
s_file_reader_create(const char * filename);

void
s_file_reader_destroy(struct s_file_reader * fr);

char *
s_file_reader_data_p(struct s_file_reader * fr);

int
s_file_reader_data_len(struct s_file_reader * fr);

#endif


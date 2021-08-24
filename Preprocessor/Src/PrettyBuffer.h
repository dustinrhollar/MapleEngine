#ifndef _PRETTY_BUFFER_H
#define _PRETTY_BUFFER_H

typedef struct PrettyBuffer {
	char*           start;
	char*           current;
	int             cap;  // size of the allocation
	int             size; // current size from the start to the end of the contents
} PrettyBuffer;

void pb_init(PrettyBuffer *buffer, int initial_size);
void pb_free(PrettyBuffer *buffer);
void pb_clear(PrettyBuffer *buffer);
void pb_write(PrettyBuffer *buffer, char *fmt, ...);
int  pb_read_del(PrettyBuffer *buffer, void *out_buf, int out_buf_len, char del);

// move the current location forward by "dist" amount of characters
void pb_seek(PrettyBuffer *buffer, int dist);

#endif //PRETTY_BUFFER_H

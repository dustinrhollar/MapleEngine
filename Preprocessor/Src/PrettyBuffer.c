
void pb_init(PrettyBuffer *buffer, int initial_size) {
	buffer->start = 0;
	if (initial_size > 0) buffer->start = (char*)SysAlloc(initial_size);
	buffer->current = buffer->start;
	buffer->cap = initial_size;
}

void pb_free(PrettyBuffer *buffer) {
	if (buffer->start) SysFree(buffer->start);
	buffer->start = 0;
	buffer->current = 0;
	buffer->cap = 0;
}

void pb_clear(PrettyBuffer *buffer) {
	buffer->current = buffer->start;
}

int __FormatString(char *buf, int len, char *fmt, va_list list) {
	va_list cpy;
	va_copy(cpy, list);
	int needed_chars = vsnprintf(NULL, 0, fmt, cpy);
	va_end(cpy);
    
	if (buf && needed_chars < len) {
		needed_chars = vsnprintf(buf, len, fmt, list);
	}
	return needed_chars;
}

void pb_write(PrettyBuffer *buffer, char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
    
	// Check to see if we need to resize the buffer
    i64 chars_needed = __FormatString(NULL, 0, fmt, args);
	if (chars_needed + (buffer->current - buffer->start) > buffer->cap) {
        i64 offset = buffer->current - buffer->start;
		i64 min_size = chars_needed + offset;
		buffer->cap = (i32)min_size * 2;
		buffer->start = (char*)SysRealloc(buffer->start, buffer->cap);
		buffer->current = buffer->start + offset;
	}
    
    u64 leftover = buffer->cap - (buffer->current - buffer->start);
	chars_needed = __FormatString(buffer->current, (i32)leftover, fmt, args);
    
	buffer->size += (i32)chars_needed;
	buffer->current += chars_needed;
    
	va_end(args);
}


// Read from the pretty buffer based on a delimeter
int pb_read_del(PrettyBuffer *buffer, void *out_buf, int out_buf_len, char del) {
	assert(buffer->start && "Buffer not initialized!");
    
	// if the buffer is at the end of the allocation, return -1
	if ((buffer->current - buffer->start) >= buffer->size) {
		return -1;
	}
    
	const char *first_occurance = strchr(buffer->current, del);
    u64 len = 0;
	if (!first_occurance) { 
		len = buffer->size - (buffer->current - buffer->start); 
	}
	else len = first_occurance - buffer->current;
    
	assert(len >= 0 && "Attempted to read from Pretty Buffer but somehow managed to have read len less than 0!");
	//assert(out_buf_len >= len && "Attempted to read from Pretty Buffer to output buffer, but out buffer length was not large enough!\n");
    
	if (out_buf && out_buf_len >= len) {
		memcpy(out_buf, buffer->current, len);
		buffer->current += len + 1;
	}
    
	return (i32)len;
}


void pb_seek(PrettyBuffer *buffer, int dist) {
	if (dist + (buffer->current - buffer->start) >= buffer->size) {
		buffer->current = buffer->start + buffer->size; // move current to the end of the buffer
	}
	else {
		buffer->current += dist;
	}
}


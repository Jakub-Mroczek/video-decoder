struct circular_buffer {
	void *head;
	void *tail;
	void *data[];
};

/* Sets head and tail */
void circular_buffer_init(struct circular_buffer *buffer);
/* Returns value at head; if empty, blocks */
void *circular_buffer_read(struct circular_buffer *buffer);
/* Writes value at tail; if full, blocks */
void circular_buffer_write(struct circular_buffer *buffer);

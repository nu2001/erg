
#pragma once

#define BUF_SIZE 100

extern volatile int read_buf_len;
extern char read_buf[BUF_SIZE];
extern volatile int read_something;

void cdcacm_init(void);
int cdcacm_write(char *buf, int len);
void cdcacm_poll(void);

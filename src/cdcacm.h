
#pragma once

#define BUF_SIZE 100

extern volatile int read_buf_len;
extern char read_buf[BUF_SIZE];
extern volatile int read_something;

usbd_device* cdcacm_init(void);
int cdcacm_write(usbd_device *usbd_dev, char *buf, int len);

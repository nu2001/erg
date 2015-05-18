
#pragma once

usbd_device* cdcacm_init(void);
int cdcacm_write(usbd_device *usbd_dev, char *buf, int len);

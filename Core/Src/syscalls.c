#include "usbd_cdc_if.h"

int __io_putchar(int ch) 
{
	CDC_Transmit_FS((uint8_t *)&ch, 1);
	return 0;
}

int _write(int file, char *ptr, int len)
{
	CDC_Transmit_FS((uint8_t *)ptr, (uint16_t)len);
	return len;
}

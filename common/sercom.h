#ifndef SERCOM_H_INCLUDED
#define SERCOM_H_INCLUDED
#include <stdint.h>

void uart_initialize();
void set_receive_mode(void *buf, uint8_t cnt);
void send_string(const void *string, uint8_t cnt);
void send_break();
uint8_t num_bytes_received();

#endif // SERCOM_H_INCLUDED

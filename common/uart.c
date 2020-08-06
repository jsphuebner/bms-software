/**
 * @file uart.c
 * @brief Contains the serial port interface
 * @author Johannes Huebner <dev@johanneshuebner.com>
 * @date 06.04.2016
 */

#include <avr/io.h>
#include <avr/wdt.h>
#include "hwdefs.h"

extern volatile int16_t activityCounter;

int usart_send_blocking(int dummy, unsigned char c)
{
   RD_LED_PORT |= RD_LED_PIN;
   TXEN_PORT |= TXEN_PIN;
   while (!(UCSR0A & (1<<UDRE0)));  /* warten bis Senden moeglich */

   UDR0 = c;                      /* sende Zeichen */
   return 0;
}

/* Zeichen empfangen */
char usart_recv_blocking(int dummy)
{
   while (!(UCSR0A & (1<<RXC0)))   // warten bis Zeichen verfuegbar
      wdt_reset();

   activityCounter = 100;         // Watchdog zuruecksetzen
   return UDR0;                   // Zeichen aus UDR an Aufrufer zurueckgeben
}

void flush_input_buffer(int dummy)
{
   UCSR0B &= ~(1<<RXEN0);
   UCSR0B |= (1<<RXEN0);
}

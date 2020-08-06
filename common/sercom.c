/** @file
 * @brief Bit length coded communication driver
 * @author Johannes Huebner <dev@johanneshuebner.com>
 * @date 21.12.2015
 *
 */

#include <stdint.h>
#include "hwdefs.h"
#include <util/delay.h>

#define TIMER1_MODE_NONE 0
#define TIMER1_MODE_SENDER 1
#define TIMER1_MODE_RECEIVER 2

#define RECV_STATE_WAIT_EDGE  0
#define RECV_STATE_WAIT_START 1
#define RECV_STATE_RECV       2
#define RECV_STATE_DONE       3

#define SEND 0
#define RECV 1

static volatile uint8_t shiftByte;
static volatile uint8_t bitCnt;
static volatile uint8_t idle;
static uint8_t mode;
static uint8_t *curBuf;
static uint8_t inverted;
static uint8_t expectedBytes;
static uint8_t currentByte;
static uint8_t sendBits;

void uart_initialize()
{
   SETUP_BIT_TIMER();
   SETUP_RX_PINCHANGE_IRQ();
}

void set_receive_mode(void *buf, uint8_t cnt)
{
   curBuf = (uint8_t*)buf;
   currentByte = 0x0; //start receiving immediately
   expectedBytes = cnt;
   ENABLE_RX_PINCHANGE_IRQ();
}

static void send(const uint8_t *string, uint8_t cnt)
{
   mode = SEND;
   inverted = 1; //(PINA & TXRX_PIN) == 0;
   DISABLE_RX_PINCHANGE_IRQ();
   //TXRX_DDR |= TXRX_PIN;

   while (cnt > 0)
   {
      bitCnt = 0;
      shiftByte = *string;
      TCNT0 = 0;
      ENABLE_CLOCK_IRQ();
      while (bitCnt < 11);
      cnt--;
      string++;
   }
   DISABLE_CLOCK_IRQ();
   //TXRX_DDR &= ~TXRX_PIN;
}

void send_string(const uint8_t *string, uint8_t cnt)
{
   sendBits = 9; //including start bit
   send(string, cnt);
}

void send_break()
{
   uint8_t zero = 0;
   sendBits = 10;
   send(&zero, 1);
}

uint8_t num_bytes_received()
{
   return idle ? currentByte : 0;
}

RECV_TIMER_CAPT_ISR
{
   if (mode == SEND)
   {
      uint8_t pinval = !inverted; //default to stop bit

      if (bitCnt == 0)
      {
         pinval = inverted;
      }
      else if (bitCnt < sendBits)
      {
         pinval = inverted ^ (shiftByte & 1);
         //Send LSB first
         shiftByte >>= 1;
      }

      if (pinval)
         TX_HIGH();
      else
         TX_LOW();
   }
   else /*if (mode == RECV)*/
   {
      uint8_t bit = SAMPLE();
      if (bitCnt == 0)
      {
         //Check for spurious start bit
         //Start bit must always be 0
         if (bit)
         {
            DISABLE_CLOCK_IRQ();
            ENABLE_RX_PINCHANGE_IRQ();
         }
         shiftByte = 0;
      }
      else if (bitCnt < 9)
      {
         shiftByte >>= 1;

         if (bit)
         {
            shiftByte |= 1 << 7;
         }
      }
      else if (bitCnt == 9) //stop bit received
      {
         if (0 == shiftByte && 0 == bit)
         {
            //break frame - now we actually start receiving
            currentByte = 0;
         }
         else if (currentByte != 0xff)
         {
            if (currentByte < expectedBytes)
            {
               curBuf[currentByte] = shiftByte;
            }
            currentByte++;
         }
      }
      else if (bitCnt == 10)
      {
         ENABLE_RX_PINCHANGE_IRQ();
      }
      else if (bitCnt == 12)
      {
         DISABLE_CLOCK_IRQ();
         idle = 1;
      }
   }
   bitCnt++;
}

ISR(PCINT0_vect)
{
   TCNT0 = OCR0A >> 1; //Fire in the middle of first data bit
   ENABLE_CLOCK_IRQ();
   DISABLE_RX_PINCHANGE_IRQ();
   bitCnt = 0;
   idle = 0;
   mode = RECV;
   //We are in the stop bit. 0 is non-inverted.
   inverted = (RX_INPUT & RX_PIN) != 0;
}

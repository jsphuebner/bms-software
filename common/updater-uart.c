/** @file
 * @brief Firmware Updater for Attiny44
 * @author Johannes Huebner <dev@johanneshuebner.com>
 * @date 21.12.2015
 *
 * @details
 * This firmware updater resides at the end of the flash (section .bootloader).
 * To be able to compile it, you need to specify the location of the
 * bootloader section in the gcc linker command line: -Wl,--section-start=.bootloader=0xD00
 * The updater needs to be explicitly called by the application software simply
 * by calling the updater() function
 * It receives data via the custom 1-wire interface or infrared communication using the same
 * logical protocol.
 *
 * The update master transmits all pages multiple times. The updater will not exit until
 * it has received all pages without errors (crc16 check). The data structure for page
 * transmission is described in PageBuf
 */

#include <avr/io.h>
#include <avr/boot.h>
#include <util/crc16.h>
#include <util/delay.h>
#include <avr/wdt.h>
#define BAUD 115200
#include <util/setbaud.h>
#include "hwdefs.h"
#include "bms_shared.h"

/** Size of page in bytes */
#define PAGE_BYTES (PAGE_WORDS * 2)
/** number of bytes to be CRC checked */
#define CRC_BYTES  (sizeof(struct PageBuf) - sizeof(uint16_t))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

static void HWInit(void);
static uint16_t CalcCrc(uint8_t* buf);
static void WritePage(uint16_t* buf, uint8_t pageNum);
static void ReceiveString(uint8_t *string, uint16_t numBytes);


/** @brief updater entry function
 * @pre LED_PORT must be configured as output
 * @post modifies all registers without saving them
 * @post Disables interrupts
 */
void __attribute__ ((section (".bootloader"))) Updater(void)
{
   struct PageBuf pageBuf;
   uint8_t done = 0;
   uint8_t maxPage = 0;
   uint8_t expectedPage = 0;

   cli();
   HWInit();

   RD_LED_PORT |= RD_LED_PIN;

   while (!done)
   {
      ReceiveString((uint8_t*)&pageBuf, sizeof(pageBuf));
      uint16_t calcCrc = CalcCrc((uint8_t*)&pageBuf);

      if (calcCrc == pageBuf.crc && pageBuf.pageNum < ATMEGA_MAX_APPLICATION_PAGES)
      {
         maxPage = MAX(maxPage, pageBuf.pageNum);
         if (pageBuf.pageNum == expectedPage)
         {
            WritePage(pageBuf.buf, pageBuf.pageNum);
            expectedPage++;
         }

         if (pageBuf.pageNum == (ATMEGA_MAX_APPLICATION_PAGES - 1))
            done = 1;
      }
      else
      {  //indicate a CRC error by putting an E on the bus
         TXEN_PORT |= TXEN_PIN;
         UDR0 = 'E';
         _delay_ms(1);
         TXEN_PORT &= ~TXEN_PIN;
      }
      RD_LED_PORT ^= RD_LED_PIN;
   }
   boot_rww_enable ();
}

static void __attribute__ ((section (".bootloader"))) HWInit(void)
{
   UBRR0H = UBRRH_VALUE;
   UBRR0L = UBRRL_VALUE;
#if USE_2X
   /* U2X-Modus erforderlich */
   UCSR0A |= (1 << U2X0);
#else
   /* U2X-Modus nicht erforderlich */
   UCSR0A &= ~(1 << U2X0);
#endif

   UCSR0B = (1<<TXEN0) | (1<<RXEN0);
   // Frame Format: Asynchron 8N1
   UCSR0C = (1<<UCSZ01) | (1<<UCSZ00);

   RD_LED_DDR |= RD_LED_PIN;
   TXEN_DDR |= TXEN_PIN;
}

static uint16_t __attribute__ ((section (".bootloader"))) CalcCrc(uint8_t* buf)
{
   uint16_t crc = 0;
   for (uint8_t *p = buf; p < (buf + CRC_BYTES); p++)
      crc = _crc_xmodem_update(crc, *p);

   return crc;
}

static void __attribute__ ((section (".bootloader"))) WritePage(uint16_t* buf, uint8_t pageNum)
{
   uint16_t* pageAddr = (uint16_t*)((uint16_t)pageNum * PAGE_BYTES);

   boot_page_erase(pageAddr);
   boot_spm_busy_wait();      // Wait until the memory is erased.

   for (uint16_t *p = pageAddr; p < (pageAddr + PAGE_WORDS); p++, buf++)
      boot_page_fill(p, *buf);

   boot_page_write(pageAddr);
   boot_spm_busy_wait();
}

static void __attribute__ ((section (".bootloader"))) ReceiveString(uint8_t *string, uint16_t numBytes)
{
   for (;numBytes > 0; numBytes--, string++)
   {
      while (!(UCSR0A & (1<<RXC0)))
         wdt_reset();

      *string = UDR0;
   }
}

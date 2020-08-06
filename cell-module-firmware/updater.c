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
#include "hwdefs.h"
#include "bms_shared.h"

/** Size of page in bytes */
#define PAGE_BYTES (PAGE_WORDS * 2)
/** number of bytes to be CRC checked */
#define CRC_BYTES  (sizeof(struct PageBuf) - sizeof(uint16_t))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define BOOT_SECTION __attribute__ ((section (".bootloader")))

/** The application programs main entry function */
extern void __init();

static uint16_t calc_crc(uint8_t* buf);
static void write_page(uint16_t* buf, uint8_t pageNum);
static void receive_page(uint8_t *page);
static uint8_t inverted;

/** @brief updater entry function
 * @pre LED_PORT must be configured as output
 * @pre MCU must be clocked appropriately for the 1-wire protocol to work
 * @post modifies all registers without saving them
 * @post Disables interrupts
 */
void __attribute__((OS_main)) BOOT_SECTION updater(void)
{
    struct PageBuf pageBuf;
    uint8_t done = 0;
    uint8_t expectedPage = 0;

    cli();
    PCMSK0 &= ~TXRX_PIN;
    TIMSK0 &= ~(1 << OCIE0A); //disable timer interrupt
    TX_STOP();
    inverted = (PINA & TXRX_PIN) == 0;

    PORTB |= (1 << PIN2) | (1 << PIN1) | (1 << PIN0);

    while (!done)
    {
        receive_page((uint8_t*)&pageBuf);
        PORTB ^= (1 << PIN2) | (1 << PIN1) | (1 << PIN0);

        if (pageBuf.pageNum < ATTINY_MAX_APPLICATION_PAGES)
        {
           uint16_t calcCrc = calc_crc((uint8_t*)&pageBuf);

           if (calcCrc == pageBuf.crc && pageBuf.pageNum < ATTINY_MAX_APPLICATION_PAGES)
           {
               if (pageBuf.pageNum == expectedPage)
               {
                   write_page(pageBuf.buf, pageBuf.pageNum);
                   expectedPage++;
               }

               if (pageBuf.pageNum == (ATTINY_MAX_APPLICATION_PAGES - 1))
                   done = 1;
           }
           else
           {
               //indicate a CRC error by putting a 0 on the bus
               TX_START();
               TCNT0 = 0;
               TIFR0 |= 1 << OCF0A;
               while ((TIFR0 & (1 << OCF0A)) == 0);
               TX_STOP();
           }
        }
    }

    //Jump to crt entry function. It will reset the stack pointer
    __init();
}

static uint16_t BOOT_SECTION calc_crc(uint8_t* buf)
{
    uint16_t crc = 0;
    for (uint8_t *p = buf; p < (buf + CRC_BYTES); p++)
        crc = _crc_xmodem_update(crc, *p);

    return crc;
}

static void BOOT_SECTION write_page(uint16_t* buf, uint8_t pageNum)
{
    uint16_t* pageAddr = (uint16_t*)((uint16_t)pageNum * PAGE_BYTES);

    boot_page_erase(pageAddr);
    boot_spm_busy_wait();      // Wait until the memory is erased.

    for (uint16_t *p = pageAddr; p < (pageAddr + PAGE_WORDS); p++, buf++)
        boot_page_fill(p, *buf);

    boot_page_write(pageAddr);
    boot_spm_busy_wait();
}

static int16_t BOOT_SECTION receive_byte()
{
   uint8_t currentBit = 0;
   uint8_t shiftByte = 0;
   int16_t result = 0;

   while (currentBit < 10)
   {
      if (currentBit == 0)
      {
         //wait for start bit
         if (inverted)
         {
            while ((PINA & TXRX_PIN) == 0);
         }
         else
         {
            while ((PINA & TXRX_PIN) != 0);
         }
         TCNT0 = OCR0A >> 1; //Fire in the middle of first data bit
      }
      TIFR0 |= 1 << OCF0A;
      while ((TIFR0 & (1 << OCF0A)) == 0);
      //TCNT0 = 0;

      uint8_t bit = SAMPLE();

      if (currentBit == 0 && bit)
      {
         continue; //not a valid start bit
      }
      else if (currentBit > 0 && currentBit < 9)
      {
         if (bit)
         {
            shiftByte |= 1 << (currentBit - 1);
         }
         else
         {
            shiftByte &= ~(1 << (currentBit - 1));
         }
      }
      else if (currentBit == 9)
      {
         if (0 == shiftByte && !bit)
         {
            result = -1; //stop bit is low -> break
         }
         else
         {
            result = shiftByte;
         }
      }
      currentBit++;
   }
   return result;
}

static void BOOT_SECTION receive_page(uint8_t *page)
{
   uint8_t bytesReceived = 0;

   while (bytesReceived < sizeof(struct PageBuf))
   {
      int16_t result = receive_byte();

      if (result < 0) //break
      {
         bytesReceived = 0;
      }
      else
      {
         page[bytesReceived] = (uint8_t)result;
         bytesReceived++;
      }
   }
}

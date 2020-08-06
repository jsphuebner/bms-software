/** @file
 * @brief Main program of Cell Management Unit
 * @author Johannes Huebner <dev@johanneshuebner.com>
 * @date 14.01.2019
 *
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <avr/eeprom.h>
#include <util/crc16.h>
#include "sercom.h"
#include "hamming.h"
#include "measure.h"
#include "hwdefs.h"
#include "bms_shared.h"
#include "updater.h"
#include "eeprom.h"

#define MIN_ADDR          1
#define MAX_ADDR          63
#define MAX_EMPTY_CYCLES  65535
#define CALIB_CYCLES      40000
#define UPDATE_DELAY_CYCLES 30000
#define cycles            emptyCycles

int __attribute__((OS_main)) main(void);
static void CmdSetAddr(uint8_t addr);
static void CmdGetData(void);
static void CmdGetVersion(void);
static void CmdShunt(uint16_t arg);
static void HWSetup(void);
static void CheckCmd(void);
static void GoToSleep(void);

VERSION(version,2,0,13,'R',1,'A');

enum mode_t
{
   WAIT_ADDR,
   RUN,
   SLEEP
};

static uint8_t mode = WAIT_ADDR;
static uint8_t cmuAddress = 0x0;
static uint16_t emptyCycles = 0;
static struct BatValues vals;
static uint16_t curCmd[2];
static uint8_t enabledShunts = 0;
static uint16_t shuntTimeout = 0;
static uint8_t led = 0;

int main(void)
{
   HWSetup();
   adc_initialize(vals.values);
   uart_initialize();
   DISABLE_ADC_INPUT_BUFFERS();

   set_receive_mode(curCmd, sizeof(curCmd));
   sei();

   while(1)
   {
      switch (mode)
      {
      case WAIT_ADDR:
         led = (led + 1) & 0x3;
         SHUNT_SET(1 << led);
         DISABLE_PROPAGATION();

         for (cycles = 0; cycles < 1000; cycles++)
         {
            _delay_ms(1);
            CheckCmd();
         }
         break;
      case RUN:
         adc_cycle();
         CheckCmd();

         if (shuntTimeout == 0)
         {
            SHUNT_SET(0);
            enabledShunts = 0;
         }
         else
         {
            shuntTimeout--;
         }

         if (emptyCycles >= MAX_EMPTY_CYCLES)
         {
            GoToSleep();
         }
         break;
      }
   }
}

static void HWSetup(void)
{
   SHUNT_CONFIGURE();
   CLKPR = 1 << CLKPCE;
   CLKPR = 1 << CLKPS0; //4Mhz
   OSCCAL = eeprom_read_byte(&osccal_eep);
}

static void CheckCmd(void)
{
   struct cmd decodedCmd;
   uint8_t cnt = num_bytes_received();

   if (cnt == sizeof(uint16_t) || cnt == sizeof(struct cmd))
   {
      if (hamming_decode(curCmd[0], (uint16_t*)&decodedCmd)== DEC_RES_OK)
      {
         //addressed commands
         if (cmuAddress == decodedCmd.addr)
         {
            switch (decodedCmd.op)
            {
            case OP_GETDATA:
               CmdGetData();
               break;
            case OP_VERSION:
               CmdGetVersion();
               break;
            case OP_SHUNTON:
               if (cnt == sizeof(struct cmd))
                  CmdShunt(curCmd[1]);
               break;
            }
         }

         //broadcast commands
         switch (decodedCmd.op)
         {
         case OP_BOOT:
            if (decodedCmd.addr == 0xaa)
               updater();
            break;
         case OP_ADDRMODE:
            if (decodedCmd.addr == 0xaa)
               mode = WAIT_ADDR;
            break;
         case OP_SETADDR:
            if (WAIT_ADDR == mode)
               CmdSetAddr(decodedCmd.addr);
            break;
         }

         emptyCycles = 0;
      }
      set_receive_mode(curCmd, sizeof(curCmd));
   }
   else
   {
      emptyCycles++;
   }
}

static void CmdSetAddr(uint8_t addr)
{
   uint16_t encodedCmd = (OP_SETADDR << 8) | (addr + 1);
   ENABLE_PROPAGATION();
   _delay_ms(10);
   cmuAddress = addr;
   vals.adr = cmuAddress;
   encodedCmd = hamming_encode(encodedCmd);
   send_break();
   send_string(&encodedCmd, sizeof(uint16_t));
   mode = RUN;
   SHUNT_SET(0);
}

static uint16_t Crc16XModem(uint8_t* data, uint8_t len)
{
   uint16_t crc = 0;

   for (; len > 0; len--, data++)
      crc = _crc_xmodem_update(crc, *data);

   return crc;
}

static void CmdGetData(void)
{
   led = (led + 1) & 0x3;

   if (enabledShunts == 0)
   {
      SHUNT_SET(1 << led);
   }
   vals.crc = Crc16XModem((uint8_t*)&vals, sizeof(vals) - sizeof(vals.crc));
   if (enabledShunts == 0)
   {
      SHUNT_SET(0);
   }

   send_string(&vals, sizeof(vals));
}

static void CmdGetVersion(void)
{
   struct versionComm ver = { version, 0, 0 };

   ver.serialNumber = eeprom_read_dword(&serial_number);
   ver.crc = Crc16XModem((uint8_t*)&ver, sizeof(ver) - sizeof(uint16_t));

   send_string(&ver, sizeof(ver));
}

static void CmdShunt(uint16_t arg)
{
   uint16_t decodedArg;

   if (DEC_RES_OK == hamming_decode(arg, &decodedArg))
   {
      enabledShunts = decodedArg;
      SHUNT_SET(enabledShunts);
      shuntTimeout = 65535;
   }
   send_string(&arg, sizeof(arg));
}

static void GoToSleep(void)
{
   SHUNT_SET(0);
   DISABLE_PROPAGATION();
   DISABLE_ADC();
   MCUCR = (1 << BODS) | (1 << BODSE);
   MCUCR = (1 << BODS);
   set_sleep_mode(SLEEP_MODE_PWR_DOWN);
   sleep_enable();
   ENABLE_RX_PINCHANGE_IRQ();
   sleep_cpu();
   sleep_disable();
   emptyCycles = 0;
   ENABLE_ADC();
   ENABLE_PROPAGATION();
   set_receive_mode(curCmd, sizeof(curCmd));
}


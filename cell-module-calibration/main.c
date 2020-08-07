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
#include "hwdefs.h"
#include "bms_shared.h"
#include "eeprom.h"

#define SCALE_BITS         17
#define ZERO_POINT_FIVE    (1L << (SCALE_BITS - 1))

int __attribute__((OS_main)) main(void);
static void HWSetup(void);
static const uint8_t differential_channels[] = { 0x0B, 0x0F, 0x11, 0x33 };
static const uint32_t differential_offset = 148945; //124121;
static const uint16_t expected[] = { 4000, 8000, 12000, 16000 }; //put here your actual voltages
static const uint8_t refTemp = 22;
//put here the converted value of your "100mV" reference by calculating reference = U * 655360 / 3.3
static const int32_t reference = 26373; //132.8mV => 0.1328*20/3.3*512*64 = 0.1328 * 655360 / 3.3

static void SendWord(uint16_t word)
{

   TX_PORT &= ~TX_PIN;
   _delay_us(104);

   for (uint8_t i = 0; i < 16; i++)
   {
      if (word & (1 << i))
         TX_PORT |= TX_PIN;
      else
         TX_PORT &= ~TX_PIN;
      _delay_us(104);
   }
   TX_PORT |= TX_PIN;
   _delay_us(208);
}

static int32_t Sample()
{
   uint8_t samples = 65;
   int32_t value = 0;

   while (samples > 0)
   {
      ADCSRA |= 1 << ADSC;
      while ((ADCSRA & (1 << ADSC)) > 0);

      if (samples < 65)
      {
         int16_t adcVal = (ADCSRB & (1 << ADLAR)) ? (((int16_t)ADC) >> 6) : ADC;
         value += adcVal;
      }
      samples--;
   }

   return value;
}

static uint8_t OscCal()
{
   while (1)
   {
      while ((PINA & (1 << PIN6)) == 0);
      TCNT1 = 0;
      while ((PINA & (1 << PIN6)) == (1 << PIN6));
      while ((PINA & (1 << PIN6)) == 0);
      uint16_t meas = TCNT1;

      if (meas < 1999)
         OSCCAL += 1;
      else if (meas > 2002)
         OSCCAL -= 1;
      else
         return OSCCAL;

      while ((PINA & (1 << PIN6)) == (1 << PIN6));
   }
}

int main(void)
{
   uint8_t chan = 0;
   int32_t value = 0;
   uint16_t single_ended_gains[NUM_INPUTS];
   uint16_t differential_gains[NUM_INPUTS];
   uint16_t differential_gain_corr = 0;
   uint16_t temperature_offset;
   //int16_t offset;
   uint8_t doneChannels = 0;
   uint8_t oscCal;

   HWSetup();

   eeprom_read_block(single_ended_gains, &single_ended_gains_eep, sizeof(single_ended_gains));
   eeprom_read_block(differential_gains, &differential_gains_eep, sizeof(differential_gains));
   temperature_offset = eeprom_read_word(&temperature_offset_eep);

   DDRA |= (1 << PIN5);
   oscCal = OscCal();

   PORTA |= (1 << PIN5);

   _delay_ms(10000);
   ADMUX = 4;
   PORTA &= ~(1 << PIN5);
   DIDR0 = 0xFF;

   /*while (ADC < 950 || ADC > 980)
   {
      ADCSRA |= 1 << ADSC;
      while ((ADCSRA & (1 << ADSC)) > 0);
   }*/

   while(chan < NUM_INPUTS)
   {
      ADMUX = chan < 3 ? chan : chan + 1;
      value = Sample();

      uint32_t gain = single_ended_gains[chan];
      uint32_t vtg = ((ZERO_POINT_FIVE + (gain * value)) >> SCALE_BITS);
      SendWord(chan);
      SendWord(vtg);

      gain = ((uint32_t)single_ended_gains[chan] * (uint32_t)expected[chan]) / vtg;

      if (gain > (single_ended_gains[chan] - 1000) && gain < (single_ended_gains[chan] + 1000))
      {
         single_ended_gains[chan] = (uint16_t)gain;
      }
      else
      {
         chan = 1;
         while (1) //This will lead to timeout of control program
         {
            _delay_ms(100);
            PORTB = chan;
            chan <<= 1;

            if (chan == 4) chan = 1;
         }
      }

      chan++;
   }

   ADMUX = (1 << REFS1) | CHAN_TEMP;
   ADCSRA |= 1 << ADSC;
   while ((ADCSRA & (1 << ADSC)) > 0);
   ADCSRA |= 1 << ADSC;
   while ((ADCSRA & (1 << ADSC)) > 0);

   int16_t temp = ADC;
   temp -= temperature_offset;
   temperature_offset += temp - refTemp;

   SendWord(5);
   SendWord((int16_t)ADC - temperature_offset);
   _delay_ms(500);

   /*PORTB = 1;
   _delay_ms(1000);
   PORTB = 0;*/

   while (differential_gain_corr < 15000 || differential_gain_corr > 17000)
   {
      ADCSRB |= (1 << BIN) | (1 << ADLAR);
      //ADMUX = 0x25; //PA3 - PA3 - offset calibration
      //offset = Sample();
      //SendWord(10);
      //SendWord(offset);
      ADMUX = 0x3D; //PA6 - PA5
      value = Sample();
      SendWord(11);
      SendWord(value);
      differential_gain_corr = (reference << 14) / value;
      SendWord(12);
      SendWord(differential_gain_corr);
   }

   doneChannels = 0;
   chan = 0;

   while(doneChannels != 0xF)
   {
      ADMUX = differential_channels[chan];
      value = Sample();
      //value -= offset;
      value *= differential_gain_corr;
      value >>= 16;

      uint32_t gain = differential_gains[chan];
      uint32_t vtg = (ZERO_POINT_FIVE + (gain * (value + differential_offset))) >> SCALE_BITS;
      SendWord(differential_channels[chan]);
      SendWord(vtg);

      if (vtg > expected[chan])
      {
         differential_gains[chan]--;
      }
      else if (vtg < expected[chan])
      {
         differential_gains[chan]++;
      }
      else
      {
         doneChannels |= 1 << chan;
      }

      chan++;
      if (chan == NUM_INPUTS) chan = 0;
   }

   eeprom_write_block(single_ended_gains, &single_ended_gains_eep, sizeof(single_ended_gains));
   eeprom_write_block(differential_gains, &differential_gains_eep, sizeof(differential_gains));
   eeprom_write_word(&temperature_offset_eep, temperature_offset);
   eeprom_write_word(&differential_gain_corr_eep, differential_gain_corr);
   eeprom_write_byte(&osccal_eep, oscCal);
   PORTA |= (1 << PIN5);

   while (1)
   {
      PORTB |= (1 << PIN2) | (1 << PIN1) | (1 << PIN0);
      _delay_ms(500);
      PORTB &= ~((1 << PIN2) | (1 << PIN1) | (1 << PIN0));
      _delay_ms(500);
   }
}

static void HWSetup(void)
{
   CLKPR = 1 << CLKPCE;
   CLKPR = 1 << CLKPS0; //4Mhz
   TCCR1A = (1 << WGM10) | (1 << WGM11);
   TCCR1B = (1 << CS10) | (1 << WGM12) | (1 << WGM13);
   OCR1A = 0xFFF;
   OCR1B = 0x800;
   //TX_DDR |= TX_PIN;
   ADCSRA = (1 << ADPS2) | (1 << ADPS1) | (1 << ADEN);
   DDRB |= (1 << PIN2) | (1 << PIN1) | (1 << PIN0);
   PORTA = 0;
}


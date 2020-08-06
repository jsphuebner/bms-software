/*
 * This file is part of the tumanako_vc project.
 *
 * Copyright (C) 2018 Johannes Huebner <dev@johanneshuebner.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <avr/io.h>
#include "bms_shared.h"
#include "eeprom.h"
#include "hwdefs.h"

#define SAMPLES_PER_CHAN  (64 + 3)
#define DIFF_THRESHOLD     480

#define MODE_SINGLE_ENDED  0
#define MODE_BIPOLAR_DIFF  1
#define SCALE_BITS         17
#define ZERO_POINT_FIVE    (1L << (SCALE_BITS - 1))

static uint8_t sample_chan(uint8_t chan);

static uint16_t single_ended_gains[NUM_INPUTS];
static uint16_t differential_gains[NUM_INPUTS];
static uint16_t temperature_offset;

//3V virtual result for 20x PGA and 64 times over sampling, 2x left shift
static const uint32_t differential_offset =
#ifdef LEGACY
595780;
#else
//124121;
148945;
static uint16_t differential_gain_corr;
#endif // LEGACY

//Multiplexer selector for differential channels
static const uint8_t differential_channels[] = { 0x0B, 0x0F, 0x11, 0x33 };

static uint16_t* values;

void adc_initialize(uint16_t* pvalues)
{
   eeprom_read_block(single_ended_gains, &single_ended_gains_eep, sizeof(single_ended_gains));
   eeprom_read_block(differential_gains, &differential_gains_eep, sizeof(differential_gains));
   #ifndef LEGACY
   differential_gain_corr = eeprom_read_word(&differential_gain_corr_eep);
   #endif // LEGACY
   temperature_offset = eeprom_read_word(&temperature_offset_eep);
   values = pvalues;
   ADCSRA = (1 << ADPS2) | (1 << ADPS1) | (1 << ADEN); //62.5kHz
}

void adc_cycle()
{
   static uint8_t chan = 0;

   while ((ADCSRA & (1 << ADSC)) > 0);

   if (chan < NUM_INPUTS)
   {
      if (sample_chan(chan))
      {
         chan++;
         if (chan < NUM_INPUTS)
         {
            ADMUX = differential_channels[chan];
            //We use left align to force sign extension in differential mode
            ADCSRB |= (1 << BIN) | (1 << ADLAR);
         }
         else
         {
            chan = CHAN_TEMP;
            ADMUX = (1 << REFS1) | CHAN_TEMP;
            ADCSRB &= ~(1 << ADLAR);
         }
      }
   }
   else if (CHAN_TEMP == chan)
   {
      values[TEMP_IDX] = (int16_t)ADC - temperature_offset;
      chan = 0;
      ADMUX = differential_channels[chan];
      ADCSRB |= (1 << BIN) | (1 << ADLAR);
   }

   ADCSRA |= 1 << ADSC;
}

static uint8_t sample_chan(uint8_t chan)
{
   static int32_t sum = 0;
   static uint8_t samples = 0;
   static uint8_t differentialMode = 1;
   static uint16_t vtgLast = 0;
   int16_t adcVal = differentialMode ? (((int16_t)ADC) >> 6) : ADC;

   //Discard first three samples:
   //One because we switched the MUX
   //One to figure out which input to use
   //One because we possibly switched the MUX again
   if (samples > 2)
   {
      sum += adcVal;
   }

   if (samples == 1 && chan < NUM_INPUTS)
   {
      //If the differential measurement comes close to its
      //dynamic range, switch to single ended mode
      if (adcVal < -DIFF_THRESHOLD || adcVal > DIFF_THRESHOLD)
      {
         ADMUX = chan < 3 ? chan : chan + 1; //PA3 is our 3V bias
         ADCSRB &= ~(1 << ADLAR);
         differentialMode = 0;
      }
      else
      {
         differentialMode = 1;
      }
   }

   samples++;

   if (SAMPLES_PER_CHAN == samples)
   {
      #ifndef LEGACY
      if (differentialMode)
      {
         //compensate gain error of internal 20x gain stage
         sum *= differential_gain_corr;
         sum >>= 16;
      }
      #endif


      uint32_t gain = differentialMode ? differential_gains[chan] : single_ended_gains[chan];
      int32_t offset = differentialMode ? differential_offset : 0;
      uint16_t vtg = ((ZERO_POINT_FIVE + (gain * (sum + offset))) >> SCALE_BITS);

      values[chan] = chan == 0 ? vtg : vtg - vtgLast;
      vtgLast = vtg;

      sum = 0;
      samples = 0;
      differentialMode = 1;
      return 1;
   }

   return 0;
}

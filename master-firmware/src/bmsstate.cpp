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
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/crc.h>
#include "hwdefs.h"
#include "bmsstate.h"

#define STT_WORDS        (sizeof(struct savedstate) / sizeof(uint32_t))
#define STT_WORDS_NO_CRC (STT_WORDS - sizeof(uint32_t))
#define CURRENT_VERSION  1

struct savedstate
{
   uint32_t version;
   s32fp estimatedSoC;
   s32fp chargein;
   s32fp chargeout;
   uint32_t crc;
};

s32fp BMSState::estimatedSoC; //!< Member variable "estimatedSoC"
s32fp BMSState::chargein; //!< Member variable "chargein"
s32fp BMSState::chargeout; //!< Member variable "chargeout"
bool BMSState::saved = false;

void BMSState::SaveToFlash()
{
   struct savedstate stt;

   if (saved) return; //Only allow saving once per session

   crc_reset();

   stt.version = CURRENT_VERSION;
   stt.estimatedSoC = estimatedSoC;
   stt.chargein = chargein;
   stt.chargeout = chargeout;
   stt.crc = crc_calculate_block((uint32_t*)&stt, STT_WORDS_NO_CRC);

   flash_unlock();
   flash_erase_page(BATSTT_ADDRESS);

   for (uint32_t idx = 0; idx < STT_WORDS; idx++)
   {
      uint32_t* pData = ((uint32_t*)&stt) + idx;
      flash_program_word(BATSTT_ADDRESS + idx * sizeof(uint32_t), *pData);
   }
   flash_lock();
   saved = true;
}

bool BMSState::LoadFromFlash()
{
   bool res = false;
   struct savedstate *stt = (struct savedstate *)BATSTT_ADDRESS;

   crc_reset();
   uint32_t crc = crc_calculate_block((uint32_t*)stt, STT_WORDS_NO_CRC);

   if (crc == stt->crc && stt->version == CURRENT_VERSION)
   {
      estimatedSoC = stt->estimatedSoC;
      chargein = stt->chargein;
      chargeout = stt->chargeout;
      res = true;
   }

   return res;
}

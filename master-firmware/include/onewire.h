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
#ifndef ONEWIRE_H
#define ONEWIRE_H

#include <stdint.h>

extern "C" void dma1_channel5_isr();

/** @brief Implements one wire bit length encoded protocol with minimum software interaction */
class OneWire
{
   public:
      static void StartReceiveMode();
      static int GetReceivedData(uint8_t* data, int numBytes);
      static void SendData(const uint8_t* data, int numBytes);
      static bool IsReceiving();

   private:
      static const int bufferSize = 512; //large buffer because there will be many break frames in address mode
      static uint8_t buffer[bufferSize];
};

#endif // ONEWIRE_H

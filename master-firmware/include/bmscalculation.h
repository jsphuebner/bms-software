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
#ifndef BMSCALCULATION_H
#define BMSCALCULATION_H

#include <stdint.h>
#include "my_fp.h"

class BmsCalculation
{
   public:
      /** Default constructor */
      static void AggregateVoltages(int& min, int& max, int& avg, s32fp& sum);
      static s32fp GetTemperatureAverage();
      static void SetVoltageSource(const uint16_t* voltages, int numVoltages) { _voltages = voltages; _numVoltages = numVoltages; }
      static void SetTemperatureSource(const int8_t* temperatures, int numTemperatures) { _temperatures = temperatures, _numTemperatures = numTemperatures; }
      static void SetVoltageToSoCTable(const uint16_t* table);
      static void SetCharge(s32fp chargeIn, s32fp chargeOut) { _chargeIn = chargeIn, _chargeOut = chargeOut; }
      static int EstimateSocFromVoltage(uint16_t vtg);

   private:
      static s32fp _chargeIn;
      static s32fp _chargeOut;
      static uint16_t vtgToSoc[11];
      static const uint16_t* _voltages;
      static int _numVoltages;
      static const int8_t* _temperatures;
      static int _numTemperatures;
};

#endif // BMSCALCULATION_H

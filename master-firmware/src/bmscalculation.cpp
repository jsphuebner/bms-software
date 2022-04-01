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
#include "bmscalculation.h"
#include "my_math.h"

uint16_t BmsCalculation::vtgToSoc[11];
s32fp BmsCalculation::_chargeIn;
s32fp BmsCalculation::_chargeOut;
const uint16_t* BmsCalculation::_voltages;
int BmsCalculation::_numVoltages;
const int8_t* BmsCalculation::_temperatures;
int BmsCalculation::_numTemperatures;

void BmsCalculation::AggregateVoltages(int& min, int& max, int& avg, s32fp& sum)
{
   int cnt = 0;
   min = 5000;
   max = 0;
   avg = 0;

   for (int i = 0; i < _numVoltages; i++)
   {
      //ignore implausible voltages from unused channels
      if (_voltages[i] < 5000 && _voltages[i] > 50)
      {
         min = MIN(min, _voltages[i]);
         max = MAX(max, _voltages[i]);
         avg += _voltages[i];
         cnt++;
      }
   }

   sum = FP_FROMINT(avg) / 1000;
   avg /= cnt;
}

s32fp BmsCalculation::GetTemperatureAverage()
{
   s32fp tmpavg = 0;

   for (int i = 0; i < _numTemperatures; i++)
   {
      tmpavg += _temperatures[i];
   }

   tmpavg = FP_FROMINT(tmpavg);
   tmpavg /= _numTemperatures;

   return tmpavg;
}

void BmsCalculation::SetVoltageToSoCTable(const uint16_t* table)
{
   for (uint32_t i = 0; i < sizeof(vtgToSoc) / sizeof(vtgToSoc[0]); i++)
   {
      vtgToSoc[i] = table[i];
   }
}

int BmsCalculation::EstimateSocFromVoltage(uint16_t vtg)
{
   int soc = 100;
   for (uint32_t i = 0; i < sizeof(vtgToSoc) / sizeof(vtgToSoc[0]); i++)
   {
      if (vtg <= vtgToSoc[i])
      {
         if (i == 0)
         {
            soc = 0;
            break;
         }
         soc = (i - 1) * 10;
         soc += (10 * (vtg - vtgToSoc[i - 1])) / (vtgToSoc[i] - vtgToSoc[i - 1]);
         break;
      }
   }
   return soc;
}


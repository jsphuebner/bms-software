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
#ifndef ISASHUNT_H
#define ISASHUNT_H

#include "stm32_can.h"

class IsaShunt
{
   public:
      /** Default constructor */
      static void SetInterface(Can* can);
      static void Initialize();
      static void ResetCounters();
      static void RequestCharge();
      static void Stop();
      static void Start();
      static void HandleCanMessage(uint32_t id, uint32_t data[]);
      static bool IsReady() { return state == Operate; }
      static int32_t GetCurrent() { return current; }
      static int32_t GetVoltage() { return voltage; }
      static int32_t GetPower() { return power; }
      static int32_t GetChargeIn() { return chargeIn; }
      static int32_t GetChargeOut() { return chargeOut; }

      static const int CAN_ID_REPLY = 0x511;
      static const int CAN_ID_CURRENT = 0x521;
      static const int CAN_ID_VOLTAGE = 0x522;
      static const int CAN_ID_POWER = 0x526;

   protected:

   private:
      enum IsaStates
      {
         Init, Reset, RequestChargeOut, ReadChargeOut, Operate
      };
      static void RunStateMachine(uint32_t data[]);
      static Can *_can;
      static int32_t current;
      static int32_t voltage;
      static int32_t power;
      static uint32_t chargeIn;
      static uint32_t chargeOut;
      static enum IsaStates state;
};

#endif // ISASHUNT_H

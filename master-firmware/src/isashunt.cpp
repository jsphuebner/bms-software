/*
 * This file is part of the tumanako_vc project.
 *
 * Copyright (C) 2020 Johannes Huebner <dev@johanneshuebner.com>
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
#include "isashunt.h"

Can* IsaShunt::_can;
int32_t IsaShunt::current;
int32_t IsaShunt::voltage;
int32_t IsaShunt::power;
uint32_t IsaShunt::chargeIn;
uint32_t IsaShunt::chargeOut;
IsaShunt::IsaStates IsaShunt::state;
static int channel;

void IsaShunt::SetInterface(Can *can)
{
   _can = can;
}

void IsaShunt::Initialize()
{
   state = Init;
   channel = 0;
}

void IsaShunt::ResetCounters()
{
   uint32_t configData[2] = { 0x0030, 0 };

   Stop();
   _can->Send(0x411, configData);
   Start();
}

void IsaShunt::RequestCharge()
{
   uint32_t configData[2] = { 0x0243, 0 };

   _can->Send(0x411, configData);
   state = RequestChargeOut;
}

void IsaShunt::Start()
{
   uint32_t configData[2] = { 0x0134, 0 };

   _can->Send(0x411, configData);
}

void IsaShunt::Stop()
{
   uint32_t configData[2] = { 0x0034, 0 };

   _can->Send(0x411, configData);
}

void IsaShunt::HandleCanMessage(uint32_t id, uint32_t data[])
{
   if (state == Init && channel == 0)
   {
      Stop();
   }

   switch (id)
   {
   case CAN_ID_REPLY:
      RunStateMachine(data);
      break;
   case CAN_ID_CURRENT:
      current = (data[0] >> 16) + (data[1] << 16);
      break;
   case CAN_ID_VOLTAGE:
      voltage = (data[0] >> 16) + (data[1] << 16);
      break;
   case CAN_ID_POWER:
      power = (data[0] >> 16) + (data[1] << 16);
      break;
   }
}

void IsaShunt::RunStateMachine(uint32_t data[])
{
   if (state == Init)
   {
      //Data channels
      //00: I [mA]
      //01: U1 [mV]
      //05: P [W]
      //06: As [As]
      //07: Wh [Wh]

      if (channel < 8)
      {
         //config channel I (0x20), little endian/disabled (0x40), 100ms (0x64, 0x00)
         uint32_t configData[2] = { 0x64004020, 0 };

         configData[0] |= channel;
         //Only enable I, U1 and P
         if (channel == 0 || channel == 1 || channel == 5)
            configData[0] |= 0x200; //cyclic trigger
         _can->Send(0x411, configData);
         channel++;
      }
      else
      {
         Start();
         state = Operate;
      }
   }
   else if (state == RequestChargeOut) //Read charge in and request charge out
   {
      uint32_t configData[2] = { 0x0343, 0 };
      uint8_t* u8data = (uint8_t*)data;

      chargeIn = u8data[7] + (u8data[6] << 8) + (u8data[5] << 16) + (u8data[4] << 24);

      _can->Send(0x411, configData);
      state = ReadChargeOut;
   }
   else if (state == ReadChargeOut)
   {
      uint8_t* u8data = (uint8_t*)data;
      chargeOut = u8data[7] + (u8data[6] << 8) + (u8data[5] << 16) + (u8data[4] << 24);
      state = Operate;
   }
}

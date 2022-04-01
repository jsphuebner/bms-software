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
#ifndef BMSCOMM_H
#define BMSCOMM_H
#include "bms_shared.h"

class BmsComm
{
   public:
      static void SetAddress();
      static int GetNumberOfCellModules();
      static void ResetAddress();
      static void StartAcquisition(int slave);
      static bool Acquire(int slave);
      static void StartVersionAcquisition(int slave);
      static bool AcquireVersion(int slave);
      static void SetShunt(int slave, int vtg);
      static void StartUpdate();
      static int UpdateNextPage();
      static const uint16_t* GetVoltages();
      static const int8_t* GetTemperatures();
      static const struct version* GetVersions();
      static const int voltagesPerModule = 4;
      static const int MaxModules = 64;

   protected:

   private:
      static int Crc16XModem(uint8_t *addr, int num);
      static void SendEncodedCmd(struct cmd *cmd);
      static int numModules;
      static PageBuf pageBuf;
      static uint16_t voltages[MaxModules * voltagesPerModule];
      static int8_t temperatures[MaxModules];
      static struct version versions[MaxModules];
};

#endif // BMSCOMM_H

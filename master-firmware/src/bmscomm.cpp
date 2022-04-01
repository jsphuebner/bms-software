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

#include "bmscomm.h"
#include "bms_shared.h"
#include "onewire.h"
#include "hamming.h"
#include "params.h"

#define poly 0x1021
#define NUM_DATA_BYTES (NUM_DATA_BITS / 8)
#define NUM_CMD_BYTES  (NUM_CMD_BITS / 8)
#define NUM_PARAM_BYTES  (NUM_PARAM_BITS / 8)

uint16_t BmsComm::voltages[];
int8_t BmsComm::temperatures[];
struct version BmsComm::versions[];
int BmsComm::numModules = -1;
PageBuf BmsComm::pageBuf;

void BmsComm::SetAddress()
{
   struct cmd cmd = { 0x1, OP_SETADDR, 0 };

   SendEncodedCmd(&cmd);
}

void BmsComm::ResetAddress()
{
   struct cmd cmd = { 0xaa, OP_ADDRMODE, 0 };

   numModules = -1;
   SendEncodedCmd(&cmd);
}

int BmsComm::GetNumberOfCellModules()
{
   if (numModules < 0)
   {
      struct cmd cmd;
      uint16_t encodedCmd;
      int numbytes = OneWire::GetReceivedData((uint8_t*)&encodedCmd, sizeof(encodedCmd));

      if (numbytes != sizeof(encodedCmd)) return -1;

      if (hamming_decode(encodedCmd, (uint16_t*)&cmd) == DEC_RES_OK)
      {
         if (cmd.addr > MaxModules)
            return -1;
         numModules = cmd.addr - 1;
      }
   }
   return numModules;
}

void BmsComm::StartAcquisition(int slave)
{
   struct cmd cmd = { (uint8_t)slave, OP_GETDATA, 0 };

   SendEncodedCmd(&cmd);
}

bool BmsComm::Acquire(int slave)
{
   struct BatValues batValues;
   int numbytes = OneWire::GetReceivedData((uint8_t*)&batValues, sizeof(batValues));
   int offset = 4 * (slave - 1);

   if (numbytes != sizeof(batValues)) return false;

   int crc = Crc16XModem((uint8_t*)&batValues, sizeof(batValues) - sizeof(uint16_t));

   if (crc != batValues.crc) return false;

   for (int i = 0; i < voltagesPerModule; i++)
   {
      voltages[i + offset] = batValues.values[i];
   }

   temperatures[slave - 1] = (int8_t)(batValues.values[4] & 0xFF);

   return true;
}

void BmsComm::StartVersionAcquisition(int slave)
{
   struct cmd cmd = { (uint8_t)slave, OP_VERSION, 0 };

   SendEncodedCmd(&cmd);
}

bool BmsComm::AcquireVersion(int slave)
{
   struct versionComm version;
   int numbytes = OneWire::GetReceivedData((uint8_t*)&version, sizeof(version));

   if (numbytes != sizeof(version)) return false;

   int crc = Crc16XModem((uint8_t*)&version, sizeof(version) - sizeof(uint16_t));

   if (crc != version.crc) return false;

   versions[slave - 1] = version.version;

   return true;
}

void BmsComm::SetShunt(int slave, int vtg)
{
   struct cmd cmd;
   uint16_t encodedCmd[2];
   int offset = 4 * (slave - 1);

   cmd.op = OP_SHUNTON;
   cmd.addr = slave;
   cmd.arg = 0;

   for (int i = 0; i < voltagesPerModule; i++)
   {
      cmd.arg |= (voltages[i + offset] > vtg && voltages[i + offset] < 5000) << i;
   }

   encodedCmd[0] = hamming_encode(*((uint16_t*)&cmd));
   encodedCmd[1] = hamming_encode(cmd.arg);

   OneWire::SendData((const uint8_t*)&encodedCmd, sizeof(encodedCmd));
}

const uint16_t* BmsComm::GetVoltages()
{
   return voltages;
}

const int8_t* BmsComm::GetTemperatures()
{
   return temperatures;
}

const struct version* BmsComm::GetVersions()
{
   return versions;
}

void BmsComm::StartUpdate()
{
   struct cmd cmd = { 0xAA, OP_BOOT, 0x1234 };

   uint16_t encodedCmd[2] = { hamming_encode(*((uint16_t*)&cmd)), cmd.arg };
   pageBuf.pageNum = 0;
   OneWire::SendData((uint8_t*)&encodedCmd, sizeof(encodedCmd));
}

int BmsComm::UpdateNextPage()
{
   extern uint16_t _binary_bms_tiny_elf_bin_start[2048];

   if (OneWire::IsReceiving() && pageBuf.pageNum > 0)
   {
      uint8_t slaveReply = 0;

      OneWire::GetReceivedData(&slaveReply, 1);

      if (slaveReply != 0)
      {
         //resend last page
         pageBuf.pageNum--;
      }
   }

   if (pageBuf.pageNum < (ATTINY_MAX_APPLICATION_PAGES + 4) && OneWire::IsReceiving())
   {
      uint16_t* const page = &_binary_bms_tiny_elf_bin_start[PAGE_WORDS * pageBuf.pageNum];
      for (int i = 0; i < PAGE_WORDS; i++)
         pageBuf.buf[i] = page[i];
      pageBuf.crc = Crc16XModem((uint8_t*)&pageBuf, PAGE_WORDS * sizeof(uint16_t) + sizeof(uint8_t));

      OneWire::SendData((uint8_t*)&pageBuf, sizeof(pageBuf));
      pageBuf.pageNum++;
   }
   return pageBuf.pageNum;
}

void BmsComm::SendEncodedCmd(struct cmd *cmd)
{
   uint16_t encodedCmd = hamming_encode(*((uint16_t*)cmd));
   OneWire::SendData((const uint8_t*)&encodedCmd, sizeof(uint16_t));
}

/* On entry, addr=>start of data
             num = length of data
             crc = incoming CRC     */
int BmsComm::Crc16XModem(uint8_t *addr, int num)
{
   int i, crc = 0;

   for (; num>0; num--)               /* Step through bytes in memory */
   {
      crc = crc ^ (*addr++ << 8);      /* Fetch byte from memory, XOR into CRC top byte*/
      for (i=0; i<8; i++)              /* Prepare to rotate 8 bits */
      {
         crc = crc << 1;                /* rotate */
         if (crc & 0x10000)             /* bit 15 was set (now bit 16)... */
            crc = (crc ^ poly) & 0xFFFF; /* XOR with XMODEM polynomic */
                                   /* and ensure CRC remains 16-bit value */
    }                              /* Loop for 8 bits */
  }                                /* Loop until num=0 */
  return(crc);                     /* Return updated CRC */
}

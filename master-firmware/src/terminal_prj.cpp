/*
 * This file is part of the tumanako_vc project.
 *
 * Copyright (C) 2011 Johannes Huebner <dev@johanneshuebner.com>
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

#include <libopencm3/stm32/usart.h>
#include "hwdefs.h"
#include "terminal.h"
#include "params.h"
#include "my_string.h"
#include "my_fp.h"
#include "printf.h"
#include "param_save.h"
#include "errormessage.h"
#include "stm32_can.h"
#include "bmscomm.h"
#include "terminalcommands.h"

static void PrintVoltages(Terminal* t, char* arg);
static void ParamStream(Terminal* t, char *arg);
static void LoadDefaults(Terminal* t, char *arg);
static void SaveParameters(Terminal* t, char *arg);
static void LoadParameters(Terminal* t, char *arg);
static void Help(Terminal* t, char *arg);
static void PrintParamsJson(Terminal* t, char *arg);
static void PrintSerial(Terminal* t, char *arg);
static void PrintErrors(Terminal* t, char *arg);

extern "C" const TERM_CMD TermCmds[] =
{
  { "voltages", PrintVoltages },
  { "set", TerminalCommands::ParamSet },
  { "get", TerminalCommands::ParamGet },
  { "flag", TerminalCommands::ParamFlag },
  { "stream", ParamStream },
  { "defaults", LoadDefaults },
  { "save", SaveParameters },
  { "load", LoadParameters },
  { "help", Help },
  { "json", PrintParamsJson },
  { "can", TerminalCommands::MapCan },
  { "serial", PrintSerial },
  { "errors", PrintErrors },
  { "reset", TerminalCommands::Reset },
  { NULL, NULL }
};

static void PrintVoltages(Terminal* t, char* arg)
{
   t = t;
   arg = arg;
}

static void PrintParamsJson(Terminal* t, char *arg)
{
   const Param::Attributes *pAtr;
   int numSlaves = BmsComm::GetNumberOfCellModules();
   const uint16_t* voltages = BmsComm::GetVoltages();
   const int8_t* temperatures = BmsComm::GetTemperatures();
   const struct version* versions = BmsComm::GetVersions();

   t = t;
   arg = arg;
   printf("{");
   for (uint32_t idx = 0; idx < Param::PARAM_LAST; idx++)
   {
      int canId, canOffset, canLength;
      bool isRx;
      s32fp canGain;
      pAtr = Param::GetAttrib((Param::PARAM_NUM)idx);

      if ((Param::GetFlag((Param::PARAM_NUM)idx) & Param::FLAG_HIDDEN) == 0)
      {
         printf("\r\n   \"%s\": {\"unit\":\"%s\",\"value\":%f,", pAtr->name, pAtr->unit, Param::Get((Param::PARAM_NUM)idx));

         if (Can::GetInterface(0)->FindMap((Param::PARAM_NUM)idx, canId, canOffset, canLength, canGain, isRx))
         {
            printf("\"canid\":%d,\"canoffset\":%d,\"canlength\":%d,\"cangain\":%d,\"isrx\":%s,",
                   canId, canOffset, canLength, canGain, isRx ? "true" : "false");
         }

         if (Param::IsParam((Param::PARAM_NUM)idx))
         {
            printf("\"isparam\":true,\"minimum\":%f,\"maximum\":%f,\"default\":%f,\"category\":\"%s\"},", pAtr->min, pAtr->max, pAtr->def, pAtr->category);
         }
         else
         {
            printf("\"isparam\":false},");
         }
      }
   }
   printf("\r\n   \"serial\": {\"unit\":\"\",\"value\":\"%X:%X:%X\",\"isparam\":false}", DESIG_UNIQUE_ID2, DESIG_UNIQUE_ID1, DESIG_UNIQUE_ID0);

   for (int slave = 0; slave < numSlaves; slave++)
   {
      const uint8_t* ver = versions[slave].swVersion;
      for (int channel = 0; channel < BmsComm::voltagesPerModule; channel++)
      {
         uint16_t vtg = voltages[4 * slave + channel];
         if (vtg < 5000)
            printf(",\r\n   \"u.%02d.%d\": {\"unit\":\"mV\",\"value\":%d,\"isparam\":false}", slave + 1, channel + 1, vtg);
      }
      printf(",\r\n   \"t.%02d\": {\"unit\":\"°C\",\"value\":%d,\"isparam\":false}", slave + 1, temperatures[slave]);
      printf(",\r\n   \"swver.%02d\": {\"unit\":\"\",\"value\":\"%d.%d.%d.%c\",\"isparam\":false}", slave + 1, ver[0], ver[1], ver[2], ver[3]);
   }

   printf("\r\n}\r\n");
}

static int GetBatVoltageIdx(char* arg)
{
   if (arg[0] == 'u')
   {
      arg += 2; //Skip u.
      char* dot = (char*)my_strchr(arg, '.');
      *dot = 0;
      int mod = my_atoi(arg) - 1;
      *dot = '.';
      arg = dot + 1;
      int cell = my_atoi(arg) - 1;
      return BmsComm::voltagesPerModule * mod + cell;
   }
   return -1;
}

static void ParamStream(Terminal* t, char *arg)
{
   Param::PARAM_NUM indexes[10];
   int vindexes[10];
   int maxIndex = sizeof(indexes) / sizeof(Param::PARAM_NUM);
   int curIndex = 0;
   int repetitions = -1;
   char* comma;
   char orig;

   t = t;
   arg = my_trim(arg);
   repetitions = my_atoi(arg);
   arg = (char*)my_strchr(arg, ' ');

   if (0 == *arg)
   {
      printf("Usage: stream n val1,val2...\r\n");
      return;
   }
   arg++; //move behind space

   do
   {
      comma = (char*)my_strchr(arg, ',');
      orig = *comma;
      *comma = 0;

      Param::PARAM_NUM idx = Param::NumFromString(arg);
      int vidx = GetBatVoltageIdx(arg);

      *comma = orig;
      arg = comma + 1;

      indexes[curIndex] = idx;
      vindexes[curIndex] = vidx;
      curIndex++;
   } while (',' == *comma && curIndex < maxIndex);

   maxIndex = curIndex;
   usart_recv(TERM_USART);

   while (!usart_get_flag(TERM_USART, USART_SR_RXNE) && (repetitions > 0 || repetitions == -1))
   {
      comma = (char*)"";
      for (curIndex = 0; curIndex < maxIndex; curIndex++)
      {
         s32fp val;
         if (indexes[curIndex] == Param::PARAM_INVALID)
            val = FP_FROMINT(BmsComm::GetVoltages()[vindexes[curIndex]]);
         else
            val = Param::Get(indexes[curIndex]);
         printf("%s%f", comma, val);
         comma = (char*)",";
      }
      printf("\r\n");
      if (repetitions != -1)
         repetitions--;
   }
}

static void LoadDefaults(Terminal* t, char *arg)
{
   t = t;
   arg = arg;
   Param::LoadDefaults();
   printf("Defaults loaded\r\n");
}

static void SaveParameters(Terminal* t, char *arg)
{
   t = t;
   Can::GetInterface(0)->Save();
   printf("CANMAP stored\r\n");
   uint32_t crc = parm_save();

   arg = arg;
   printf("Parameters stored, CRC=%x\r\n", crc);
}

static void LoadParameters(Terminal* t, char *arg)
{
   arg = arg;
   t = t;
   if (0 == parm_load())
   {
      parm_Change((Param::PARAM_NUM)0);
      printf("Parameters loaded\r\n");
   }
   else
   {
      printf("Parameter CRC error\r\n");
   }
}

static void PrintErrors(Terminal* t, char *arg)
{
   arg = arg;
   t = t;
   ErrorMessage::PrintAllErrors();
}

static void PrintSerial(Terminal* t, char *arg)
{
   arg = arg;
   t = t;
   printf("%X%X%X\r\n", DESIG_UNIQUE_ID2, DESIG_UNIQUE_ID1, DESIG_UNIQUE_ID0);
}

static void Help(Terminal* t, char *arg)
{
   t = t;
   arg = arg;
}



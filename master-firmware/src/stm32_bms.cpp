/*
 * This file is part of the tumanako_vc project.
 *
 * Copyright (C) 2010 Johannes Huebner <contact@johanneshuebner.com>
 * Copyright (C) 2010 Edward Cheeseman <cheesemanedward@gmail.com>
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
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
#include <stdint.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/rtc.h>
#include <libopencm3/stm32/iwdg.h>
#include <libopencm3/stm32/can.h>
#include <libopencm3/stm32/crc.h>
#include "stm32_can.h"
#include "terminal.h"
#include "params.h"
#include "hwdefs.h"
#include "digio.h"
#include "hwinit.h"
#include "anain.h"
#include "param_save.h"
#include "my_math.h"
#include "errormessage.h"
#include "printf.h"
#include "stm32scheduler.h"
#include "onewire.h"
#include "bmscomm.h"
#include "bmscalculation.h"
#include "bmsstate.h"
#include "isashunt.h"

#define CAN_TIMEOUT       50  //500ms

static Stm32Scheduler* scheduler;
static Can* can1;
static Can* can2;
static uint32_t noCurrentMillis = 0;
static uint32_t ignOffTime = 0;
static const uint16_t lfpVtgToSoc[] = { 2700, 3223, 3257, 3283, 3292, 3295, 3302, 3323, 3331, 3334, 3357 };

static void Ms100Task(void)
{
   static int relayStopCnt = 0;
   static bool lastIgnState = true;
   static s32fp batmaxFiltered = 0, batminFiltered = 0;
   States state = (States)Param::GetInt(Param::opmode);

   iwdg_reset();
   DigIo::LedOut.Toggle();

   s32fp cpuLoad = FP_FROMINT(scheduler->GetCpuLoad());
   Param::SetFlt(Param::cpuload, cpuLoad / 10);

   if (IsaShunt::IsReady())
   {
      IsaShunt::RequestCharge();

      Param::SetInt(Param::chargein, IsaShunt::GetChargeIn());
      Param::SetInt(Param::chargeout, IsaShunt::GetChargeOut());
      BmsCalculation::SetCharge(FP_FROMINT(IsaShunt::GetChargeIn()), FP_FROMINT(IsaShunt::GetChargeOut()));
   }

   Param::SetInt(Param::ignition, DigIo::IgnIn.Get());
   Param::SetInt(Param::relay, DigIo::Relay.Get());
   Param::SetInt(Param::trun, rtc_get_counter_val());

   if (DigIo::IgnIn.Get())
   {
      Param::SetInt(Param::ttostandby, -1);
   }
   else
   {
      int ttostandby = ignOffTime - rtc_get_counter_val() + 7200;
      //edge detection
      if (lastIgnState)
      {
         ignOffTime = rtc_get_counter_val();
      }
      Param::SetInt(Param::ttostandby, ttostandby);

      if (ttostandby == 0)
      {
         BMSState::SaveToFlash();
         DigIo::BoardPower.Clear();
      }
   }

   if (Param::GetInt(Param::commquality) > 0 && (state == Run || state == Shunt))
   {
      batmaxFiltered = IIRFILTER(batmaxFiltered, Param::Get(Param::batmax), 7);
      batminFiltered = IIRFILTER(batminFiltered, Param::Get(Param::batmin), 7);
      s32fp vtgErr = Param::Get(Param::chargestop) - batmaxFiltered;
      s32fp curLim = vtgErr / 4;
      curLim = MIN(Param::Get(Param::chgmaxcur), curLim);
      curLim = MAX(0, curLim);
      Param::SetFlt(Param::chargelim, curLim);

      vtgErr = batminFiltered - Param::Get(Param::loadstop);
      curLim = vtgErr / 4;
      curLim = MIN(Param::Get(Param::dismaxcur), curLim);
      curLim = MAX(0, curLim);
      Param::SetFlt(Param::dislim, curLim);
   }
   else
   {
      Param::SetFlt(Param::dislim, 0);
      Param::SetFlt(Param::chargelim, 0);
   }

   lastIgnState = DigIo::IgnIn.Get();

   switch (Param::GetInt(Param::testcmd))
   {
   case AllOn:
      DigIo::SensPower.Set();
      DigIo::WifiPower.Set();
      break;
   case WifiOff:
      DigIo::SensPower.Set();
      DigIo::WifiPower.Clear();
      break;
   case CursensOff:
      DigIo::SensPower.Clear();
      DigIo::WifiPower.Set();
      break;
   case AllOff:
      DigIo::SensPower.Clear();
      DigIo::WifiPower.Clear();
      break;
   case BoardOff: //This disables the 5V voltage regulator if IGN is low
      DigIo::BoardPower.Clear();
      break;
   case EstSoC:
      int soc = BmsCalculation::EstimateSocFromVoltage(Param::GetInt(Param::batavg));
      Param::SetInt(Param::socest, soc);
      Param::SetInt(Param::soc, soc);
      BMSState::SetEstimatedSoC(FP_FROMINT(soc));
      BMSState::SaveToFlash();
      break;
   }

   if (DigIo::IgnIn.Get())
   {
      Param::SetInt(Param::testcmd, AllOn);
   }

   if (Param::GetInt(Param::relaytest) == 1)
   {
      DigIo::Relay.Set();
   }
   else if (Param::GetInt(Param::relaytest) == 0)
   {
      DigIo::Relay.Clear();
   }
   else if (Param::GetInt(Param::relaymode) == CurThresh)
   {
      if (Param::Get(Param::idc) < Param::Get(Param::relayhyst))
      {
         DigIo::Relay.Clear();
      }
      else if (Param::Get(Param::idc) > Param::Get(Param::relaythresh))
      {
         DigIo::Relay.Set();
      }
   }
   else if (state == Run || state == Shunt)
   {
      int batmax = Param::GetInt(Param::batmax);
      int batmin = Param::GetInt(Param::batmin);

      if (batmax > Param::GetInt(Param::chargestop) || batmin < Param::GetInt(Param::loadstop) || Param::GetInt(Param::commquality) == 0)
      {
         if (relayStopCnt == 0)
            DigIo::Relay.Clear();
         else
            relayStopCnt--;
      }
      else if ((batmax < Param::GetInt(Param::chargestart) && batmax > 0) || batmin > Param::GetInt(Param::loadstart))
      {
         DigIo::Relay.Set();
         relayStopCnt = 100; //10 seconds
      }
   }
   else
   {
      DigIo::Relay.Clear();
   }

   if (Param::GetInt(Param::canperiod) == CAN_PERIOD_100MS)
      can1->SendAll();
}

static void CellModuleCommunication()
{
   static int numCellMods;
   static int timeout = 10, commTimeout = 10;
   static int currentCellMod = 1;
   static int lastSocEst = -1;
   static bool inverted = false;
   static bool commRunning = true;
   static States state = Start;

   Param::SetInt(Param::curmodule, currentCellMod);

   if (state == Run)
   {
      commRunning &= BmsComm::Acquire(currentCellMod);

      if (currentCellMod < numCellMods)
      {
         currentCellMod++;
      }
      else
      {
         currentCellMod = 1;
         int min, max, avg;
         s32fp voltageSum;

         BmsCalculation::AggregateVoltages(min, max, avg, voltageSum);

         if (!commRunning)
         {
            if (commTimeout > 0)
            {
               commTimeout--;
            }
            commRunning = true; //will be reset if one module does not respond
         }
         else
         {
            commTimeout = 10;
         }

         Param::SetInt(Param::commquality, commTimeout * 10);

         s32fp udc = voltageSum;
         udc += Param::Get(Param::udc2);
         Param::SetFlt(Param::udc, udc);

         if (Param::Get(Param::batavg2) > 0)
         {
            avg += Param::GetInt(Param::batavg2);
            avg /= 2;
            min = MIN(min, Param::GetInt(Param::batmin2));
            max = MAX(max, Param::GetInt(Param::batmax2));
         }

         if (noCurrentMillis > 3600000)
         {
            static int numEstimates = 0;

            int soc = BmsCalculation::EstimateSocFromVoltage(avg);
            s32fp chargein = Param::Get(Param::chargein);
            s32fp chargeout = Param::Get(Param::chargeout);
            Param::SetInt(Param::socest, soc);
            Param::SetInt(Param::soc, soc);
            Param::SetFlt(Param::chargein, 0);
            Param::SetFlt(Param::chargeout, 0);
            BMSState::SetEstimatedSoC(FP_FROMINT(soc));
            numEstimates++;

            if (numEstimates > 10)
            {
               if (lastSocEst < 0)
               {
                  lastSocEst = soc;
               }
               //The battery has been charged more than 50% since last estimate
               else if (lastSocEst < soc && (soc - lastSocEst) > 50)
               {
                  s32fp chargeDelta = chargein - chargeout;
                  lastSocEst = soc; //Now we don't come here again
                  s32fp capacity = chargeDelta * 100 / (soc - lastSocEst);
                  capacity /= 3600; //As to Ah
                  Param::SetFlt(Param::capacity, capacity);
               }
               //The battery has been discharged more than 50% since last estimate
               else if (lastSocEst > soc && (lastSocEst - soc) > 50)
               {
                  s32fp chargeDelta = chargeout - chargein;
                  lastSocEst = soc; //Now we don't come here again
                  s32fp capacity = chargeDelta * 100 / (lastSocEst - soc);
                  capacity /= 3600; //As to Ah
                  Param::SetFlt(Param::capacity, capacity);
               }
            }
         }
         else
         {
            s32fp soc = Param::Get(Param::socest);
            s32fp chargeDiff = Param::Get(Param::chargein) - Param::Get(Param::chargeout);
            chargeDiff /= 36; //From As to Ah times 100%
            soc += FP_DIV(chargeDiff, Param::Get(Param::capacity));
            Param::SetFlt(Param::soc, soc);
         }
         Param::SetInt(Param::batmin, min);
         Param::SetInt(Param::batmax, max);
         Param::SetInt(Param::batavg, avg);
         Param::SetFlt(Param::tmpavg, BmsCalculation::GetTemperatureAverage());
      }
   }
   else if (state == GetVersion)
   {
      if (BmsComm::AcquireVersion(currentCellMod))
      {
         currentCellMod++;
         timeout = 10;
      }
   }

   switch (state)
   {
      case Start:
         BmsComm::StartVersionAcquisition(1); //Send random command to wake up cell modules
         timeout--;
         if (timeout == 0)
            state = ResetAddress;
         break;
      case ResetAddress:
         BmsComm::ResetAddress();
         state = SetAddress;
         break;
      case SetAddress:
         BmsComm::SetAddress();
         state = WaitAddress;
         timeout = 30;
         break;
      case WaitAddress:
         numCellMods = BmsComm::GetNumberOfCellModules();
         timeout--;
         if (numCellMods == Param::GetInt(Param::modcount))
         {
            timeout = 20;
            state = WaitReady;
         }
         else if (timeout == 0)
         {
            state = ResetAddress;
            if (inverted)
            {
               default_bms_uart();
               inverted = false;
            }
            else
            {
               remap_bms_uart();
               inverted = true;
            }
         }
         break;
      case WaitReady:
         timeout--;
         if (timeout == 0)
         {
            BmsCalculation::SetVoltageSource(BmsComm::GetVoltages(), BmsComm::GetNumberOfCellModules() * BmsComm::voltagesPerModule);
            BmsCalculation::SetTemperatureSource(BmsComm::GetTemperatures(), BmsComm::GetNumberOfCellModules());
            state = GetVersion;
            timeout = 20;
         }
         break;
      case GetVersion:
         timeout--;

         if (timeout == 0)
         {
            state = ResetAddress;
         }
         if (currentCellMod > numCellMods)
         {
            state = Run;
         }
         else
         {
            BmsComm::StartVersionAcquisition(currentCellMod);
         }
         break;
      case Run:
         timeout--;

         if (Param::GetInt(Param::cellmodop) == FWUpgrade)
         {
            BmsComm::StartUpdate();
            state = SWUpgrade;
            timeout = 0;
            Param::SetInt(Param::cellmodop, None);
         }
         else if (Param::GetInt(Param::cellmodop) == AssignAddress)
         {
            state = ResetAddress;
            Param::SetInt(Param::cellmodop, None);
         }
         else if (Param::GetInt(Param::cellmodop) == StopAcq)
         {
            state = Standby;
         }
         else if (timeout <= 0)
         {
            state = Shunt;
            currentCellMod = 1;
         }
         else
         {
            BmsComm::StartAcquisition(currentCellMod);
         }
         break;
      case Standby:
         if (Param::GetInt(Param::cellmodop) != StopAcq)
            state = Run;
         break;
      case Shunt:
         BmsComm::SetShunt(currentCellMod, Param::GetInt(Param::shuntvtg));
         currentCellMod++;
         if (currentCellMod > numCellMods)
         {
            state = Run;
            timeout = 300;
         }
         break;
      case SWUpgrade:
         if (timeout == 0)
         {
            int page = BmsComm::UpdateNextPage();

            if (page == (ATTINY_MAX_APPLICATION_PAGES + 4))
            {
               state = ResetAddress;
            }
            timeout = 4;
         }
         timeout--;
         break;
   }

   Param::SetInt(Param::opmode, state);
}

static void Ms10Task(void)
{
   int dcoffset = Param::GetInt(Param::gaugeoffset);
   s32fp gaugegain = Param::Get(Param::gaugegain);
   int soctest = Param::GetInt(Param::soctest);
   int soc = (soctest > 0 ? soctest : Param::GetInt(Param::soc)) - 50;
   int dc1 = FP_TOINT(gaugegain * soc) + dcoffset;
   int dc2 = FP_TOINT(-gaugegain * soc) + dcoffset;
   s32fp uaux = FP_FROMINT(AnaIn::uaux.Get()) / 291;

   Param::SetFlt(Param::uaux, uaux);

   timer_set_oc_value(GAUGE_TIMER, TIM_OC1, dc1);
   timer_set_oc_value(GAUGE_TIMER, TIM_OC2, dc2);



   if (Param::GetInt(Param::canperiod) == CAN_PERIOD_10MS)
      can1->SendAll();
}

static void MeasureCurrent()
{
   int idcmode = Param::GetInt(Param::idcmode);
   s32fp voltage = Param::Get(Param::udc);
   s32fp current = 0;

   if (rtc_get_counter_val() < 2) return; //Discard the first few current samples

   if (idcmode == IDC_DIFFERENTIAL || idcmode == IDC_SINGLE)
   {
      static int samples = 0;
      static u32fp amsIn = 0, amsOut = 0;
      static s32fp idcavg = 0;
      int curpos = AnaIn::curpos.Get();
      int curneg = AnaIn::curneg.Get();
      s32fp idcgain = Param::Get(Param::idcgain);
      int idcofs = Param::GetInt(Param::idcofs);
      int rawCurrent = idcmode == IDC_SINGLE ? curpos : curpos - curneg;

      current = FP_DIV(FP_FROMINT(rawCurrent - idcofs), idcgain);

      if (current < -FP_FROMFLT(0.8))
      {
         amsOut += -current;
         noCurrentMillis = 0;
      }
      else if (current > FP_FROMFLT(0.8))
      {
         amsIn += current;
         noCurrentMillis = 0;
      }
      else
      {
         noCurrentMillis++;
      }

      idcavg += current;
      samples++;

      if (samples == 1000)
      {
         s32fp chargein = Param::Get(Param::chargein);
         s32fp chargeout = Param::Get(Param::chargeout);

         chargein += amsIn / 1000;
         chargeout += amsOut / 1000;
         idcavg /= 1000;

         s32fp power = FP_MUL(voltage, idcavg);

         Param::SetFlt(Param::idcavg, idcavg);
         Param::SetFlt(Param::power, power);

         amsIn = 0;
         amsOut = 0;
         samples = 0;
         idcavg = 0;

         BmsCalculation::SetCharge(chargein, chargeout);
         Param::SetFlt(Param::chargein, chargein);
         Param::SetFlt(Param::chargeout, chargeout);
      }
   }
   else //Isa Shunt
   {
      current = FP_FROMINT(IsaShunt::GetCurrent()) / 1000;
      s32fp power = FP_MUL(voltage, current);
      Param::SetFlt(Param::power, power);
   }

   Param::SetFlt(Param::idc, current);
}

/** This function is called when the user changes a parameter */
extern void parm_Change(Param::PARAM_NUM paramNum)
{
   switch (paramNum)
   {
      case Param::canspeed:
         can1->SetBaudrate((enum Can::baudrates)Param::GetInt(Param::canspeed));
         break;
      case Param::idcmode:
         if (Param::GetInt(Param::idcmode) == IDC_ISACAN1)
         {
            IsaShunt::SetInterface(can1);
            IsaShunt::Initialize();
         }
         else if (Param::GetInt(Param::idcmode) == IDC_ISACAN2)
         {
            IsaShunt::SetInterface(can2);
            IsaShunt::Initialize();
         }
      default:
         break;
   }
}

void CanHandler(uint32_t id, uint32_t data[2])
{
   IsaShunt::HandleCanMessage(id, data);
}

extern "C" void tim4_isr(void)
{
   scheduler->Run();
}

extern "C" int main(void)
{
   extern const TERM_CMD TermCmds[];

   clock_setup();
   rtc_setup();
   usart_setup();
   tim_setup();
   nvic_setup();
   parm_load();
   ANA_IN_CONFIGURE(ANA_IN_LIST);
   DIG_IO_CONFIGURE(DIG_IO_LIST);
   AnaIn::Start();

   Can c1(CAN1, (Can::baudrates)Param::GetInt(Param::canspeed));
   Can c2(CAN2, (Can::baudrates)Param::GetInt(Param::canspeed));
   can1 = &c1;
   can2 = &c2;

   c1.SetReceiveCallback(CanHandler);
   c2.SetReceiveCallback(CanHandler);

   c1.RegisterUserMessage(IsaShunt::CAN_ID_REPLY);
   c1.RegisterUserMessage(IsaShunt::CAN_ID_CURRENT);
   c1.RegisterUserMessage(IsaShunt::CAN_ID_VOLTAGE);
   c1.RegisterUserMessage(IsaShunt::CAN_ID_POWER);
   c2.RegisterUserMessage(IsaShunt::CAN_ID_REPLY);
   c2.RegisterUserMessage(IsaShunt::CAN_ID_CURRENT);
   c2.RegisterUserMessage(IsaShunt::CAN_ID_VOLTAGE);
   c2.RegisterUserMessage(IsaShunt::CAN_ID_POWER);

   BmsCalculation::SetVoltageToSoCTable(lfpVtgToSoc);

   if (BMSState::LoadFromFlash())
   {
      Param::SetFlt(Param::socest, BMSState::GetEstimatedSoC());
   }

   Stm32Scheduler s(TIM4); //We never exit main so it's ok to put it on stack
   scheduler = &s;

   s.AddTask(MeasureCurrent, 1);
   s.AddTask(Ms10Task, 10);
   s.AddTask(CellModuleCommunication, 40);
   s.AddTask(Ms100Task, 100);

   parm_Change(Param::idcmode);
   Param::SetInt(Param::version, 4); //backward compatibility
   Terminal t(USART3, TermCmds);

   while(true)
      t.Run();

   return 0;
}


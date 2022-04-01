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

#define VER 0.29.B
#define VERSTR STRINGIFY(4=VER)


extern const char* errorListString;


/* Entries must be ordered as follows:
   1. Saveable parameters (id != 0)
   2. Temporary parameters (id = 0)
   3. Display values
 */
//Next param id (increase when adding new parameter!): 19
/*              category     name         unit       min     max     default id */
#define PARAM_LIST \
    PARAM_ENTRY(CAT_BMS,     cellmodop,   MODOPS,    0,      3,      0,      0   ) \
    PARAM_ENTRY(CAT_BMS,     shuntvtg,    "mV",      3000,   4200,   4200,   0   ) \
    PARAM_ENTRY(CAT_BMS,     chargestop,  "mV",      3000,   4200,   4200,   4   ) \
    PARAM_ENTRY(CAT_BMS,     chargestart, "mV",      3000,   4200,   3300,   6   ) \
    PARAM_ENTRY(CAT_BMS,     loadstop,    "mV",      2500,   4200,   2600,   14  ) \
    PARAM_ENTRY(CAT_BMS,     loadstart,   "mV",      2500,   4200,   3300,   15  ) \
    PARAM_ENTRY(CAT_BMS,     capacity,    "Ah",      1,      2000,   100,    8   ) \
    PARAM_ENTRY(CAT_CUR,     idcgain,     "dig/A",   -1000,  1000,   10,     3   ) \
    PARAM_ENTRY(CAT_CUR,     idcofs,      "dig",    -4095,   4095,   0,      5   ) \
    PARAM_ENTRY(CAT_CUR,     idcmode,     IDCMODES,  0,      3,      0,      7   ) \
    PARAM_ENTRY(CAT_IO,      relaymode,   RELAYMODS, 0,      1,      0,      9   ) \
    PARAM_ENTRY(CAT_IO,      relaythresh, "A",       -4000,  4000,   0,      10  ) \
    PARAM_ENTRY(CAT_IO,      relayhyst,   "A",       -4000,  4000,   0,      11  ) \
    PARAM_ENTRY(CAT_CHARGER, chgmaxvtg,   "V",       0,      1000,   0,      12  ) \
    PARAM_ENTRY(CAT_CHARGER, chgmaxcur,   "A",       0,      1000,   0,      13  ) \
    PARAM_ENTRY(CAT_CHARGER, dismaxcur,   "A",       0,      1000,   0,      17  ) \
    PARAM_ENTRY(CAT_GAUGE,   gaugeoffset, "dig",     0,      4096,   1000,   1   ) \
    PARAM_ENTRY(CAT_GAUGE,   gaugegain,   "dig/%",   0,      4096,   5,      2   ) \
    PARAM_ENTRY(CAT_COMM,    canspeed,    CANSPEEDS, 0,      4,      2,      83  ) \
    PARAM_ENTRY(CAT_COMM,    canperiod,   CANPERIODS,0,      1,      0,      88  ) \
    PARAM_ENTRY(CAT_COMM,    modcount,    "",        0,      63,     0,      16  ) \
    PARAM_ENTRY(CAT_TEST,    soctest,     "%",       0,      100,    0,      0   ) \
    PARAM_ENTRY(CAT_TEST,    relaytest,   ONOFF,     0,      2,      2,      0   ) \
    PARAM_ENTRY(CAT_TEST,    testcmd,     TESTS,     0,      5,      0,      0   ) \
    VALUE_ENTRY(opmode,      OPMODES, 2000 ) \
    VALUE_ENTRY(commquality, "%",     2029 ) \
    VALUE_ENTRY(curmodule,   "",      2025 ) \
    VALUE_ENTRY(soc,         "%",     2001 ) \
    VALUE_ENTRY(socest,      "%",     2019 ) \
    VALUE_ENTRY(idc,         "A",     2002 ) \
    VALUE_ENTRY(idcavg,      "A",     2022 ) \
    VALUE_ENTRY(udc,         "V",     2003 ) \
    VALUE_ENTRY(udc2,        "V",     2004 ) \
    VALUE_ENTRY(power,       "W",     2005 ) \
    VALUE_ENTRY(chargelim,   "A",     2028 ) \
    VALUE_ENTRY(dislim,      "A",     2030 ) \
    VALUE_ENTRY(chargein,    "As",    2006 ) \
    VALUE_ENTRY(chargeout,   "As",    2007 ) \
    VALUE_ENTRY(batmin,      "mV",    2008 ) \
    VALUE_ENTRY(batmax,      "mV",    2009 ) \
    VALUE_ENTRY(batavg,      "mV",    2010 ) \
    VALUE_ENTRY(tmpavg,      "°C",    2023 ) \
    VALUE_ENTRY(batmin2,     "mV",    2011 ) \
    VALUE_ENTRY(batmax2,     "mV",    2012 ) \
    VALUE_ENTRY(batavg2,     "mV",    2013 ) \
    VALUE_ENTRY(ignition,    ONOFF,   2019 ) \
    VALUE_ENTRY(relay,       ONOFF,   2024 ) \
    VALUE_ENTRY(ttostandby,  "s",     2020 ) \
    VALUE_ENTRY(trun,        "s",     2021 ) \
    VALUE_ENTRY(uaux,        "V",     2014 ) \
    VALUE_ENTRY(version,     VERSTR,  2015 ) \
    VALUE_ENTRY(cpuload,     "%",     2017 ) \

//Next value Id: 2031

#define CAT_TEST     "Testing"
#define CAT_GAUGE    "Fuel Gauge"
#define CAT_COMM     "Communication"
#define CAT_BMS      "Bms"
#define CAT_CUR      "Current Sensing"
#define CAT_IO       "IO settings"
#define CAT_CHARGER  "Charger and Load Control"
#define CANSPEEDS    "0=125k, 1=250k, 2=500k, 3=800k, 4=1M"
#define CANPERIODS   "0=100ms, 1=10ms"
#define OPMODES      "0=Start, 1=ResetAddr, 2=SetAddr, 3=WaitAddr, 4=WaitRdy, 5=GetVersion, 6=Run, 7=Standby, 8=SetShunt, 9=SWUpgrade, 10=TestExpired"
#define MODOPS       "0=none, 1=AssignAddress, 2=StopAcq, 3=FWUpgrade"
#define IDCMODES     "0=AdcSingle, 1=AdcDifferential, 2=IsaCan1, 3=IsaCan2"
#define ONOFF        "0=Off, 1=On, 2=na"
#define TESTS        "0=AllOn, 1=WifiOff, 2=CursensOff, 3=AllOff, 4=BoardOff, 5=EstSoC"
#define RELAYMODS    "0=CellVtg, 1=CurThresh"

enum
{
   IDC_SINGLE, IDC_DIFFERENTIAL, IDC_ISACAN1, IDC_ISACAN2
};

enum
{
   CAN_PERIOD_100MS, CAN_PERIOD_10MS
};

enum SlaveOps
{
   None, AssignAddress, StopAcq, FWUpgrade
};

enum Power
{
   AllOn, WifiOff, CursensOff, AllOff, BoardOff, EstSoC
};

enum States
{
   Start, ResetAddress, SetAddress, WaitAddress, WaitReady, GetVersion, Run, Standby, Shunt, SWUpgrade
};

enum RelayModes
{
   CellVoltage, CurThresh
};

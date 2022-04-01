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
#ifndef BMSSTATE_H
#define BMSSTATE_H
#include <stdint.h>
#include "my_fp.h"

class BMSState
{
   public:
      /** Access estimatedSoC
       * \return The current value of estimatedSoC
       */
      static s32fp GetEstimatedSoC() { return estimatedSoC; }
      /** Set estimatedSoC
       * \param val New value to set
       */
      static void SetEstimatedSoC(s32fp val) { estimatedSoC = val; }
      /** Access chargein
       * \return The current value of chargein
       */
      static s32fp GetChargein() { return chargein; }
      /** Set chargein
       * \param val New value to set
       */
      static void SetChargein(s32fp val) { chargein = val; }
      /** Access chargeout
       * \return The current value of chargeout
       */
      static s32fp GetChargeout() { return chargeout; }
      /** Set chargeout
       * \param val New value to set
       */
      static void SetChargeout(s32fp val) { chargeout = val; }

      static void SaveToFlash();
      static bool LoadFromFlash();

   protected:

   private:
      static bool saved; //!< For every startup battery state can only be saved once to prevent flash death
      static s32fp estimatedSoC; //!< Member variable "estimatedSoC"
      static s32fp chargein; //!< Member variable "chargein"
      static s32fp chargeout; //!< Member variable "chargeout"
};

#endif // BMSSTATE_H

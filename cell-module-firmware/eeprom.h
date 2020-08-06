#ifndef EEPROM_H_INCLUDED
#define EEPROM_H_INCLUDED

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

#include <avr/eeprom.h>

extern EEMEM uint8_t osccal_eep;
extern EEMEM uint16_t single_ended_gains_eep[];
extern EEMEM uint16_t differential_gains_eep[];
#ifndef LEGACY
extern EEMEM uint16_t differential_gain_corr_eep;
#endif // LEGACY
extern EEMEM uint16_t temperature_offset_eep;
extern EEMEM uint32_t serial_number;

#endif // EEPROM_H_INCLUDED

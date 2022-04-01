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
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/usart.h>

#include "onewire.h"
#include "hwdefs.h"
#include "params.h"
#include "digio.h"

uint8_t OneWire::buffer[];

void OneWire::StartReceiveMode()
{
   usart_set_mode(USART1, USART_MODE_TX_RX);
   dma_disable_channel(DMA1, BMS_USART_DMARX);
   dma_disable_channel(DMA1, BMS_USART_DMATX);
   dma_set_memory_address(DMA1, BMS_USART_DMARX, (uint32_t)buffer);
   dma_set_number_of_data(DMA1, BMS_USART_DMARX, sizeof(buffer));
   dma_enable_channel(DMA1, BMS_USART_DMARX);
   gpio_toggle(GPIOB, GPIO9);
}

int OneWire::GetReceivedData(uint8_t* data, int numBytes)
{
   uint16_t availableBytes = sizeof(buffer) - dma_get_number_of_data(DMA1, BMS_USART_DMARX);
   int res = numBytes < availableBytes ? numBytes : availableBytes;

   //Always copy the last received bytes. Especially in set address commands
   //the buffer is polluted with break frames
   while (numBytes > 0 && availableBytes > 0)
   {
      numBytes--;
      availableBytes--;
      data[numBytes] = buffer[availableBytes];
   }

   return res;
}

void OneWire::SendData(const uint8_t* data, int numBytes)
{
   usart_set_mode(USART1, USART_MODE_TX);

   dma_disable_channel(DMA1, BMS_USART_DMARX);
   dma_disable_channel(DMA1, BMS_USART_DMATX);
   dma_set_number_of_data(DMA1, BMS_USART_DMATX, numBytes);
   dma_set_memory_address(DMA1, BMS_USART_DMATX, (uint32_t)buffer);
   dma_clear_interrupt_flags(DMA1, BMS_USART_DMATX, DMA_TCIF);

   while (numBytes > 0)
   {
      buffer[numBytes - 1] = data[numBytes - 1];
      numBytes--;
   }

   USART1_CR1 |= USART_CR1_SBK;
   dma_enable_channel(DMA1, BMS_USART_DMATX);
}

bool OneWire::IsReceiving()
{
   return (USART_CR1(BMS_USART) & USART_CR1_RE) != 0;
}

extern "C" void usart1_isr()
{
   USART1_SR &= ~USART_SR_TC;
   OneWire::StartReceiveMode();
}


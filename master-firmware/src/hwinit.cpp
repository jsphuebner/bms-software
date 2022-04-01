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
#include <libopencm3/cm3/common.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/rtc.h>
#include "hwdefs.h"
#include "hwinit.h"

/**
* Start clocks of all needed peripherals
*/
void clock_setup(void)
{
   RCC_CLOCK_SETUP();

   //The reset value for PRIGROUP (=0) is not actually a defined
   //value. Explicitly set 16 preemtion priorities
   SCB_AIRCR = SCB_AIRCR_VECTKEY | SCB_AIRCR_PRIGROUP_GROUP16_NOSUB;

   rcc_periph_clock_enable(RCC_GPIOA);
   rcc_periph_clock_enable(RCC_GPIOB);
   rcc_periph_clock_enable(RCC_GPIOC);
   rcc_periph_clock_enable(RCC_GPIOD);
   rcc_periph_clock_enable(RCC_USART1);
   rcc_periph_clock_enable(RCC_USART3);
   rcc_periph_clock_enable(RCC_TIM3); //Gauge
   rcc_periph_clock_enable(RCC_TIM4); //Scheduler
   rcc_periph_clock_enable(RCC_DMA1);  //ADC and UART
   rcc_periph_clock_enable(RCC_ADC1);
   rcc_periph_clock_enable(RCC_CRC);
   rcc_periph_clock_enable(RCC_AFIO); //CAN
   rcc_periph_clock_enable(RCC_CAN1); //CAN
   rcc_periph_clock_enable(RCC_CAN2); //CAN
}

void default_bms_uart()
{
   //Normal UART pins
   AFIO_MAPR &= ~AFIO_MAPR_USART1_REMAP;
   //gpio_primary_remap(0, 0);
   //Normal TX pin as UART output
   gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_USART1_TX);
   //Remapped TX pin as input to not disturb operation
   gpio_set_mode(GPIOB, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO6);
   //Remapped RX pin hard pull-up (Opto collector)
   gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO7);
   gpio_set(GPIOB, GPIO7);
   //Normal RX pin as input with pull-down
   gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO_USART1_RX);
   gpio_clear(GPIOA, GPIO_USART1_RX);
}

void remap_bms_uart()
{
   // REMAP UART1 to PB7 (RX) and PB6 (TX)
   gpio_primary_remap(0, AFIO_MAPR_USART1_REMAP | AFIO_MAPR_TIM2_REMAP_PARTIAL_REMAP1);
   //Normal TX pin as input to not disturb operation
   gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO_USART1_TX);
   //Remapped TX pin as UART output
   gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO6);
   //Remapped RX pin as input with pull-up
   gpio_set_mode(GPIOB, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO7);
   gpio_set(GPIOB, GPIO7);
   //Normal RX pin hard pull-down (Opto emitter)
   gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO_USART1_RX);
   gpio_clear(GPIOA, GPIO_USART1_RX);
}

/**
* Setup UART3 115200 8N1
*/
void usart_setup(void)
{
   gpio_set_mode(TERM_USART_TXPORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, TERM_USART_TXPIN);

   default_bms_uart();
   usart_set_baudrate(BMS_USART, 10000);
   usart_set_databits(BMS_USART, 8);
   usart_set_stopbits(BMS_USART, USART_STOPBITS_2);
   usart_set_mode(BMS_USART, USART_MODE_TX);
   usart_set_parity(BMS_USART, USART_PARITY_NONE);
   usart_set_flow_control(BMS_USART, USART_FLOWCONTROL_NONE);
   USART_CR1(BMS_USART) |= USART_CR1_TCIE; //on transmission complete we enable receiver

   usart_enable_rx_dma(BMS_USART);
   usart_enable_tx_dma(BMS_USART);

   dma_channel_reset(DMA1, BMS_USART_DMARX);
   dma_set_peripheral_address(DMA1, BMS_USART_DMARX, (uint32_t)&BMS_USART_DR);
   dma_set_peripheral_size(DMA1, BMS_USART_DMARX, DMA_CCR_PSIZE_8BIT);
   dma_set_memory_size(DMA1, BMS_USART_DMARX, DMA_CCR_MSIZE_8BIT);
   dma_enable_memory_increment_mode(DMA1, BMS_USART_DMARX);

   dma_channel_reset(DMA1, BMS_USART_DMATX);
   dma_set_read_from_memory(DMA1, BMS_USART_DMATX);
   dma_set_peripheral_address(DMA1, BMS_USART_DMATX, (uint32_t)&BMS_USART_DR);
   dma_set_peripheral_size(DMA1, BMS_USART_DMATX, DMA_CCR_PSIZE_8BIT);
   dma_set_memory_size(DMA1, BMS_USART_DMATX, DMA_CCR_MSIZE_8BIT);
   dma_enable_memory_increment_mode(DMA1, BMS_USART_DMATX);

   usart_enable(BMS_USART);

   /// Terminal
   usart_set_baudrate(TERM_USART, USART_BAUDRATE);
   usart_set_databits(TERM_USART, 8);
   usart_set_stopbits(TERM_USART, USART_STOPBITS_1);
   usart_set_mode(TERM_USART, USART_MODE_TX_RX);
   usart_set_parity(TERM_USART, USART_PARITY_NONE);
   usart_set_flow_control(TERM_USART, USART_FLOWCONTROL_NONE);
   usart_enable_rx_dma(TERM_USART);
   usart_enable_tx_dma(TERM_USART);

   dma_channel_reset(DMA1, TERM_USART_DMATX);
   dma_set_read_from_memory(DMA1, TERM_USART_DMATX);
   dma_set_peripheral_address(DMA1, TERM_USART_DMATX, (uint32_t)&TERM_USART_DR);
   dma_set_peripheral_size(DMA1, TERM_USART_DMATX, DMA_CCR_PSIZE_8BIT);
   dma_set_memory_size(DMA1, TERM_USART_DMATX, DMA_CCR_MSIZE_8BIT);
   dma_enable_memory_increment_mode(DMA1, TERM_USART_DMATX);

   dma_channel_reset(DMA1, TERM_USART_DMARX);
   dma_set_peripheral_address(DMA1, TERM_USART_DMARX, (uint32_t)&TERM_USART_DR);
   dma_set_peripheral_size(DMA1, TERM_USART_DMARX, DMA_CCR_PSIZE_8BIT);
   dma_set_memory_size(DMA1, TERM_USART_DMARX, DMA_CCR_MSIZE_8BIT);
   dma_enable_memory_increment_mode(DMA1, TERM_USART_DMARX);

   usart_enable(TERM_USART);
}

/**
* Enable various interrupts
*/
void nvic_setup(void)
{
   nvic_enable_irq(NVIC_TIM1_CC_IRQ); //One Wire Comm
   nvic_set_priority(NVIC_TIM4_IRQ, 0x1 << 4); //highest priority

   //nvic_enable_irq(NVIC_DMA1_CHANNEL5_IRQ);
	//nvic_set_priority(NVIC_DMA1_CHANNEL5_IRQ, 0xd << 4); //third lowest priority
	nvic_enable_irq(NVIC_USART1_IRQ);

   nvic_enable_irq(NVIC_TIM4_IRQ); //Scheduler
   nvic_set_priority(NVIC_TIM4_IRQ, 0xe << 4); //second lowest priority
}

void rtc_setup()
{
   //Base clock is HSE/128 = 8MHz/128 = 62.5kHz
   //62.5kHz / (62500 + 1) = 100Hz
   rtc_auto_awake(RCC_HSE, 62501); //1000ms tick
   rtc_set_prescale_val(62501);
   rtc_set_counter_val(0);
}

/**
* Setup main PWM timer and timer for generating over current
* reference values and external PWM
*/
void tim_setup()
{
   /*** Setup over/undercurrent and PWM output timer */
   timer_disable_counter(GAUGE_TIMER);
   //edge aligned PWM
   timer_set_alignment(GAUGE_TIMER, TIM_CR1_CMS_EDGE);
   timer_enable_preload(GAUGE_TIMER);
   /* PWM mode 1 and preload enable */
   timer_set_oc_mode(GAUGE_TIMER, TIM_OC1, TIM_OCM_PWM1);
   timer_set_oc_mode(GAUGE_TIMER, TIM_OC2, TIM_OCM_PWM1);
   timer_enable_oc_preload(GAUGE_TIMER, TIM_OC1);
   timer_enable_oc_preload(GAUGE_TIMER, TIM_OC2);

   timer_set_oc_polarity_high(GAUGE_TIMER, TIM_OC1);
   timer_set_oc_polarity_high(GAUGE_TIMER, TIM_OC2);
   timer_enable_oc_output(GAUGE_TIMER, TIM_OC1);
   timer_enable_oc_output(GAUGE_TIMER, TIM_OC2);
   timer_generate_event(GAUGE_TIMER, TIM_EGR_UG);
   timer_set_prescaler(GAUGE_TIMER, 0);
   /* PWM frequency */
   timer_set_period(GAUGE_TIMER, 4096);
   timer_enable_counter(GAUGE_TIMER);

   /** setup gpio */

   //gpio_primary_remap(AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_ON, AFIO_MAPR_TIM2_REMAP_PARTIAL_REMAP1);
   gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO6 | GPIO7);
}


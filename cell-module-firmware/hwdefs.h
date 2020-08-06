#ifndef HWDEFS_H_INCLUDED
#define HWDEFS_H_INCLUDED

#include <avr/io.h>
#include <avr/interrupt.h>

#define USART_BAUD 10000

#define TX_START() {\
if (RX_INPUT_STATE) \
   TXRX_PORT &= ~TXRX_PIN; \
else \
   TXRX_PORT |= TXRX_PIN; \
TXRX_DDR |= TXRX_PIN; }

#define TX_STOP()                   TXRX_DDR &= ~TXRX_PIN; TXRX_PORT &= ~TXRX_PIN
#define RX_INPUT_STATE              ((RX_INPUT & TXRX_PIN) > 0)

#define DISABLE_PROPAGATION()       INHIBIT_DDR |= INHIBIT_PIN
#define ENABLE_PROPAGATION()        INHIBIT_DDR &= ~INHIBIT_PIN

#define SHUNT_CONFIGURE()           DDRB |= (1 << PIN2) | (1 << PIN1) | (1 << PIN0); DDRA |= 1 << PIN6;
#define SHUNT_SET(a) { \
if (a & 0x8) \
   PORTA |= 1 << PIN6; \
else \
   PORTA &= ~(1 << PIN6); \
PORTB &= ~((1 << PIN2) | (1 << PIN1) | (1 << PIN0)); \
PORTB |= a & ((1 << PIN2) | (1 << PIN1) | (1 << PIN0)); }

#define TXRX_DDR                    DDRA
#define TXRX_PORT                   PORTA
#define RX_INPUT                    PINA
#define TXRX_PIN                    (1 << PIN7)
#define TX_HIGH()                   TXRX_PORT |= TXRX_PIN
#define TX_LOW()                    TXRX_PORT &= ~TXRX_PIN
#define SAMPLE()                    (inverted) ? (PINA & TXRX_PIN) == 0 : (PINA & TXRX_PIN) != 0

#define INHIBIT_DDR                 DDRA
#define INHIBIT_PORT                PORTA
#define INHIBIT_PIN                 (1 << PIN5)

#define DISABLE_ADC_INPUT_BUFFERS() DIDR0 = 0x1f

#define WAKEUP_ISR                  ISR(PCINT0_vect)

#define SETUP_RX_PINCHANGE_IRQ()    GIMSK |= 1 << PCIE0
#define ENABLE_RX_PINCHANGE_IRQ()   PCMSK0 |= 1 << PCINT7
#define DISABLE_RX_PINCHANGE_IRQ()  PCMSK0 &= ~(1 << PCINT7)

#define ENABLE_CLOCK_IRQ()          TIFR0 = 1 << OCF0A;    \
                                    TIMSK0 |= 1 << OCIE0A
#define DISABLE_CLOCK_IRQ()         TIMSK0 &= ~(1 << OCIE0A)


//At 4Mhz one counter period takes 512µs
#define SETUP_BIT_TIMER()           TCCR0A = (1 << WGM01);   \
                                    TCCR0B = (1 << CS01)

#define RECV_TIMER_CAPT_ISR         ISR(TIM0_COMPA_vect)


#define ENABLE_ADC()      ADCSRA |= (1 << ADEN);
#define DISABLE_ADC()     ADCSRA &= ~(1 << ADEN);
#define CHAN_TEMP         0x22
#define CHAN_REF          0x21
#define CHAN_GND          0x20


#endif // HWDEFS_H_INCLUDED

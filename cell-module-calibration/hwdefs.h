#ifndef HWDEFS_H_INCLUDED
#define HWDEFS_H_INCLUDED

#include <avr/io.h>
#include <avr/interrupt.h>

#define BIT_ZERO_T 100
#define BIT_ONE_T (2 * BIT_ZERO_T)
#define BIT_ZERO_MIN  (uint16_t)(0.5 * BIT_ZERO_T * (F_CPU / 1000000))
#define BIT_ZERO_MAX  (uint16_t)(1.5 * BIT_ZERO_T * (F_CPU / 1000000))
#define BIT_ONE_MAX (uint16_t)(2 * BIT_ZERO_MAX)
#define BIT_ONE_MIN BIT_ZERO_MAX

#define TX_START() {\
if (RX_INPUT_STATE) \
   TX_PORT &= ~TX_PIN; \
else \
   TX_PORT |= TX_PIN; \
TX_DDR |= TX_PIN; }

#define TX_STOP()                   TX_DDR &= ~TX_PIN; TX_PORT &= ~TX_PIN
#define TX_TOGGLE()                 TX_PORT ^= TX_PIN
#define RX_INPUT_STATE              ((RX_INPUT & TX_PIN) > 0)
#define DUTYCYCLE_COR               (((F_CPU / 8) * 0) / 1000000)

#define CAPT_FLAG                   (TIFR1 & (1 << ICF1))
#define RESET_CAPT_FLAG()           TIFR1 |= (1 << ICF1)
#define SET_COUNTER(x)              TCNT1 = x
#define GET_COUNTER()               TCNT1
#define TOGGLE_EDGE_POLARITY()      TCCR1B ^= (1 << ICES1)
#define SET_START_EDGE_POLARITY() { \
\
if (RX_INPUT_STATE) \
   TCCR1B &= ~(1 << ICES1); /* falling edge */ \
else \
   TCCR1B |= (1 << ICES1); } //rising edge

#define CAPT_VALUE                  ICR1
#define SET_IR_FRQ()
#define SET_IR_DUTYCYCLE(d)

#define CONFIG_TIMER_CAPT_MODE()    TCCR1A = 0;   \
                                    TCCR1B = (1 << ICNC1) | (1 << CS10)

#define CONFIG_TIMER_PWM_MODE()

#define RESET_BIT_COUNTER()         TCNT0 = 0
#define GET_BIT_COUNTER()           TCNT0

#define BIT_TIMER_ZERO              (((F_CPU / 8) * BIT_ZERO_T) / 1000000)
#define BIT_TIMER_ONE               (((F_CPU / 8) * BIT_ONE_T) / 1000000)

#define DISABLE_PROPAGATION()       INHIBIT_DDR |= INHIBIT_PIN
#define ENABLE_PROPAGATION()        INHIBIT_DDR &= ~INHIBIT_PIN

//#define SHUNT_DDR                   DDRB
//#define SHUNT_PORT                  PORTB
//#define SHUNT_PINS                  (1 << PIN2) | (1 << PIN1) | (1 << PIN0)
#define SHUNT_CONFIGURE()           DDRB |= (1 << PIN2) | (1 << PIN1) | (1 << PIN0); DDRA |= 1 << PIN6;
#define SHUNT_SET(a) { \
if (a & 0x8) \
   PORTA |= 1 << PIN6; \
else \
   PORTA &= ~(1 << PIN6); \
PORTB &= ~((1 << PIN2) | (1 << PIN1) | (1 << PIN0)); \
PORTB |= a & ((1 << PIN2) | (1 << PIN1) | (1 << PIN0)); }

#define TX_DDR                      DDRA
#define TX_PORT                     PORTA
#define RX_INPUT                    PINA
#define TX_PIN                      (1 << PIN7)

#define INHIBIT_DDR                 DDRA
#define INHIBIT_PORT                PORTA
#define INHIBIT_PIN                 (1 << PIN5)

#define RECV_EN_DDR                 DDRA
#define RECV_EN_PORT                PORTA
#define RECV_EN_PIN                 (1 << PIN7)

#define RESETCAL_PORT               PORTA
#define RESETCAL_PIN                (1 << PIN5)
#define RESETCAL_REQ()              ((PINA & RESETCAL_PIN) == 0)

#define RESETADR_PORT               PORTA
#define RESETADR_PIN                (1 << PIN4)
#define RESETADR_REQ()              ((PINA & RESETADR_PIN) == 0)

#define DISABLE_ADC_INPUT_BUFFERS() DIDR0 = 0x1f

#define WAKEUP_ISR                  ISR(PCINT0_vect)

#define SETUP_WAKEUP_IRQ()          GIMSK |= 1 << PCIE0
#define ENABLE_WAKEUP_IRQ()         PCMSK0 |= 1 << PCINT7
#define DISABLE_WAKEUP_IRQ()        PCMSK0 &= ~(1 << PCINT7)

#define SET_BIT_TIME(t)             OCR0A = t

#define ENABLE_SEND_IRQ()           TIFR0 = 1 << OCF0A;    \
                                    TIMSK0 |= 1 << OCIE0A
#define DISABLE_SEND_IRQ()          TIMSK0 &= ~(1 << OCIE0A)
#define IS_SENDER_BUSY()            ((TIMSK0 & (1 << OCIE0A)) > 0)

#define DISABLE_OVF_CAPT_IRQ()      TIMSK1 = 0
#define ENABLE_OVF_CAPT_IRQ()       TIFR1 = (1 << ICF1) | (1 << TOV1);    \
                                    TIMSK1 |= (1 << ICIE1) | (1 << TOIE1)

//At 4Mhz one counter period takes 512µs
#define SETUP_BIT_TIMER()           TCCR0A = (1 << WGM01);   \
                                    TCCR0B = (1 << CS01)

#define RECV_TIMER_CAPT_ISR         ISR(TIM1_CAPT_vect)
#define RECV_TIMER_OVF_ISR          ISR(TIM1_OVF_vect)
#define BIT_TIMER_COMP_ISR          ISR(TIM0_COMPA_vect)
#define ADC_ISR                     ISR(ADC_vect)

#define SET_ADC_CHAN(c)  (ADMUX = (ADMUX & (uint8_t)~0x2f) | c)
#define GET_ADC_CHAN(c)  (ADMUX & 0x2f)
#define USE_INT_VREF()   (ADMUX = (ADMUX & (uint8_t)~0xC0) | (1 << REFS1))
#define USE_EXT_VREF()   (ADMUX = (ADMUX & (uint8_t)~0xC0) | (1 << REFS0))
#define USE_VCC_VREF()   (ADMUX = (ADMUX & (uint8_t)~0xC0))
#define ENABLE_ADC()      ADCSRA |= (1 << ADEN);
#define DISABLE_ADC()     ADCSRA &= ~(1 << ADEN);
#define CHAN_TEMP         0x22
#define CHAN_REF          0x21
#define CHAN_GND          0x20


#endif // HWDEFS_H_INCLUDED

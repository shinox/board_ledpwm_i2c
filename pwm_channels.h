#pragma once
#include <ctype.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
//
#include "conf.h"
// misc 
#define BRIGHT 0
#define DIMM   1
#define WHITE  0
#define BLUE   1
//
#define PWMDEFAULT 0x00  // 0x80  // default PWM value at start up for all channels, 128
//
#define CH0_CLEAR  (PORTB &= ~(1 << PB3)) // map CH0 to PB3
#define CH1_CLEAR  (PORTC &= ~(1 << PC2)) // map CH1 to PC2
#define CH2_CLEAR  (PORTC &= ~(1 << PC3)) // map CH2 to PC3
#define CH3_CLEAR  (PORTC &= ~(1 << PC4)) // map CH3 to PC4
#define CH4_CLEAR  (PORTC &= ~(1 << PC5)) // map CH4 to PC5

#define CH5_CLEAR  (PORTD &= ~(1 << PD7)) // map CH5 to PD7
#define CH6_CLEAR  (PORTC &= ~(1 << PC6)) // map CH6 to PC6
#define CH7_CLEAR  (PORTC &= ~(1 << PC7)) // map CH7 to PC7
#define CH8_CLEAR  (PORTB &= ~(1 << PB0)) // map CH8 to PB0
#define CH9_CLEAR  (PORTB &= ~(1 << PB1)) // map CH9 to PB1
#define CH10_CLEAR (PORTD &= ~(1 << PD4)) // map CH10 to PD4 // OC1B -- FAN PWM control
#define CH11_CLEAR (PORTD &= ~(1 << PD5)) // map CH11 to PD5 // OC1A -- FAN PWM control
//
//! Set bits corresponding to pin usage above
#define PORTB_MASK  (1 << PB0)|(1 << PB1)|(1 << PB3)
#define PORTC_MASK  (1 << PC2)|(1 << PC3)|(1 << PC4)|(1 << PC5)|(1 << PC6)|(1 << PC7)
#define PORTD_MASK  /*(1 << PD4)|(1 << PD5)|*/(1 << PD7)
//
volatile unsigned char softcount = 0xFF;
volatile unsigned int count = 0, i =0;

//! global buffers
unsigned char compare[CHMAX] = {0};
volatile unsigned char compbuff[CHMAX] = {0};

// Which leds to use, might be useful later on in case weather liht adaptations
uint8_t light[USELED] = { TRUE
			, TRUE
			, TRUE
			, TRUE
			, TRUE
			, TRUE
			, TRUE
			, TRUE
			, TRUE
			, TRUE	
			}; // Make sure You know what USELED is at the time of adding another value

// 
uint8_t brightness_threshold = BRIGHTNESS_INCREMENT;
uint8_t top_brightness[CHMAX] = { TOP_BR
				, TOP_BR
 				, TOP_BR
				, TOP_BR
				, TOP_BR
				, TOP_BR
				, TOP_BR
				, TOP_BR
				, TOP_BR
				, TOP_BR
//				,/*FAN_A*/TOP_BR // PWM too slow for one of the fans, moved to OCR1A and B - PD4 and 5
//				,/*FAN_B*/TOP_BR //
				}; // Contain startup values for FAN - last two 
uint8_t brightness[USELED] =  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
struct tm* time = NULL;
uint8_t MORNING = MOR, EVENING = EVE;
uint8_t set_hour = 12, set_minute = 30, set_sec = 0;
long time_counter = 0;
#if DEBUG
long time_delay = 5; // used in pwm_channels.c dimmer()
#else
long time_delay = 112; // ((HALF_DELAY_PERIOD * 60)/(LOOP_DELAY * TOP)); // for 2s loops half brightness reached - depends from choice of TOP
#endif
uint8_t fan_a_duty = FAN_A_DUTY, fan_b_duty = FAN_B_DUTY; // All in conf.h

// Prototypes
void ledpwm_timer0_init();
void set_diode_pwm(uint8_t channel, uint8_t pwm_count);
void set_fan_pwm(uint8_t fan_a_duty, uint8_t fan_b_duty);
void dimmer(uint8_t incr_brightness);



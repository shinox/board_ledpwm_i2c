#include "pwm_channels.h"
//
ISR (TIMER0_OVF_vect)
{
	// softcount globally defind in pwm_channels.h  
	if (++softcount >= TOP) {   // Was: increment modulo 256 counter and update
		//      the compare values only when counter = 0.
		// Now WHat Is: Needed define top due to software frequency limitation ~122Hz 
		// for TOP =255 at Fast PWM, Redefined to Normal, 
		// Set OCR0 = TOP set value and clear on Compare match ISR(TIMER_COMP_vect)
		// If grater resolution for brightness required USE TIMER1 !!!
		compare[0]  = compbuff[0]; 
		compare[1]  = compbuff[1];
		compare[2]  = compbuff[2];
		compare[3]  = compbuff[3];
		compare[4]  = compbuff[4];
		compare[5]  = compbuff[5];
		compare[6]  = compbuff[6];
		compare[7]  = compbuff[7];
		compare[8]  = compbuff[8];
		compare[9]  = compbuff[9];   
//		compare[10] = compbuff[10];   
//		compare[11] = compbuff[11];   // last element should equal CHMAX - 1

		PORTB = PORTB_MASK;            // update outputs
		PORTC = PORTC_MASK;            // update outputs
		PORTD = PORTD_MASK;            // update outputs
		
		softcount = 0;
	} 
	// clear port pin on compare match (executed on next interrupt)
	if(compare[0]  == softcount) CH0_CLEAR;
	if(compare[1]  == softcount) CH1_CLEAR;
	if(compare[2]  == softcount) CH2_CLEAR;
	if(compare[3]  == softcount) CH3_CLEAR;
	if(compare[4]  == softcount) CH4_CLEAR;

	if(compare[5]  == softcount) CH5_CLEAR;
	if(compare[6]  == softcount) CH6_CLEAR;
	if(compare[7]  == softcount) CH7_CLEAR;
	if(compare[8]  == softcount) CH8_CLEAR;
	if(compare[9]  == softcount) CH9_CLEAR;
//	if(compare[10] == softcount) CH10_CLEAR;
//	if(compare[11] == softcount) CH11_CLEAR;  
}
//
/*
ISR (TIMER0_COMP_vect) 
{
	TCNT0 = 0; // Reset Timer0
}
*/
//
void fan_timer1_init()  /* WORK IN PROGRESS */
{
	DDRD = 0x30;                      // Set Port D4 and d5 as Output
    
    	// 
   	TCCR1A  = (1<<WGM10);
   	TCCR1B  = (1<<WGM12);   
   	TCCR1B |= (1<<CS10) | (1<<CS12); // Preskaler 1024

	// clear OC1A, OC1B on compare
   	TCCR1A |= (1<<COM1A1) | (1<<COM1B1);                                  		
	
	sei();
}
//
void timer2_init()
{
	TCCR2 = 0x00; // Stop the timer
	TCNT2 = 0x00; // 8-bit counter
	// set up timer with prescaler = 0 and Fast PWM mode
	TCCR2 |= /*(1 << WGM20)|(1 << WGM21)|*/(1 << CS20); // Normal, no preskaler
	// initialize counter
	TCNT2 = 0;
	TIMSK |= (1 << TOIE2);

	sei();
}
// Declarations
void ledpwm_timer0_init()
{
	TCCR0 = 0x00; // Stop the timer
	TCNT0 = 0x00; // 8-bit counter

	// set up timer with prescaler = 0 and Fast PWM mode
	//TCCR0 |= (1 << WGM00)  |(1 << WGM01) | (1 << CS00); // Fast PWM, No preskaler (CS00)
	TCCR0 |= (1 << CS00); // Fast PWM must not be used for set TOP=OCR0, Normal mode, No preskaler

	// initialize counter
	TCNT0 = 0;

	OCR0  = TOP; // defined in conf.h
	//OCR0  = 0xFF; // 100% PWM pulse width

	TIMSK |= (1 << TOIE0); // Overflow
	//TIMSK |= (1 << TOIE0)|(1 << OCIE0); // Overflow and CTC

	//TIFR |= (1 << OCF0)/*|(1 << TOV0)*/;

	// Need to change this to mask I got
	DDRB = PORTB_MASK;            // set port pins to output
	DDRC = PORTC_MASK;            // set port pins to output
	DDRD = PORTD_MASK;            // set port pins to output
	//
	uint8_t i, pwm;

	pwm = PWMDEFAULT;

	for(i=0 ; i<CHMAX ; i++)      // initialise all channels
	{
		compare[i] = pwm;           // set default PWM values
		compbuff[i] = pwm;          // set default PWM values
	}

	// enable global interrupts
	sei();
}

//
void set_diode_pwm(uint8_t channel, uint8_t pwm_count)
{
	if (channel < CHMAX)
	{
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
		{
			compbuff[channel] = pwm_count;
		}
	}
}
//
void set_fan_pwm(uint8_t fan_a_duty, uint8_t fan_b_duty)
{
/*
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		compbuff[FAN_A] = fan_a_duty; 
		compbuff[FAN_B] = fan_b_duty;
	}
	*/
	OCR1A = fan_a_duty;
	OCR1B = fan_b_duty;
}

// * dimmer
void dimmer(uint8_t incr_brightness)
{
	if (time_counter >= time_delay) {
		for (int channel = 0; channel < USELED; channel++) 
		{
			if ( incr_brightness == TRUE 
					&& light[channel] == TRUE 

			   ) { 
				if (brightness[channel] <= top_brightness[channel]) 
				{
					set_diode_pwm(channel, brightness[channel]);
					brightness[channel]+=brightness_threshold;
				} else {
					light[channel] = FALSE;
				}
			} 
			else 
				if ( incr_brightness == FALSE  
						&& light[channel] == TRUE
				   ) {
					if (brightness[channel] >= NIGHTMODE) 
					{
						set_diode_pwm(channel, brightness[channel]);
						brightness[channel]-=brightness_threshold;
					}
					else
					{
						if (channel < 4) { // NOT FLEXIBLE !!!!
							set_diode_pwm(channel, 0);
							light[channel] = FALSE;
						}
						else
							light[channel] = FALSE; // Test this approach !!!
					}
				}
		}
		time_counter = 0;
	}
	//    time_counter++; // Use this if counter NOT incremented by timer
}


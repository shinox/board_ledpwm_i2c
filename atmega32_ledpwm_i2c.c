/*
   simavr is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   simavr is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with simavr.  If not, see <http://www.gnu.org/licenses/>.
   */
//
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
//
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <util/delay.h>
#include <stdbool.h>

// for linker, emulator, and programmer's sake
#include "avr_mcu_section.h"
AVR_MCU(F_CPU, "atmega32");

//#include "../shared/avr_twi_master.h" // NOT USED
#include "../shared/twimaster.h"

#include "conf.h"
#include "rtc.h"
#include "uart.h"
//
// eample putchar UART usage, Works on FreeBSD better !!!
//
static int uart_putchar(char c, FILE *stream) {
	if (c == '\n')
		uart_putchar('\r', stream);
	loop_until_bit_is_set(UCSRA, UDRE);
	UDR = c;
	return 0;
}

//
// printf
static FILE mystdout = FDEV_SETUP_STREAM( // Use above if problems detected
		uart_putc
		, NULL
		, _FDEV_SETUP_WRITE);
//				
#if DEBUG
int test = 1;
#endif
//
extern volatile unsigned char compbuff[CHMAX];
//
void communicate(char command[], int val1, int val2);
void uartRecvString();
int convertTime(struct tm* ttime);
void getNTP(struct tm* time);
void updateLedArray();
void requestLedArray();
void saveTopBrightness();
//
int main()
{      
	//set_sleep_mode(SLEEP_MODE_PWR_SAVE);
	set_sleep_mode(SLEEP_MODE_IDLE);
	// printf
	stdout = &mystdout;

	fan_timer1_init();
	//
	ledpwm_timer0_init();
	//
	uart_init(UART_BAUD_SELECT(UART_BAUD,F_CPU));

	// Initialise time if needed
	time->sec  = 0;     // 0 to 59
	time->min  = 0;     // 0 to 59
	time->hour = 8;     // 0 to 23
	time->mday = 30;    // 1 to 31
	time->mon  = 9;     // 1 to 12
	time->year = 2017;  // year-99
	time->wday = 6;     // 1-7

	struct tm* rtc_time = NULL; //
#if !SIM 
	//rtc_set_time(time); // Used ONLY !!! to initially set time and reprogram
	rtc_time = rtc_get_time();
#endif
	//if (convertTime(time) > convertTime(rtc_time))
	//	rtc_set_time(time);
	//
	int8_t temp_d = 0;  // numeric
	uint8_t temp_f = 0; // fraction
	// 
	long loops = 0
		, ntp_delay = 1000
		;
	//
//#if !SIM	
	saveTopBrightness(); // Saves at start only if values of top differ from eeprom
//#endif
	//set_fan_pwm(TOP_BR); // initially set FAN speed, But also think how to monitor the temp and adapt !!!
	set_fan_pwm(fan_a_duty, fan_b_duty); // Not using soft PWM for FAN
	//
	while (1) {
		//
		// Find a way to publish Temperature
		// Time fromm RTC Module
#if !SIM	    
		time = rtc_get_time();
#endif	    
		//uint8_t hour, min, sec;
		//rtc_get_time_s(&hour, &min, &sec);
		//strcat(char *dest, const char *src)
		communicate("RTC_TIME:", time->hour, time->min);
		//
#if !SIM	    
		ds3231_get_temp_int(&temp_d, &temp_f); // WHY THIS CAUSES COUTERS STOP ???? 
#endif	    
		communicate("SETTEMP:", temp_d, temp_f);

		if (loops == ntp_delay) { // I am sure there is better way but not critical for now
			//
			getNTP(rtc_time);
			if ( convertTime(rtc_time) == 1 )
				rtc_set_time_s(rtc_time->hour, rtc_time->min, rtc_time->sec);
			loops = 0;
		}
//#if !SIM	    
		//	    
		updateTopFromEE(); /* Might cause top_brightness be wrong due to false eeprom reading
		*/
//#endif	    
		// Publish array to esp8266 
		updateLedArray();
		// Update top_brightness only if requested/flag set, and save it back
		requestLedArray();
		//
#if DEBUG	    
		// Test dimmer funtion,
		dimmer(test);
		if (brightness[0] == top_brightness[0]) {
			test = 0;
		}	    
#else
		// Start dimming or brightning up
		if (time->hour >= MORNING && time->hour <= MORNING+1) {
			uart_puts("<Rise and shine>\n\r");
			dimmer(TRUE);
		}  
		else if (time->hour >= EVENING && time->hour <= EVENING+1) {
			uart_puts("<Time to sleep>\n\r");
			dimmer(FALSE);
		}
		else if (time->hour >= MORNING+1 && time->hour <= EVENING) {
			for (int i = 0; i < USELED; i++){
				if (compbuff[i] != top_brightness[i])
					set_diode_pwm(i, top_brightness[i]);
			}

		}
#endif	    
		// 
		communicate("LOOPS_TIME_COUNTER:", loops, time_counter);
		//
		loops++;
		time_counter++; // Maybe will introduce ISR for this but for now not needed
		sleep_mode();  //
		_delay_ms(LOOP_DELAY);
	}
}

//
//
//
void communicate(char command[], int val1, int val2)
{
	/*
	   char messageBuf[32] = {0};
	//strcat(char *dest, const char *src)

	char buf[10];

	strcat(messageBuf, "<");
	strcat(messageBuf, command);
	strcat(messageBuf, itoa(val1, buf, 10));

	//if (val2 != null) {
	strcat(messageBuf, ":");
	strcat(messageBuf, itoa(val2, buf, 10));
	//    sprintf(messageBuf, "<%s%d:%d>\n\r", command, val1, val2);
	*/
	/*} else {
	  sprintf(messageBuf, "<%s%d>\n\r", command, val1);
	  }*/
	/*
	   strcat(messageBuf, ">\n\r");
	   uart_puts(messageBuf);
	   */
	printf("<%s%d:%d>\n\r", command, val1, val2);
}


void uartRecvString() { /* Got that from Arduino esp8266 */
	uint8_t counter = 0; // Just in case to terminate when length exceeded
	boolean newData = FALSE;
	static boolean recvInProgress = FALSE;
	static byte ndx = 0;
	char startMarker = '<';
	char endMarker = '>';
	char rc;
	const byte numChars = sizeof(receivedChars)/sizeof(receivedChars[0]);

	while (newData == FALSE) {
		rc = uart_getc();

		if (recvInProgress == TRUE) {
			if (rc != endMarker) {
				receivedChars[ndx] = rc;
				ndx++;
				if (ndx >= numChars) {
					ndx = numChars - 1;
				}
			}
			else {
				receivedChars[ndx] = '\0'; // terminate the string
				recvInProgress = FALSE;
				ndx = 0;
				newData = TRUE;
			}
		}
		else if (rc == startMarker) {
			recvInProgress = TRUE;
		}
		if (++counter >= 20) break;
	}
}

//
int convertTime(struct tm* ttime) {// THIS IS WRONG AND WILL NOT WORK IN A LONG RUN !!!

	long btime = ttime->year * 100000000
		   + ttime->mon * 1000000
		   + ttime->mday * 10000
                   + ttime->hour * 100 
		   + ttime->min 
		   ;

	long ctime = time->year * 100000000
		   + time->mon * 1000000
		   + time->mday * 10000
                   + time->hour * 100 
		   + time->min
		   ;

	int stuff = 0;
	if ( ctime > btime ) stuff = 1;


	return stuff;
}
//

void getNTP(struct tm* time) {
	// Send get NTP and receive and set time 
	communicate("GET_NTP", 0, 0);
	// receive string
	uartRecvString();

	/* Very imperfect way
	   uint8_t max = 13;
	   char c, buf[max];
	//while (c = uart_getc() && c != '\n' && c != '\r' && i < max) {
	while (c = uart_getc() && c != '>' && c != '\n' && c != '\r' && i < max) {
	buf[i] = c;
	i++;
	}
	buf[i] = '\0';
	*/

	//char *pch = strstr(buf, "NTPGET");
	if(strstr(receivedChars, "NTP_IS:"));
	{
                uint8_t h_len = 1
		      , m_len = 1
		      , s_len = 1      // Cheat
		      , start_from = 7
		      ;
		//copy from srt[start]
		if (receivedChars[start_from + 2] == ':') { // SIMPLIFY/REFINE ???
		    h_len = 2;
		    if (receivedChars[(start_from + h_len + 3)] == ':') { 
		        m_len = 2;
		    }
		} else {
		    if (receivedChars[(start_from + h_len + 3)] == ':') m_len = 2;
		}

	        char hur[h_len], min[m_len], sec[s_len];

		strlcpy(hur, &receivedChars[start_from], h_len); // use strlcpy instead of strncpy !!! MIND THE FORMAT OF RECEIVED TIME !!!
		hur[h_len] = '\0';
		strlcpy(min, &receivedChars[(start_from + h_len + 1)], m_len);
		min[m_len] = '\0';
		strlcpy(min, &receivedChars[(start_from + h_len + m_len + 2)], s_len); // This will always be out of sync
		sec[s_len] = '\0';

		time->hour = atoi(hur);
		time->min  = atoi(min);
		time->sec  = atoi(sec);

	        printf("<%s%s%s>", hur, min, sec);
	}
	//strlcpy (pch,"NTPGET",6);
	//
	/*
	   uart_putc('<');
	   uart_puts(hur);
	   uart_puts(min);
	   uart_puts(sec);
	   uart_putc('>');
	   */
}
//
void updateLedArray() { /* This only sends current state of the led array */
	// Only 8 LED is going to be used for now hence -2
	for (int i = 0; i < USELED; i++){
		// Send current LED state
		communicate("SETLED:", i, compbuff[i]); //
	}
//	communicate("SETFAN:", 10, compbuff[10]);
//	communicate("SETFAN:", 11, compbuff[11]);
	communicate("SETFAN:", FAN_A, fan_a_duty);
	communicate("SETFAN:", FAN_B, fan_b_duty);

}
//
void requestLedArray() { // This requires some work/thought <NOTE>
	//
	char ledBuf[2];
	uint8_t led, ledVal, save = 0; 
	//
	// Send current LED state
	communicate("RECVAL:", 0, 0); //
	// receive string
	uartRecvString();
	// Only 8 LED is going to be used for now
	uint8_t size_of_received = sizeof(receivedChars)/sizeof(receivedChars[0]);
	//
	if       (  size_of_received > 7
			&& strstr(receivedChars, "RECEIV:")
		 ) {
		uint8_t pos = 7, step = 1, charCount, charBuf; // Start cutting the string from
		for (uint8_t i = 0; i < CHMAX; i++) { // This is 1 by one why not send all at once ??
			//copy from srt[start]
			//strlcpy(led, &receivedChars[7], 1);
			// led = receivedChars[7];
			charCount = 0;
			while (charBuf != ':') {
			    strlcpy(charBuf, &receivedChars[pos], step); // what if values are less than 10 ???, Build string with separators and parser!!!
			    ledBuf[charCount] = charBuf;
			    pos += step;
			    charCount++;
			} 			//
			ledBuf[charCount] = '\0'; // 
			ledVal = atoi(ledBuf);
			if (ledVal > 0 && top_brightness[i] != ledVal) { // > 
				top_brightness[i] = ledVal; // Investigate the 
				set_diode_pwm(i, ledVal); 
#if DEBUG
				printf("<LED VAL: %d>\n\r", ledVal);
				printf("<TOP VAL: %d>\n\r", top_brightness[i]);
#endif
			}
		}
		saveTopBrightness(); // 
	}
}
//
void saveTopBrightness() {// ??? What, to save eeprom from quick death it has to be explicitly requested
	// For instance button from web etc.
	// And run after requestLedArray() or inside;
	//
	uint8_t eepromReceived[USELED];
	//
	eeprom_rread_data( EEPROM_LED_PAGE     
			, EEPROM_LED_ADDR
			, eepromReceived
			, USELED);
	//
#if DEBUG
	printf("From EEPROM: ");
	for (int i = 0; i < USELED; i++){
		printf("%d", eepromReceived[i]);
	}
	printf("\n\r");
	printf("Top Brightness: ");
	for (int i = 0; i < USELED; i++){
		printf("%d", top_brightness[i]);
	}
	printf("\n\r");
#endif
	//
	// Only 8 LED is going to be used for now hence -2
	for (int i = 0; i < USELED; i++){
		if ( top_brightness[i] != eepromReceived[i] ) {
			eepromReceived[i] = top_brightness[i];
			eeprom_write_byte( EEPROM_LED_PAGE // SOMETHING WRONG HERE !!  
					, i
					, eepromReceived[i]
					);
		}
	}
#if DEBUG
	printf("Saved to EEPROM: ");
	for (int i = 0; i < USELED; i++){
		printf("%d", eepromReceived[i]);
	}
	printf("\n\r");
#endif
}

void updateTopFromEE() { // For now two similar funcmtions (above )
	//
	uint8_t eepromReceived[USELED];
	//
	eeprom_rread_data( EEPROM_LED_PAGE     
			, EEPROM_LED_ADDR
			, eepromReceived
			, USELED);
	//
	for (int i = 0; i < USELED; i++){
		if (  eepromReceived[i] > 0 
				&& eepromReceived[i] < TOP /* To avoid accidental values of min/max */

		   )  top_brightness[i] = eepromReceived[i];	
	}

#if DEBUG
	printf("<EEPROM REC: ");
	for (int i = 0; i < USELED; i++){
		printf("%d", eepromReceived[i]);
	}
	printf(">\n\r");
	//
	printf("<TOP BRIGHTNESS: ");
	for (int i = 0; i < USELED; i++){
		printf("%d", top_brightness[i]);
	}
	printf(">\n\r");
#endif
}


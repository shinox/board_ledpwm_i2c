/* !!! HEAVILY BASED ON: !!!
 * DS RTC Library: DS1307 and DS3231 driver library
 * (C) 2011 Akafugu Corporation
 *
 * This program is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 */

/*
 * DS1307 register map
 *  
 *  00h-06h: seconds, minutes, hours, day-of-week, date, month, year (all in BCD)
 *     bit 7 of seconds enables/disables clock
 *     bit 6 of hours toggles 12/24h mode (1 for 12h, 0 for 24h)
 *       when 12h mode is selected bit 5 is high for PM, low for AM
 *  07h: control
 *      bit7: OUT
 *      bit6: 0
 *      bit5: 0
 *      bit4: SQWE
 *      bit3: 0
 *      bit2: 0
 *      bit1: RS0
 *      bit0: RS1
 *  08h-3fh: 56 bytes of SRAM
 *
 * DS3231 register map
 *
 *  00h-06h: seconds, minutes, hours, day-of-week, date, month, year (all in BCD)
 *       bit 7 should be set to zero: The DS3231 clock is always running
 *  07h: A1M1  Alarm 1 seconds
 *  08h: A1M2  Alarm 1 minutes
 *  09h: A1M3  Alarm 1 hour (bit6 is am/pm flag in 12h mode)
 *  0ah: A1M4  Alarm 1 day/date (bit6: 1 for day, 0 for date)
 *  0bh: A2M2  Alarm 2 minutes
 *  0ch: A2M3  Alarm 2 hour (bit6 is am/pm flag in 12h mode)
 *  0dh: A2M4  Alarm 2 day/data (bit6: 1 for day, 0 for date)
 *       <see data sheet page12 for Alarm register mask bit tables:
 *        for alarm when hours, minutes and seconds match set 1000 for alarm 1>
 *  0eh: control
 *      bit7: !EOSC
 *      bit6: BBSQW
 *      bit5: CONV
 *      bit4: RS2
 *      bit3: RS1
 *      bit2: INTCN
 *      bit1: A2IE
 *      bit0: A1IE
 *  0fh: control/status
 *      bit7: OSF
 *      bit6: 0
 *      bit5: 0
 *      bit4: 0
 *      bit3: EN32kHz
 *      bit2: BSY
 *      bit1: A2F alarm 2 flag
 *      bit0: A1F alarm 1 flag
 * 10h: aging offset (signed)
 * 11h: MSB of temp (signed)
 * 12h: LSB of temp in bits 7 and 6 (0.25 degrees for each 00, 01, 10, 11)
 *
 */

#include <avr/io.h>

#include "rtc.h"
#include "conf.h"

#define RTC_ADDR 0x68 // I2C address
#define CH_BIT 7 // clock halt bit

// statically allocated structure for time value
struct tm _tm;

uint8_t dec2bcd(uint8_t d)
{
	return ((d/10 * 16) + (d % 10));
}

uint8_t bcd2dec(uint8_t b)
{
	return ((b/16 * 10) + (b % 16));
}

uint8_t rtc_read_byte(uint8_t offset)
{
	i2c_start(RTC_ADDR << 1);
	i2c_write(offset);
	i2c_start((RTC_ADDR << 1) | 1); /// ???	

	uint8_t ret = i2c_readNak();

	i2c_stop();
	return ret;
}

void rtc_write_byte(uint8_t b, uint8_t offset)
{
	i2c_start(RTC_ADDR << 1); // Write address and direction
	i2c_write(offset);
	i2c_write(b);
	i2c_stop();
}

static bool s_is_ds1307 = false;
static bool s_is_ds3231 = false;

void rtc_init(void)
{
	// Attempt autodetection:
	// 1) Read and save temperature register
	// 2) Write a value to temperature register
	// 3) Read back the value
	//   equal to the one written: DS1307, write back saved value and return
	//   different from written:   DS3231

	uint8_t temp1 = rtc_read_byte(0x11);
	uint8_t temp2 = rtc_read_byte(0x12);

	rtc_write_byte(0xee, 0x11);
	rtc_write_byte(0xdd, 0x12);

	if (rtc_read_byte(0x11) == 0xee && rtc_read_byte(0x12) == 0xdd) {
		s_is_ds1307 = true;
		// restore values
		rtc_write_byte(temp1, 0x11);
		rtc_write_byte(temp2, 0x12);
	}
	else {
		s_is_ds3231 = true;
	}
}

// Autodetection
bool rtc_is_ds1307(void) { return s_is_ds1307; }
bool rtc_is_ds3231(void) { return s_is_ds3231; }

// Autodetection override
void rtc_set_ds1307(void) { s_is_ds1307 = true;   s_is_ds3231 = false; }
void rtc_set_ds3231(void) { s_is_ds1307 = false;  s_is_ds3231 = true;  }

struct tm* rtc_get_time(void)
{
	uint8_t rtc[9];
	uint8_t century = 0;

	// read 7 bytes starting from register 0
	// sec, min, hour, day-of-week, date, month, year
	// write address & direction
	//i2c_start(RTC_ADDR << 1);
	//i2c_write(0x0);   //
	//i2c_stop();

	//i2c_start((RTC_ADDR << 1) | 1);
	i2c_start((RTC_ADDR << 1) | 1);

	uint8_t ack = 1;
	for (uint8_t i = 0; i < 7; i++) {
		/* ack:
		 *     1 send ack, request more data from device
		 *     0 send nak, read is followed by a stop condition 
		 */
		if (i == 6 /* less than ( 7 - 1 ) */) ack = 0;
		rtc[i] = i2c_read(ack);

	}

	i2c_stop();

	// Clear clock halt bit from read data
	// This starts the clock for a DS1307, and has no effect for a DS3231
	//rtc[0] &= ~(_BV(CH_BIT)); // clear bit

	_tm.sec = bcd2dec(rtc[0]);
	_tm.min = bcd2dec(rtc[1]);
	_tm.hour = bcd2dec(rtc[2]);
	_tm.mday = bcd2dec(rtc[4]);
	_tm.mon = bcd2dec(rtc[5] & 0x1F); // returns 1-12
	century = (rtc[5] & 0x80) >> 7;
	_tm.year = century == 1 ? 2000 + bcd2dec(rtc[6]) : 1900 + bcd2dec(rtc[6]); // year 0-99
	_tm.wday = bcd2dec(rtc[3]); // returns 1-7

	if (_tm.hour == 0) {
		_tm.twelveHour = 0;
		_tm.am = 1;
	} else if (_tm.hour < 12) {
		_tm.twelveHour = _tm.hour;
		_tm.am = 1;
	} else {
		_tm.twelveHour = _tm.hour - 12;
		_tm.am = 0;
	}

	return &_tm;
}

void rtc_get_time_s(uint8_t* hour, uint8_t* min, uint8_t* sec)
{
	uint8_t rtc[9];

	// write address & direction
	i2c_start(RTC_ADDR << 1);
	i2c_write(0x0);   // 
	i2c_start((RTC_ADDR << 1) | 1);

	uint8_t ack = 1;
	for(uint8_t i=0; i<7; i++) {
		/* ack:
		 *     1 send ack, request more data from device
		 *     0 send nak, read is followed by a stop condition 
		 */
		if (i == 6 /* less than ( 7 - 1 ) */) ack = 0;
		rtc[i] = i2c_read(ack);
	}

	i2c_stop();

	if (sec)  *sec =  bcd2dec(rtc[0]);
	if (min)  *min =  bcd2dec(rtc[1]);
	if (hour) *hour = bcd2dec(rtc[2]);
}

// fixme: support 12-hour mode for setting time
void rtc_set_time(struct tm* tm_)
{
	i2c_start(RTC_ADDR << 1); // ???
	i2c_write(0x0);
	uint8_t century;
	if (tm_->year > 2000) {
		century = 0x80;
		tm_->year = tm_->year - 2000;
	} else {
		century = 0;
		tm_->year = tm_->year - 1900;
	}

	// clock halt bit is 7th bit of seconds: this is always cleared to start the clock
	i2c_write(dec2bcd(tm_->sec)); // seconds
	i2c_write(dec2bcd(tm_->min)); // minutes
	i2c_write(dec2bcd(tm_->hour)); // hours
	i2c_write(dec2bcd(tm_->wday)); // day of week
	i2c_write(dec2bcd(tm_->mday)); // day
	i2c_write(dec2bcd(tm_->mon) + century); // month
	i2c_write(dec2bcd(tm_->year)); // year

	i2c_stop();
}

void rtc_set_time_s(uint8_t hour, uint8_t min, uint8_t sec)
{
	i2c_start(RTC_ADDR << 1); // ???
	i2c_write(0x0);

	i2c_write(dec2bcd(sec)); // seconds
	i2c_write(dec2bcd(min)); // minutes
	i2c_write(dec2bcd(hour)); // hours

	i2c_stop();
}

void ds3231_get_temp_int(int8_t* i, uint8_t* f)
{
	uint8_t msb, lsb;

	*i = 0;
	*f = 0;

	// if (s_is_ds1307) return; // only valid on DS3231
	i2c_start(RTC_ADDR << 1);
	i2c_write(0x11);   //

	i2c_start((RTC_ADDR << 1) | 1);

	msb = i2c_readAck();
	lsb = i2c_readNak();

	*i = msb;
	*f = (lsb >> 6) * 25;

	i2c_stop();
}


//
// EEPROM
//
byte highAddressByte(word address)
{
	/*
	   byte BYTE_1;
	   BYTE_1 = address >> 8;
	   return BYTE_1;
	   */
	return (uint8_t)(((unsigned)address) >> 8);
}

byte lowAddressByte(word address)
{
	/*
	   byte BYTE_1;
	   byte BYTE_2;
	   BYTE_1 = address >> 8;
	   BYTE_2 = address - (BYTE_1 << 8);
	   return BYTE_2;
	   */
	return (uint8_t)(((unsigned)address) & 0xff);
}
// 
/* From Datasheet
 *
 * BYTE WRITE:
 A write operation requires two 8-bit data word addresses following the
 device address word and acknowledgment. Upon receipt of this address, the 
 EEPROM
 will again respond with a zero and then clock in the first 8-bit data word. Following
 receipt of the 8-bit data word, the EEPROM will output a zero and the addressing
 device, such as a microcontroller, must terminate the write sequence with a stop condi-
 tion. At this time the EEPROM enters an internally-timed write cycle, t
 WR, to the nonvolatile memory. All inputs are disabled during this write cycle and the EEPROM will
 not respond until the write is complete (refer to Figure 2).
 PAGE WRITE:
 The 32K/64K EEPROM is capable of 32-byte page writes.
 A page write is initiated the same way as a byte write, but the microcontroller does not
 send a stop condition after the first data word is clocked in. Instead, after the EEPROM
 acknowledges receipt of the first data word, the microcontroller can transmit up to 31
 more data words. The EEPROM will respond with a zero after each data word received.
 The microcontroller must terminate the page write sequence with a stop condition (refer
 to Figure 3).
 The data word address lower 5 bits are internally incremented following the receipt of
 each data word. The higher data word address bits are not incremented, retaining the
 memory page row location. When the word address, internally generated, reaches the
 page boundary, the following byte is placed at the beginning of the same page. If more
 than 32 data words are transmitted to the EEPROM, the data word address will ¿roll
 over¿ and previous data will be overwritten.
 ACKNOWLEDGE POLLING:
 Once the internally-timed write cycle has started and the
 EEPROM i
 nputs are disabled, acknowl
 edge polling can be initiated. This involves send-
 ing a start condition followed by the device address word. The read/write bit is
 representative of the operation desired. Only if the internal write cycle has completed
 will the EEPROM respond with a zero, allowing the read or write sequence to continue.
 Read operations are initiated the same way as write operations with the exception that the
 read/write select bit in the device address word 
 is set to one. There are three read operations:
 current address read, random
 address read and sequential read.
 CURRENT ADDRESS READ:
 The internal data word address counter maintains the last
 address accessed during the last read or write operation, incremented by one. This address
 stays valid between operations as long as the chip power is maintained. The address ¿roll
 over¿ during read is from the last byte of the last memory page, to the first byte of the first
 page. The address ¿roll over¿ during write is from the last byte of the current page to the first
 byte of the same page.
 Once the device address with the read/write select bit set to one is clocked in and acknowl-
 edged by the EEPROM, the current address data word is serially clocked out. The
 microcontroller does not respond with an input zero but does generate a following stop condi-
 tion (refer to Figure 4).
 RANDOM READ: 
 A random read requires a ¿dummy¿ byte write sequence to load in the data
 word address. Once the device address word and data word address are clocked in and
 acknowledged by the EEPROM, the microcontroller must generate another start condition.
 The microcontroller now initiates a current address read by sending a device address with the
 read/write select bit high. The EEPROM acknowledges the device address and serially clocks
 out the data word. The microcontroller does not respond with a zero but does generate a fol-
 lowing stop condition (refer to Figure 5).
 SEQUENTIAL READ:
 Sequential reads are initiated by either a current address read or a ran-
 dom address read. After the microcontroller receives a data word, it responds with an
 acknowledge. As long as the EEPROM receives an acknowledge, it will continue to increment
 the data word address and serially clock out sequential data words. When the memory
 address limit is reached, the data word addre
 ss will ¿roll over¿ and the sequential read will con-
 tinue. The sequential read operation is terminated when the microcontroller does not respond
 with a zero but does generate a following stop condition (refer to Figure 6)
 */
void eeprom_write_byte( uint8_t start_at_page_address // READ DATASHEET !!!
		, uint8_t start_at_row_address
		, unsigned char stuff
		) {
	unsigned short a = (start_at_row_address << 8) | start_at_page_address;
#if SIM	
	i2c_start(AT24C32 + I2C_WRITE);
#else	
	i2c_start((uint8_t)(AT24C32 << 1));
#endif	
	i2c_write(highAddressByte(a));
	i2c_write(lowAddressByte(a));
	i2c_write(stuff);
	i2c_stop();
}

/*
 * Write whole page up to 32 bytes in length
 * 0 - 256 pages, 0 - 32 addresses
 * There is no verification for exceeding maximum page 32 bytes !!!!
 */ 
void eeprom_write_upto32b_page( uint8_t start_at_page_address
		, uint8_t start_at_row_address
		, unsigned char *page
		, uint8_t size
		) { //
	unsigned short a = (start_at_row_address << 8) | start_at_page_address;
#if SIM
#else
	i2c_start((uint8_t)(AT24C32 << 1));
#endif	
	i2c_write(highAddressByte(a));
	i2c_write(lowAddressByte(a));
	for (uint8_t i = 0; i < size; i++) {
		i2c_write(page[i]);
	}
	i2c_stop();	
}

/*
 * Read whole page up to 32 bytes in length
 * 0 - 256 pages, 0 - 32 addresses
 * There is no verification for exceeding maximum page 32 bytes !!!!
 */ 
void eeprom_read_page( uint8_t start_at_page_address    // 
		, uint8_t start_at_row_address
		, unsigned char *page
		, uint8_t size
		) { //
	unsigned short a = (start_at_row_address << 8) | start_at_page_address;
#if SIM
#else
	i2c_start((uint8_t)(AT24C32 << 1));
	i2c_write(highAddressByte(a));
	i2c_write(lowAddressByte(a));
	i2c_start((uint8_t)((AT24C32 << 1) | 1));
#endif	
	uint8_t ack = 1;
	for (uint8_t i = 0; i < size; i++) {
		if (i == (size - 1) /* less than ( size - 1 ) */) ack = 0;
		page[i] = i2c_read(ack);	    	       
	}
	i2c_stop();
}

//
unsigned char eeprom_read_byte( uint8_t start_at_page_address    // 
		, uint8_t start_at_row_address
		) { //
	unsigned short a = (start_at_row_address << 8) | start_at_page_address;
#if SIM	
	i2c_start(AT24C32 + I2C_WRITE);
	i2c_write(highAddressByte(a));   // Random Read Dummy Write Address 1st 2nd Word
	i2c_write(lowAddressByte(a));
	i2c_start(AT24C32 + I2C_READ);
#else	
	i2c_start((uint8_t)(AT24C32 << 1));
	i2c_write(highAddressByte(a));   // Random Read Dummy Write Address 1st 2nd Word
	i2c_write(lowAddressByte(a));
	i2c_start((uint8_t)((AT24C32 << 1) | 1));
#endif	
	uint8_t ret = i2c_readAck();
	i2c_stop();
	return ret;
}

/*
 * No test for > 32 bytes write !!!
 */
uint8_t eeprom_rwrite_data( uint8_t start_at_page_address    // 
		, uint8_t start_at_row_address
		, unsigned char *data
		, uint8_t size
		) {
	for (int i = 0; i < size; i++) {
		eeprom_write_byte( start_at_page_address // READ DATASHEET !!!
				, start_at_row_address
				, data[i]
				);
		start_at_row_address++;

	}
	return start_at_row_address; // Return where finished so it can be followed by usual write page or byte,
	// calculate full address ???,  <- need to change all for this
}

/*
 * 0 - 256 pages, 0 - 32 addresses
 * There is no verification for exceeding maximum page 32 bytes !!!!
 * Least efficient way, easiest would be read page and deal with it later
 */
uint8_t eeprom_rread_data( uint8_t start_at_page_address    // 
		, uint8_t start_at_row_address
		, unsigned char *data
		, uint8_t size
		) {
	for (int i = 0; i < size; i++) {
		data[i] = eeprom_read_byte(start_at_page_address, start_at_row_address);
		start_at_row_address++;
	}
	return start_at_row_address; 
}

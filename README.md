# board_ledpwm_i2c
based on simavr, Atmega32 test of pwm dimming LED and FAN control,

!!! NO WARRANTY IS OFFERED NOR ANY RESPONSIBILITY TAKEN FOR USAGE OF THIS CODE AND OR HARDWARE DESIGN !!!

Idea to test ability of Atmega32 to software PWM 10x 10W LED lights and cooling FANs, providing cheap light alternative for Marine Aquarium.

   It is based on various other projects, mainly "simavr" for testing and core functionality, AVR note 136 (AVR136) modified to supply ~1953Hz PWM,
   Mode achieved by setting the "Normal" mode "No Preskaler" for timer0 and set OCR0 = TOP, then using vector OVF to conrol the PWM, Here TOP is set to be 16 due to software pwm speed limitation. Overflow happens every 256 CPU cycles what for fastpwm mode is frequency=fcpu/256/256 ~122Hz because Timer0 is 8 bit and maximum count value is 256, when 16 is chosen in theory this should give frequency=fcpu/256/16 ~1953Hz at a cost of very low resolution of approx 16 steps to max PWM.
    The value is confirmed by examination of .vcd file in gtkwave, the period is ~513us = 0.000513s i.e. 1 / 0.000513 = 1949Hz.

The Hardware:
* EasyEDA for final PCB design and development of the board: 
https://easyeda.com/etomni/Mtmega32_LED_Driver-38f18a490ff54bd6aba8b6f0eb5d93fa

Requires:
1. Eternal DS3231 module to function in Live mode,
2. Optionally WiFi ns8266 can be connected but needs separate code for Website monitoring and Controller (Dev sketch exists but not ready for publication yet),


To try:

    Cd to "simavr/examples/" then clone and "make".
    
    
What works:
1. After power on software will try to slowly rise or dimm brightness of all connected LEDs depending on DAY/NIGHT settings in "conf.h", 
2. Time is obtained by connecting to ds3231 RTC module using SDA/SCL pins (TWI/I2C). The TOP brightness then will be saved to AT24C32 flash onboard DS3231 module,
3. When DEBUG and SIM enabled ( = 1) the software will demonstrate rising and falling brightness - demo mode,
4. WHen RTC connected Live mode works very well,

What does not work:
1. Communication protocol(UART) between The Board and ns8266 WiFi module need a lot tuning - probably redefining too,
2. Simulation only works if DEBUG and SIM enabled, RTC simulation is not implemented at all !!!
3. Requires connection to DS3231 RTC module for Live operation (DEBUG = 0 and SIM = 0),

Wish list:
1. FAN controll based on temperature reading from DS3231,
2. Second Temperature probe ?,
3. Communication with ns8266 via UART need re-organising or re-defining,
4. WebUI after ns... connected,
5. External LCD or button controlls (but why if already fully auto)???,
6. Ability for adaptive light based on Weather, 

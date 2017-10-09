# board_ledpwm_i2c
based on simavr, Atmega32 test of pwm dimming LED and FAN control,

NO WARRANTY IS OFFERED NOR ANY RESPONSIBILITY TAKEN FOR USAGE OF THIS CODE AND OR HARDWARE DESIGN !!!

To try:
    Clone to "simavr/examples/" then make.

Idea to test ability of Atmega32 to software PWM 10x 10W LED lights and cooling FANs, providing cheap light alternative for Marine Aquarium.

It is based on various other projects, mainly "simavr" for testing and core functionality,
    AVR note 136 (AVR136) modified to supply ~973Hz PWM,
    EasyEDA for final PCB design and development of the board: 
    https://easyeda.com/etomni/Mtmega32_LED_Driver-38f18a490ff54bd6aba8b6f0eb5d93fa
  
Requires:
 1.) Eternal DS3231 module to function in Live mode,
 2.) Optionally WiFi ns8266 can be connected but needs separate code for Website monitoring and Controller (Dev sketch exists but not ready for publication yet),

What works:
 1.) After power on software will try to slowly rise or dimm brightness of all connected LEDs depending on DAY/NIGHT settings in "conf.h", Time is obtained by connecting to ds3231 RTC module using SDA/SCL pins (TWI/I2C). The TOP brightness then will be saved to AT24C32 flash onboard DS3231 module,
 When DEBUG and SIM enabled ( = 1) the software will demonstrate rising and falling brightness - demo mode,

What does not work:
 1.) Communication protocol(UART) between The Board and ns8266 WiFi module need a lot tuning - probably redefining too,
 2.) Simulation only works if DEBUG and SIM enabled, RTC simulation is not implemented at all !!!
 3.) Real life mode works very well providing all modules connected,
 4.) Requires connection to DS3231 RTC module for Live operation (DEBUG = 0 and SIM = 0),
 

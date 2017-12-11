#define F_CPU          8000000L // This is also defined in twimaster.h
//#define F_CPU          16000000L // This does not work !!!
#define UART_BAUD      38400 // For 8MHz 9600, 19.2K, 38.4K have 0.2% error
//
#define DEBUG          1 // By default to show rising and dimming
#define SIM            1 // Currently AT24C32 functionality is slightly different from simavr i2c approach 
			 // hence need to distinguish between sim mode and live operations
//
#define LOOP_DELAY     2000
// Top counter value (OC0 = TOP), Very Important Choice !!!
#define TOP	       32 // 64 = 2xPWM softw freq, 32 = 4x softw PWM freq, 16 = 8x but only 16 steps 
// Calculation depends strictly from OCR0 value, 64 ~= 50% for OCR0 = TOP = 127
//
#define NIGHTMODE      1       // Roughly value of 10% for choosen TOP = 32 value
//
#define BRIGHTNESS_INCREMENT 1
//
#define TOP_BR	       (TOP/2) // Always start with max up to 50% brightness
//
#define HALF_DELAY_PERIOD 30   // In minutes how long till 50% brightness reached,
			       // !!! NOT USED SEE pwm_channels.h line 76
// Morning and evening values
#define MOR            8
#define EVE            19
//
#define CHMAX          10    // maximum number of PWM channels, 10 LED, -1
#define SKIP_LAST_LEDS 0  // in case any number of last led avoided, here using only first 8 LED, not using these for FAN anymore hence 2 instead of 4
// NULL defined
#define NULL           ( (void *) 0) // or #define NULL 0
// Size of A
#define ARRAY_SIZE(a)  ((sizeof(a)/sizeof(a[0])))
// Uart Receive
typedef unsigned char byte;
typedef _Bool         boolean; // In C99 _Bool is bool otherwise include stdbool.h 
char                  receivedChars[32];   // an array to store the received data
char                  messageBuf[32];
// LED devfs in pwm_channels.h
extern long           time_delay; // The number of counter loops required to trigger dimmer() action, 
// look into pwm_channels.c (h) dimmer() function and defines
extern uint8_t        brightness_threshold;
extern uint8_t        top_brightness[CHMAX]; // 
extern uint8_t        brightness[USELED];
extern long           time_counter;
extern uint8_t        MORNING, EVENING;
extern uint8_t        set_hour, set_minute, set_sec;
extern struct         tm* time;
//
#define FALSE           0
#define TRUE            1
// EEPROM
#define EEPROM_LED_PAGE 255     
#define EEPROM_LED_ADDR 0
#define AT24C_PAGESIZE  0x20
#if SIM
#define AT24C32	        0xA0	/* simavr i2c value */
#else
#define AT24C32	        0x57	/* Real Value for EEPROM Address in DS... module */
#endif
typedef unsigned char  byte;
typedef unsigned short word;
byte data;
// FAN
#define FAN_A           10
#define FAN_B           11
#define FAN_A_DUTY      127 // This is driven by Timer1 NOT soft pwm for LED
#define FAN_B_DUTY      127
extern uint8_t fan_a_duty, fan_b_duty;

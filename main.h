


#ifndef MAIN_H_
#define MAIN_H_

typedef unsigned char Uchar;
typedef unsigned char Uint8;
typedef signed char Int8;
typedef unsigned short Ushort;
typedef unsigned short Uint16;
typedef short Int16;
typedef unsigned long Uint32;
typedef unsigned long Ulong;
typedef long Int32;


#define true     1
#define false    0
#define SOFTWARE_VERSION "0.0.0"

#define _WDT_RESET()            asm("wdr")
#define _SLEEP()                asm("sleep")

#define get_val(x) ({ typeof(x) temp; char tsreg=SREG; cli(); temp = x; SREG = tsreg; temp; }) // доступ к неатомарным переменным
#define set_val(x, val) { char tsreg=SREG; cli(); x=val; SREG = tsreg; } // доступ к неатомарным переменным



#include <avr/io.h>
#include "avr/interrupt.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"
#include "ctype.h"
#include "stdlib.h"
#include "avr/pgmspace.h"

#include "GSM.h"
#include "UART.h"
#include "timer.h"
#include "port.h"
#include "eeprom.h"
#include "app.h"
#include "ADC.h"
#include "sms_parse.h"


extern void reset_mcu(void);






#endif /* MAIN_H_ */

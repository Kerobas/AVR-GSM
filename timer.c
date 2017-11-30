
#include "main.h"

static short time_ms=0;
static short time_s=0;

static char beep_on = false;
static Ushort beep_counter = 0;

static Ushort soft_wdt = 0;
Ulong time_from_start_s=0;
Ulong time_from_event_s=0;

char switch_off_from_button = false;

// период переполнения 10 мс при частоте кварца 7,37МГц
void timer0_init(void)
{
	TCCR0 = (1<<WGM01)|(1<<CS02)|(1<<CS00); // Clear Timer on Compare Match (CTC), presciler=1024
	TCNT0 = 0;
	OCR0 = 72;
	TIMSK |= 1<<OCIE0;
}

//*******************************************************************************************************************

void beep_non_block(Ushort time_to_beep)
{
	cli();
	beep_on = true;
	beep_counter = time_to_beep;
	start_buzzer();
	sei();
}

//*******************************************************************************************************************

void beep_control(void)
{
	if(beep_on)
	{
		if(beep_counter >= 10)
			beep_counter -= 10;
		else
		{
			beep_counter = 0;
			beep_on = false;
			stop_buzzer();
		}
	}
}

//*******************************************************************************************************************

// прерывание вызывается с периодом 10 мс
ISR(TIMER0_COMP_vect)
{
	static Uchar i=0;
	
	ADC_result_processing();
	led_management();
	check_button();
	beep_control();
	time_ms+=10;
	i++;
	if(i>=100) // 1 секунда
	{
		i=0;
		time_s++;
		time_from_start_s++;
		soft_wdt++;
#if(DEBUG==0)		
		if(soft_wdt > 1200) // 1200 секунд 20 минут
			reset_mcu();
#endif
		check_power();
		inc_time_from_last_event();
	}
#if(DEBUG==0)
	_WDT_RESET(); // сброс сторожевого таймера
#endif
}

//*******************************************************************************************************************

short get_time_ms(void)
{
	return get_val(time_ms);
}

//*******************************************************************************************************************

short get_time_s(void)
{
	return get_val(time_s);
}

//*******************************************************************************************************************

void reset_soft_wdt(void)
{
	set_val(soft_wdt, 0);
}

//*******************************************************************************************************************

void inc_time_from_last_event(void)
{
	if((config.last_event[0] != false) && (time_from_event_s < MAX_TIME_FROM_EVENT))
		time_from_event_s++;
}

//*******************************************************************************************************************
// задержка на х милисекунд
void delay_ms(short delay)
{
	short time_stamp = get_time_ms() + delay;
	while((get_time_ms() - time_stamp) <= 0)
	{
		#if(DEBUG==0)
		_SLEEP();
		#endif
		while(is_queue_not_empty()) // тем временем, проверяем входной буфер
			get_message_from_mdm();
	}
}

//*******************************************************************************************************************
// задержка на х секунд
void delay_s(short delay)
{
	short time_stamp = get_time_s() + delay;
	while((get_time_s() - time_stamp) <= 0)
	{
		#if(DEBUG==0)
		_SLEEP();
		#endif
		while(is_queue_not_empty()) // тем временем, проверяем входной буфер
			get_message_from_mdm();
	}
}

//*******************************************************************************************************************

// таймер для генерации звукового сигнала
void timer1_init(void)
{
	TCCR1A = 0x10;
	TCCR1B = 0x08; 
	OCR1AH = 0x03;
	OCR1AL = 0xE8;
	OCR1BH = 0x03;
	OCR1BL = 0xE8;
}

//*******************************************************************************************************************
// звуковой сигнал
void start_buzzer(void)
{
	TCCR1B |= 2;
	DDRD |= 1<<4;
}

//*******************************************************************************************************************
// звуковой сигнал
void stop_buzzer(void)
{
	TCCR1B &= ~0x7;
	DDRD &= ~(1<<4);
}

//*******************************************************************************************************************

void beep_ms(Ushort time_ms)
{
	if(config.debug_voice_enable == true)
		start_buzzer();
	delay_ms(time_ms);
	stop_buzzer();
	delay_ms(30);
}












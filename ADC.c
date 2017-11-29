

#include "main.h"

#define FILTER_COEFFICIENT 4 /*не более 6, чтобы Ushort не переполнился*/


static Ushort Integral = 0;
static Ushort filtered_value;

Ulong time_without_power=0;

//*******************************************************************************************************************

// преобразование стартуется по Timer/Counter0 Compare Match
void ADC_init(void)
{
	ADMUX = (1<<REFS0)|(1<<MUX1)|(1<<MUX0);
	ADCSRA = (1<<ADEN)|(1<<ADPS1)|(1<<ADPS2)|(1<<ADSC);
}

//*******************************************************************************************************************

void ADC_result_processing(void)
{
	Ushort data;
	
	((char*)(&data))[0] = ADCL;
	((char*)(&data))[1] = ADCH & 0x3;
	ADCSRA = (1<<ADEN)|(1<<ADPS1)|(1<<ADPS2)|(1<<ADSC); // старт следующего преобразования
	
	// экспоненциальный фильтр
	filtered_value = Integral>>FILTER_COEFFICIENT;
	Integral -= filtered_value;
	Integral += data;
}

//*******************************************************************************************************************

char is_external_pwr(void)
{
	if(get_val(filtered_value) > 248) // 248*(33/1024) = 8 В (порог - 8 В)
		return true;
	else
		return false;
}

//*******************************************************************************************************************

float get_pwr_voltage_f(void)
{
	float rez;
	rez = get_val(filtered_value);
	rez = rez*(33.0/1024.0);
	return rez;
}

//*******************************************************************************************************************

void check_power(void)
{
	if(is_external_pwr())
		time_without_power = 0;
	else
		time_without_power++;
}

//*******************************************************************************************************************

void power_control(void)
{

}













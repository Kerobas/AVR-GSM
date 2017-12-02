
#include "main.h"


config_t config;

//*******************************************************************************************************************
// здесь обязательно нужен высокий уровень оптимизации, т.к. между записями EEMWE EEWE должно быть не более 4 тактов
void __attribute__((optimize("-O3"))) EEPROM_write(Ushort address, Uchar data)
{
	char tsreg;
	tsreg = SREG;
	cli();
	while(EECR & (1<<EEWE)); // Wait for completion of previous write
	EEAR = address; // Set up address and data registers 
	EEDR = data; 
	EECR = (1<<EEMWE); // Write logical one to EEMWE 
	EECR = (1<<EEWE); // Start eeprom write by setting EEWE 
	SREG = tsreg;
}

//*******************************************************************************************************************

Uchar EEPROM_read(Ushort address)
{
	while(EECR & (1<<EEWE)); // Wait for completion of previous write 
	EEAR = address; // Set up address register 
	EECR |= (1<<EERE); // Start eeprom read by writing EERE 
	return EEDR; // Return data from data register 
}

//*******************************************************************************************************************

void EEPROM_write_buf(char *buf, Ushort len, Ushort address)
{
	while(len--)
		EEPROM_write(address++, *buf++);
}

//*******************************************************************************************************************

void EEPROM_read_buf(char *buf, Ushort len, Ushort address)
{
	while(len--)
		*buf++ = EEPROM_read(address++);
}

//*******************************************************************************************************************

void eeprom_read_config(void)
{
	EEPROM_read_buf((char*)&config, sizeof(config_t), 0);
	if(config.test_mode == 0xFF)
	{
		memset(&config, 0, sizeof(config_t));
		config.test_mode = true;
		config.debug_voice_enable = true;
		sprintf_P(config.test_domen, PSTR("www.yandex.ru"));
		eeprom_save_config();
	}
	if(get_val(time_from_event_s) < config.time_from_event_s)
		set_val(time_from_event_s, config.time_from_event_s);
}

//*******************************************************************************************************************

void eeprom_save_config(void)
{
	config.time_from_event_s = get_val(time_from_event_s);
	EEPROM_write_buf((char*)&config, sizeof(config_t), 0);
}

//*******************************************************************************************************************

void EEPROM_save_reset_count(void)
{
	EEPROM_write_buf((char*)&config.unable_to_turn_on_modem, sizeof(config.unable_to_turn_on_modem), 0 + (Ushort)((char*)&config.unable_to_turn_on_modem - (char*)&config));
	EEPROM_write_buf((char*)&config.reset_count, sizeof(config.reset_count), 0 + (Ushort)((char*)&config.reset_count - (char*)&config));
}

//*******************************************************************************************************************

void EEPROM_save_report_to_developer(void)
{
	EEPROM_write_buf((char*)&config.reports_en, sizeof(config.reports_en), 0 + (Ushort)((char*)&config.reports_en - (char*)&config));
}

//*******************************************************************************************************************

void EEPROM_save_time_from_event(void)
{
	Ulong tempUL;
	
	tempUL = get_val(time_from_event_s);
	if(config.time_from_event_s != tempUL)
	{
		config.time_from_event_s = tempUL;
		EEPROM_write_buf((char*)&config.time_from_event_s, sizeof(tempUL), 0 + (Ushort)((char*)&config.time_from_event_s - (char*)&config));
	}
}

//*******************************************************************************************************************

void check_rst_source(void)
{
	if((MCUCSR & (1<<3)) == false) // источником перезапуска был не сторожевой таймер (скорее всего по питанию)
	{
		memset(config.last_event, 0, sizeof(config.last_event));
		sprintf_P(config.last_event, PSTR("Device turned on "));
		set_val(time_from_event_s, 0);
		eeprom_save_config();
	}
	MCUCSR = 0x1F; // обнуляем статусные биты
}













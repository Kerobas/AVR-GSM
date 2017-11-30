
#include "main.h"



void reset_mcu(void)
{
#if(DEBUG==0)
	config.reset_count++;
	if(config.reset_count < 80000UL)
		EEPROM_save_reset_count();
	EEPROM_save_time_from_event();
#endif
	cli(); // запрещаем прерывания и ждем перезагрузки по сторожевому таймеру
	while(1); 
}

//*******************************************************************************************************************


int main(void)
{
	char rez;
	
#if(DEBUG==0)
	_WDT_RESET(); // сброс сторожевого таймера
	WDTCR = (1<<WDE) | (1<<WDP0) | (1<<WDP1) | (1<<WDP2); // период сторожевого таймера 2.2 секунд
#endif
	port_init();
	timer0_init();
	timer1_init();
	init_uart(true);
	ADC_init();
	sei();
	
	delay_ms(1000);
	
	eeprom_read_config();
	check_rst_source();
	
	external_communication();
	
	beep_ms(10);
	beep_ms(10);
	beep_ms(10);
	
	rez = gsm_mdm_init();
	if(rez == false)
	{
		if(config.unable_to_turn_on_modem < 80000UL)
			config.unable_to_turn_on_modem++;
		reset_mcu();
	}
	reset_soft_wdt();
	
	
	rez = mdm_wait_registration_s(600);
	if(rez == false)
		reset_mcu();
	reset_soft_wdt();
	
	rez = mdm_get_operator_name();
	if(rez == false)
		reset_mcu();
	
	rez = mdm_setup_apn();
	if(rez == false)
		reset_mcu();
	reset_soft_wdt();
	
	beep_ms(10);
	beep_ms(10);
	beep_ms(200);
	
	get_sms();
	delete_all_sms(); // попытка обойти глюк модема
	
	
	while(1)
    {
		if(reset_command_accepted)
		{
			delay_s(8);
			reset_mcu();
		}
		switch_off_server_if_needed();
		turn_on_server_if_needed();
		update_server_state_if_needed();
		reset_soft_wdt();
		external_communication();
		get_sms();
		incoming_call_processing();
		power_control();
		while(is_queue_not_empty())
			get_message_from_mdm();			
#if(DEBUG==0)
		_SLEEP();
#endif
    }
}











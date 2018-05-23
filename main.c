
#include "main.h"



void reset_mcu(char increment)
{
#if(DEBUG==0)
	if((config.reset_count < 80000UL) && increment)
	{
		config.reset_count++;
		EEPROM_save_reset_count();
	}
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
	sei(); // разрешаем прерывания
	
	delay_ms(500); // задержка перед работой с EEPROM, т.к. на старте питание может быть не стабильным
	eeprom_read_config(); // вычитываем настройки
	check_rst_source(); // смотрим, что было источником перезагрузки
	
	external_communication(); // проверяем подключение по USB
	
	beep_ms(10); // три коротких звуковых сигнала
	beep_ms(10);
	beep_ms(10);
	
	rez = gsm_mdm_init(); // инициализация GSM модема
	if(rez == false)
	{
		if(config.unable_to_turn_on_modem < 80000UL)
			config.unable_to_turn_on_modem++;
		reset_mcu(true);
	}
	reset_soft_wdt();
	
	rez = mdm_wait_registration_s(600); // ждем регистрации в GSM сети 600 секунд
	if(rez == false)
		reset_mcu(true);
	reset_soft_wdt();
	
	rez = mdm_get_operator_name();
	if(rez == false)
		reset_mcu(true);
	
	rez = mdm_setup_apn();
	if(rez == false)
		reset_mcu(true);
	reset_soft_wdt();
	
	beep_ms(10); // звуковой сигнал после успешной регистрации в сети
	beep_ms(10);
	beep_ms(200);
	
	get_sms(); // чиаем принятые СМС
	delete_all_sms(); // попытка обойти глюк модема
	
	
	while(1)
	{
		if(reset_command_accepted) // перезапускаемся, если пришла СМС команда на перезапуск
			reset_mcu(true);
		switch_off_server_if_needed(); // выключаем сервер, если была нажата кнопка или поступил звонок пользователя
		turn_on_server_if_needed(); // включаем сервер, если пришла соответствующая команда по СМС от администратора
		update_server_state_if_needed(); // запрашиваем состояние сервера
		reset_soft_wdt();
		external_communication(); // проверяем подключение по USB
		get_sms(); // чиаем принятые СМС
		incoming_call_processing(); // обрабатываем входящие звонки
		power_control();
		reset_if_needed_by_schedule(); // перезапуск микроконтроллера по графику из настроек
		test_sms_channel_if_needed(); // тест SMS канала связи по графику из настроек
		while(is_queue_not_empty())
			get_message_from_mdm(); // смотрим входной буфер UART
#if(DEBUG==0)
		_SLEEP();
#endif
	}
}











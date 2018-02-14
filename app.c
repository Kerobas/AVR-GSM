
#include "main.h"

#define STATUS_OK      1

Ulong errors_from_reset=0;

char error_code1 = 0;
char error_code2 = 0;

static char json_status;
static char json_response;

static char* get_json_val(char *ptr, __flash const char *str_p);
static void process_data_from_server(char *ptr);

char update_server_state(void)
{
	char net_buf[161];
	char rez;
	char *ptr;
	static char last_state[2] = {SERVER_STATE_UNKNOWN, SERVER_STATE_UNKNOWN};
	
	ptr = net_buf;
	ptr += sprintf_P(ptr, PSTR("GET /status HTTP/1.1\r\nHost: %s:%u\r\n"), config.domen, config.port);
	if(config.token[0])
		ptr += sprintf_P(ptr, PSTR("Authorization: Bearer %s\r\n"), config.token);
	ptr += sprintf_P(ptr, PSTR("\r\n"));
	rez = send_str_to_server(net_buf, config.domen, config.port, &process_data_from_server);
	if(rez==true)
	{
		server_state = last_state[0] = json_response; // SERVER_STATE_UP, SERVER_STATE_DOWN, SERVER_STATE_UNKNOWN
		if(server_state != SERVER_STATE_UNKNOWN)
			rez = true;
		else
			rez = false;
	}
	else
	{
		// в случае одно-двух	кратного сбоя связи оставляем предыдущее состояние сервера
		if((last_state[0] == SERVER_STATE_UNKNOWN) && (last_state[1] == SERVER_STATE_UNKNOWN))
			server_state = SERVER_STATE_UNKNOWN;
		last_state[1] = last_state[0];
		last_state[0] = SERVER_STATE_UNKNOWN;
	}
	return rez;
}

//*******************************************************************************************************************

char send_command_to_server(char command)
{
	char net_buf[161];
	char rez, next_state;
	char *ptr;
	
	ptr = net_buf;
	switch(command)
	{
		case SERVER_COMMAND_DOWN:
			ptr += sprintf_P(ptr, PSTR("GET /down HTTP/1.1\r\nHost: %s:%u\r\n"), config.domen, config.port);
			next_state = SERVER_STATE_DOWN;
			break;
		case SERVER_COMMAND_UP:
			ptr += sprintf_P(ptr, PSTR("GET /up HTTP/1.1\r\nHost: %s:%u\r\n"), config.domen, config.port);
			next_state = SERVER_STATE_UP;
			break;
		default:
			return false;
	}
	if(config.token[0])
		ptr += sprintf_P(ptr, PSTR("Authorization: Bearer %s\r\n"), config.token);
	ptr += sprintf_P(ptr, PSTR("\r\n"));
	rez = send_str_to_server(net_buf, config.domen, config.port, &process_data_from_server);
	if(rez==true)
	{
		if(json_status == STATUS_OK)
		{
			server_state = next_state;
			return true;
		}
	}
	return false;
}

//*******************************************************************************************************************

char is_net_configured(void)
{
	if(config.domen[0] && config.port && apn_set_up_ok)
		return true;
	else
		return false;
}

//*******************************************************************************************************************

void switch_off_server_if_needed(void)
{
	short time_stamp;
	
	if(switch_off_from_button || switch_off_from_call)
	{
		if(switch_off_from_call)
			beep_ms(10);
		time_stamp = get_time_s() + 900; // пытаемся отправить команду на выключение в течение 15 минут (900 секунд)
		while(send_command_to_server(SERVER_COMMAND_DOWN) == false)
		{
			if((get_time_s() - time_stamp) > 0) // если за 15 минут не удалось отправить команду, то перезапускаем систему
			{
				if(switch_off_from_call)
				{
					if(send_sms_p(PSTR("No connection with server."), phone_of_incomong_call))
						send_report_to_developer_p(PSTR("Turn off server from user call. Fail. Device is resetting")); // вторую СМС отправляем только в случае успеха первой
					sprintf_P(config.last_event, PSTR("Turn off server from user call %s. Fail."), phone_of_incomong_call);
					set_val(time_from_event_s, 0);
				}
				else if(switch_off_from_button)
				{
					sprintf_P(config.last_event, PSTR("Turn off server from push button. Fail."));
					set_val(time_from_event_s, 0);
					send_report_to_developer_p(PSTR("Turn off server from button push. Fail. Device is resetting"));
				}
				eeprom_save_config();
				reset_mcu();
			}
			delay_s(config.interval_of_test_s); // пауза между попытками. Мельтешить нет смысла.
		}
		delay_ms(5000);
		if(switch_off_from_call)
		{
			incoming_call = call_from_user = switch_off_from_call = false;
			send_sms_p(PSTR("Server received command to shut down."), phone_of_incomong_call);
			sprintf_P(config.last_event, PSTR("Turn off server from user call %s. Success."), phone_of_incomong_call);
			set_val(time_from_event_s, 0);
		}
		else if(switch_off_from_button)
		{
			switch_off_from_button = false;
			sprintf_P(config.last_event, PSTR("Turn off server from button push. Success."));
			set_val(time_from_event_s, 0);
		}
		eeprom_save_config();
	}
}

//*******************************************************************************************************************

void turn_on_server_if_needed(void)
{
	short time_stamp;
	
	if(command_to_wake_up_server == true)
	{
		time_stamp = get_time_s() + 900; // пытаемся отправить команду на включение в течение 15 минут (900 секунд)
		while(send_command_to_server(SERVER_COMMAND_UP) != true)
		{
			if((get_time_s() - time_stamp) > 0) // если за 15 минут не удалось отправить команду, то перезапускаем систему
			{
				if(send_sms_p(PSTR("No connection with server."), sms_rec_phone_number))
					send_report_to_developer_p(PSTR("Fail to turn on server from admin SMS. Device is resetting.")); // вторую СМС отправляем только в случае успеха первой
				sprintf_P(config.last_event, PSTR("Turn on server from admin SMS %s. Fail."), sms_rec_phone_number);
				set_val(time_from_event_s, 0);
				eeprom_save_config();
				reset_mcu();
			}
			delay_s(config.interval_of_test_s); // пауза между попытками. Мельтешить нет смысла.
		}
		delay_ms(1000);
		command_to_wake_up_server = false;
		send_sms_p(PSTR("Server received command to wake up."), sms_rec_phone_number);
		sprintf_P(config.last_event, PSTR("Turn on server from admin SMS %s. Success."), sms_rec_phone_number);
		set_val(time_from_event_s, 0);
		eeprom_save_config();
	}
}

//*******************************************************************************************************************

void update_server_state_if_needed(void)
{
	static char first = true;
	static short time_of_last_tcp_test_s;
	static Uchar count_of_errors = 0;
	static Ushort count_of_tests = 0;
	char rez;
	
	if(first)
	{
		first = false;
		time_of_last_tcp_test_s = get_time_s() - config.interval_of_test_s + 5;
	}
	
	if((get_time_s() - time_of_last_tcp_test_s) > config.interval_of_test_s)
	{
		mdm_get_signal_strength();
		if(count_of_errors < MAX_COUNT_OF_ERRORS)
		{
			rez = update_server_state();
			if(rez == true)
			{
				count_of_errors = 0;
				count_of_tests = 0;
			}
			else if(is_net_configured()) // инкрементируем счетчик ошибок только если сетевые настройки были сконфигуроированы
			{
				count_of_errors++;
				errors_from_reset++;
				error_code2 = rez;
			}
		}
		else
		{
			count_of_tests++;
			if(count_of_tests >= (short)((3600UL*20UL)/120UL))
				reset_mcu();
			rez = test_gprs_connection();
			if(rez==false)
				count_of_errors++;
			else
				count_of_errors = 0;
			if(count_of_errors >= (MAX_COUNT_OF_ERRORS+2))
			{
				send_report_to_developer_p(PSTR("No Internet connection. Device is resetting."));
				#if(DEBUG==0)
				reset_mcu();
				#endif
			}
		}
		time_of_last_tcp_test_s = get_time_s();
	}
}

//*******************************************************************************************************************

char test_gprs_connection(void)
{
	char net_buf[161]; // ответ кладется в этот же буфер
	char rez;
	
	sprintf_P(net_buf, PSTR("http")); // отправляем не важно что
	rez = send_str_to_server(net_buf, config.test_domen, 80, 0);
	if(rez==true)
		return true;
	else
		return false;
}

//*******************************************************************************************************************

static char* get_json_val(char *ptr, __flash const char *str_p)
{
	Uchar i;
	
	ptr = strstr_P(ptr, str_p);
	if(ptr)
	{
		ptr += strlen_P(str_p);
		for(i=0;i<100;i++) // ищем двоеточие, пропускаем пробелы
		{
			if((ptr[i]==0)||(i==99))
				return false;
			if((ptr[i]==' ')||(ptr[i]=='	'))
				continue;
			if(ptr[i]==':')
			{
				ptr += i+1;
				break;
			}
			return false;
		}
		for(i=0;i<100;i++) // ищем кавычку, пропускаем пробелы
		{
			if((ptr[i]==0)||(i==99))
				return false;
			if((ptr[i]==' ')||(ptr[i]=='	'))
				continue;
			if(ptr[i]=='\"')
			{
				ptr += i;
				break;
			}
			return false;
		}
		return ptr;
	}
	return false;
}

//*******************************************************************************************************************

static void process_data_from_server(char *ptr)
{
	json_status = json_response = 0;
	ptr = strchr(ptr, '{');
	if(ptr)
	{
		ptr = get_json_val(ptr, PSTR("\"status\""));
		if(ptr)
		{
			if(memcmp_P(ptr, PSTR("\"ok\""), 4) == 0)
			{
				json_status = STATUS_OK;
				ptr = get_json_val(ptr, PSTR("\"response\""));
				if(ptr)
				{
					if(memcmp_P(ptr, PSTR("\"up\""), 4) == 0)
						json_response = SERVER_STATE_UP;
					else if(memcmp_P(ptr, PSTR("\"down\""), 6) == 0)
						json_response = SERVER_STATE_DOWN;
				}
			}
		}
	}
}

//*******************************************************************************************************************
// информирование разработчика о неполадке
char send_report_to_developer_p(__flash const char *str)
{
	if((config.developer_phone[0][0] == '+') && (config.reports_en))
	{
		send_sms_p(str, &config.developer_phone[0][0]);
		config.reports_en--; // выключаем автоинформирование разработчикаи. Если надо, разработчик включит еще раз.
		EEPROM_save_report_to_developer();
		return true;
	}
	return false;
}















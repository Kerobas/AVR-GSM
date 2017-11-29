
#include "main.h"

#define NET_BUF_SIZE	   512
#define APN_LIST_SIZE      5
#define PIN_CODE           "4801"

static char mdm_data[NET_BUF_SIZE];
char sms_rec_phone_number[13];
char phone_of_incomong_call[13];
static unsigned long operator_number;
	
char registered_in_gsm_network = false;
static Uchar unread_sms = true; // 
char call_from_user = false;
char incoming_call = false;
char server_state = SERVER_STATE_UNKNOWN;
char switch_off_from_call = false;
char apn_set_up_ok = false;
	
char registration_status=0;
Ushort TCP_data_len;
char *TCP_data_ptr;

typedef struct  
{
	unsigned long operator_number;
	char apn[APN_MAX_LEN+1];
	char uname[UNAME_MAX_LEN+1];
	char pwd[PWD_MAX_LEN+1];
} apn_list_t;


__flash static const apn_list_t apn_list[APN_LIST_SIZE] = 
{
	{25001,"internet.mts.ru","mts","mts"},
	{25002,"internet","gdata","gdata"},
	{25020,"internet.tele2.ru","tele2",""},
	{25099,"internet.beeline.ru","beeline","beeline"},
	{25011,"internet.yota","yota",""}
};

//*******************************************************************************************************************

char mdm_setup_apn(void)
{
	Uchar i, rez, n;
	char apn[APN_MAX_LEN+1];
	char uname[UNAME_MAX_LEN+1];
	char pwd[PWD_MAX_LEN+1];
	
	
	if((config.operator_number == operator_number) && operator_number && config.apn[0] && config.uname[0])
	{
		strcpy(apn, config.apn);
		strcpy(uname, config.uname);
		strcpy(pwd, config.pwd);
	}
	else
	{
		for(n=0;n<APN_LIST_SIZE;n++) // ищем оператора в списке
		{
			if(operator_number == apn_list[n].operator_number)
				break;
		}
		if(n == APN_LIST_SIZE)
			return (false-1); // неизвестный оператор
		
		strcpy_P(apn, apn_list[n].apn);
		strcpy_P(uname, apn_list[n].uname);
		strcpy_P(pwd, apn_list[n].pwd);
	}
	
	for(i=0;i<3;i++)
	{
		delay_ms(100);
		sprintf_P(mdm_data, PSTR("AT+CIPCSGP=1,\"%s\",\"%s\",\"%s\"\r"), apn, uname, pwd);
		uart_send_str(mdm_data);
		rez = mdm_wait_ok_ms(5000);
		if(rez==true)
			break;
	}
	if(rez == false)
		return false;
	else
		apn_set_up_ok = true;
	return rez;
}

//*******************************************************************************************************************

char* gsm_wait_for_string(unsigned short time_to_wait_ms)
{
	short time_stamp = get_time_ms() + time_to_wait_ms;
	
	while(get_message_from_mdm()==false)
	{
		if((get_time_ms() - time_stamp) > 0)
			return false;
	}
	return mdm_data;	
}

//*******************************************************************************************************************

void wait_the_end_of_flow_from_mdm_ms(unsigned short time_to_wait_ms)
{
	short time_stamp = get_time_ms() + time_to_wait_ms;
	volatile char dummy;
	
	while(1)
	{
		if((get_time_ms() - time_stamp) > 0) // интервал после последнего принятого байта
			return;
		if(is_queue_not_empty())
		{
			dummy = get_byte_from_queue();
			dummy = dummy;
			time_stamp = get_time_ms() + time_to_wait_ms; 
		}
	}
}

//*******************************************************************************************************************

char* gsm_poll_for_string(void)
{
	static Uchar state=0;
	static Ushort i;
	static short time_stamp;
	static Ushort finish_item;
	Uchar ch;
	
	if((get_time_ms() - time_stamp) > 10)
		state = 0;
	if(is_queue_not_empty())
	{
		time_stamp = get_time_ms();
		ch = get_byte_from_queue();
		switch(state)
		{
		case 0:
			if((ch != 0) && (ch != '\r') && (ch != '\n'))
			{
				mdm_data[0] = ch;
				i = 1;
				state = 3;
				if(ch == '>')
				{
					mdm_data[1] = 0;
					state = 0;
					return mdm_data;
				}
			}
		break;
		case 3:
			mdm_data[i] = ch;
			i = (i+1)&(NET_BUF_SIZE-1);
			if(i>=2)
			{
				if((mdm_data[i-2] == '\r')&&(mdm_data[i-1] == '\n'))
				{
					mdm_data[i-2] = 0;
					state = 0;
					return mdm_data;
				}
			}
			if(i==0) // было принято 512 байт, переполнение входного буфера
				state = 0;
			if(i==5)
			{
				if(memcmp_P(mdm_data, PSTR("+IPD"), 4) == 0) // данные из TCP соединения
					state = 4;
			}
			break;
		case 4:
			mdm_data[i] = ch;
			i = (i+1)&(NET_BUF_SIZE-1);
			if(i>8)
			{
				state = 0;
				break;
			}
			if(ch==':')
			{
				if(isdigit(mdm_data[4]))
				{
					Ulong tempUL = strtoul(&mdm_data[4], 0, 10);
					if((tempUL+i) <= (NET_BUF_SIZE-1))
					{
						TCP_data_len = tempUL;
						finish_item = TCP_data_len + i;
						TCP_data_ptr = &mdm_data[i];
						state = 5;
						if(finish_item > (NET_BUF_SIZE-1))
							state = 0;
					}
					else
						state = 0;
				}
				else
					state = 0;
			}
			break;
		case 5:
			mdm_data[i] = ch; // собственно данные из TCP соединения
			i = (i+1)&(NET_BUF_SIZE-1);
			if(i>=finish_item)
			{
				state = 0;
				return mdm_data;
			}
			if(i==0) // было принято 512 байт, переполнение входного буфера
				state = 0;
			break;
		}
	}
	return 0;
}

//*******************************************************************************************************************

void gsm_mdm_power_up_down_seq(void)
{
	set_pwr_key_as_output();
	set_pwr_key(0);
	delay_ms(1500);
	set_pwr_key_as_input();
}

//*******************************************************************************************************************

char gsm_mdm_inter_pin(void)
{
	char rez;
	char *str;
	
	delay_ms(500);
	uart_send_str_p(PSTR("AT+CPIN?\r"));
	str = gsm_wait_for_string(5000);
	if(str == false)
		return false;
	if(strstr_P(str, PSTR("ERROR"))) 
		return false;
	if(strstr_P(str, PSTR("READY"))) // пин вводить не надо
		return true;
	if(strstr_P(str, PSTR("SIM PIN"))) // предложено ввести пин
	{
		char pin[5];
		mdm_wait_ok_ms(5000);
		delay_ms(500);
		sprintf_P(pin, PSTR(PIN_CODE));
		sprintf_P(mdm_data, PSTR("AT+CPIN=\"%s\"\r"), pin);
		uart_send_str(mdm_data);
		rez = mdm_wait_ok_ms(5000);
		return rez;
	}
	return false;
}

//*******************************************************************************************************************

char gsm_mdm_init(void)
{
	Uchar i;
	char rez;
	short time_stamp = get_time_s() + 600; // на инициализацию модема отводим 600 секунд
	
	while(1)
	{
		if((get_time_s() - time_stamp) > 0)
			return false;
		// если модем был включен, то сначала выключаем. Если выключен, то включаем
		gsm_mdm_power_up_down_seq();
		// ставим тупо задержку, т.к. сигнал STATUS при наличии внешнего питания всегда равен 1. Особенность SIM300
		delay_s(15);
		reset_uart_queue();
		
		uart_send_str_p(PSTR("ATE0\r")); // выключем эхо. Заодно проверяем, что модем включен. Если выключен, то начинаем заново.
		if(mdm_wait_ok_ms(1000) == false)
			continue;
		
		for(i=0;i<3;i++)
		{
			delay_ms(1000);
			rez = gsm_mdm_inter_pin();
			if(rez==true)
				break;
		}
		if(rez==false)
			continue;
		
		for(i=0;i<3;i++)
		{
			delay_ms(1000);
			uart_send_str_p(PSTR("AT+CMGF=1\r")); // Включить текстовый формат СМС
			rez = mdm_wait_ok_ms(5000);
			if(rez==true)
				break;
		}
		if(rez==false)
			continue;
		
		for(i=0;i<3;i++)
		{
			delay_ms(1000);
			uart_send_str_p(PSTR("AT+CNMI=0,1,0,0,0\r")); // режим приема СМС
			rez = mdm_wait_ok_ms(5000);
			if(rez==true)
				break;
		}
		if(rez==false)
			continue;
		
		for(i=0;i<3;i++)
		{
			delay_ms(1000);
			uart_send_str_p(PSTR("AT+CLIP=1\r")); // включить асинхронное информирование о входящих звонках
			rez = mdm_wait_ok_ms(5000);
			if(rez==true)
				break;
		}
		if(rez==false)
			continue;
		
		for(i=0;i<3;i++)
		{
			delay_ms(1000);
			uart_send_str_p(PSTR("AT+CGATT=1\r")); // enable GPRS
			rez = mdm_wait_ok_ms(5000);
			if(rez==true)
				break;
		}
		if(rez==false)
			continue;
		
		for(i=0;i<3;i++)
		{
			delay_ms(1000);
			uart_send_str_p(PSTR("AT+CIPHEAD=1\r")); // header information field will added before the data from remote server
			rez = mdm_wait_ok_ms(5000);
			if(rez==true)
				break;
		}
		if(rez==false)
			continue;
		
		return true;
	}
}

//*******************************************************************************************************************

char hang_up_call(void)
{
	char i, rez;
	
	for(i=0;i<3;i++)
	{
		delay_ms(100);
		uart_send_str_p(PSTR("ATH\r"));
		rez = mdm_wait_ok_ms(5000);
		if(rez)
			break;
	}
	return rez;
}

//*******************************************************************************************************************

char mdm_wait_str(short time_to_wait_ms)
{
	short time_stamp = get_time_ms() + time_to_wait_ms;
	
	while(get_message_from_mdm()==false)
	{
		if((get_time_ms() - time_stamp) > 0)
			return false;
	}
	return true;
}

//*******************************************************************************************************************

char mdm_wait_sms_header_ms(short time_to_wait_ms)
{
	short time_stamp = get_time_ms() + time_to_wait_ms;
	char *ptr;
	
	while(1)
	{
		while(get_message_from_mdm()==false)
		{
			if((get_time_ms() - time_stamp) > 0)
				return false;
		}
		ptr = strstr_P(mdm_data, PSTR("+CMGL:"));
		if(ptr)
			return true;
		ptr = strstr_P(mdm_data, PSTR("ERROR"));
		if(ptr)
			return false;
	}
}

//*******************************************************************************************************************

char mdm_wait_prompt_s(char time_to_wait_s)
{
	short time_stamp = get_time_s() + time_to_wait_s;
	char *ptr;
	
	while(1)
	{
		while(get_message_from_mdm()==false)
		{
			if((get_time_s() - time_stamp) > 0)
				return false;
		}
		
		if(mdm_data[0] == '>')
			return true;
		
		ptr = strstr_P(mdm_data, PSTR("ERROR"));
		if(ptr)
			return false;
	}
}

//*******************************************************************************************************************

char mdm_wait_ok_s(char time_to_wait_s)
{
	short time_stamp = get_time_s() + time_to_wait_s;
	char *ptr;
	
	while(1)
	{
		while(get_message_from_mdm()==false)
		{
			if((get_time_s() - time_stamp) > 0)
				return false;
		}
		ptr = strstr_P(mdm_data, PSTR("OK"));
		if(ptr)
			return true;
		ptr = strstr_P(mdm_data, PSTR("ERROR"));
		if(ptr)
			return false;
	}
}

//*******************************************************************************************************************

char mdm_wait_ok_ms(unsigned short time_to_wait_ms)
{
	short time_stamp = get_time_ms() + time_to_wait_ms;
	char *ptr;
	
	while(1)
	{
		while(get_message_from_mdm()==false)
		{
			if((get_time_ms() - time_stamp) > 0)
				return false;
		}
		ptr = strstr_P(mdm_data, PSTR("OK"));
		if(ptr)	
			return true;
		ptr = strstr_P(mdm_data, PSTR("ERROR"));
		if(ptr)
			return false;
	}
}

//*******************************************************************************************************************

char wait_connect_ok_s(char time_to_wait_s)
{
	short time_stamp = get_time_s() + time_to_wait_s;
	char *ptr;
	
	while(1)
	{
		while(get_message_from_mdm()==false)
		{
			if((get_time_s() - time_stamp) > 0)
				return false;
		}
		ptr = strstr_P(mdm_data, PSTR("CONNECT OK"));
		if(ptr)
			return true;
		ptr = strstr_P(mdm_data, PSTR("CONNECT FAIL"));
		if(ptr)
			return false;
	}
}

//*******************************************************************************************************************

char wait_send_ok_s(char time_to_wait_s)
{
	short time_stamp = get_time_s() + time_to_wait_s;
	char *ptr;
	
	while(1)
	{
		while(get_message_from_mdm()==false)
		{
			if((get_time_s() - time_stamp) > 0)
				return false;
		}
		ptr = strstr_P(mdm_data, PSTR("SEND OK"));
		if(ptr)
			return true;
		ptr = strstr_P(mdm_data, PSTR("SEND FAIL"));
		if(ptr)
			return false;
	}
}

//*******************************************************************************************************************

char mdm_wait_data_from_tcp_s(char time_to_wait_s)
{
	short time_stamp = get_time_s() + time_to_wait_s;
	
	while(1)
	{
		while(get_message_from_mdm()==false)
		{
			if((get_time_s() - time_stamp) > 0)
				return false;
		}
		if(memcmp_P(mdm_data, PSTR("+IPD"), 4) == 0)
			return true;
	}
}

//*******************************************************************************************************************

char wait_message_from_mdm_ms(short time_to_wait_ms)
{
	short time_stamp = get_time_ms() + time_to_wait_ms;
	
	while(get_message_from_mdm()==false)
	{
		if((get_time_ms() - time_stamp) > 0)
			return false;
	}
	return true;
}

//*******************************************************************************************************************

char mdm_get_operator_name(void)
{
	short time_stamp = get_time_s() + 20; // отводим 20 секунд на получение кода оператора
	char res;
	char *ptr;
	Ulong tempUL;
	
	while(1)
	{
		if((get_time_s() - time_stamp) > 0)
			return false;
		delay_ms(100);
		uart_send_str_p(PSTR("AT+COPS=3,2\r"));
		res = mdm_wait_ok_ms(5000);
		if(res != true)
			continue;
		delay_ms(100);
		uart_send_str_p(PSTR("AT+COPS?\r"));
		while(1)
		{
			res = wait_message_from_mdm_ms(5000);
			if(res != true)
				break;
			ptr = strstr_P(mdm_data, PSTR("+COPS:"));
			if(ptr == false)
				continue;
			ptr = strchr(ptr, '\"');
			if(ptr == false)
				continue;
			tempUL = strtoul(++ptr, 0, 10);
			if(tempUL)
			{
				operator_number = tempUL;
				return true;
			}
			else
				return false;
		}
	}
	return false;
}

//*******************************************************************************************************************

char mdm_wait_registration_status_ms(short time_to_wait_ms)
{
	short time_stamp = get_time_ms() + time_to_wait_ms;
	char *ptr;
	
	while(1)
	{
		while(get_message_from_mdm()==false)
		{
			if((get_time_ms() - time_stamp) > 0)
				return false;
		}
		ptr = strstr_P(mdm_data, PSTR("+CREG:")); // статус регистрации в сети
		if(ptr)
		{
			ptr = strchr(ptr, ',');
			if(ptr)
			{
				Ulong n = strtoul(++ptr, 0, 10);
				if((n==1)||(n==5)) // регистрация в домашней сети, или роуминге
					return true;
				else
					return false;
			}
		}
	}
}

//*******************************************************************************************************************

char mdm_is_registered(void)
{
	char rez;
	
	delay_ms(100);
	uart_send_str_p(PSTR("AT+CREG?\r"));
	rez = mdm_wait_registration_status_ms(5000);
	if(rez == true)
	{
		registered_in_gsm_network = true;
		return true;
	}
	else
	{
		registered_in_gsm_network = false;
		return false;
	}
}

//*******************************************************************************************************************

char mdm_wait_registration_s(short time_to_wait_s)
{
	short time_stamp = get_time_s() + time_to_wait_s;
	char rez;
	
	while(1)
	{
		delay_ms(1000);
		uart_send_str_p(PSTR("AT+CREG?\r"));
		rez = mdm_wait_registration_status_ms(5000);
		if(rez == true)
			return true;
		if((get_time_s() - time_stamp) > 0)
			return false;
	}
}

//*******************************************************************************************************************

Uchar mdm_get_battery_level(void)
{
	char *ptr;
	char rez;
	Ulong level;
	
	delay_ms(100);
	uart_send_str_p(PSTR("AT+CBC\r"));
	while(1)
	{
		rez = mdm_wait_str(5000);
		if(rez==false)
			return 0xFF;
		ptr = strstr_P(mdm_data, PSTR("+CBC:"));
		if(ptr==false)
			continue;
		ptr = strchr(ptr, ':');
		if(!ptr)
			return 0xFF;
		ptr = strchr(ptr, ',');
		if(!ptr)
			return 0xFF;
		ptr++;
		if(isdigit(*ptr) == false)
			return 0xFF;
		level = strtoul(ptr, 0, 10);
		if(level<=0xFF)
			return (Uchar)level;
		else
			return 0xFF;
	}
}

//*******************************************************************************************************************

Ushort get_battery_voltage(void)
{
	char rez;
	char *ptr;
	Ulong voltage;
	
	delay_ms(100);
	uart_send_str_p(PSTR("AT+CBC\r"));
	while(1)
	{
		rez = mdm_wait_str(5000);
		if(rez==false)
			return 0xFFFF;
		ptr = strstr_P(mdm_data, PSTR("+CBC:"));
		if(ptr==false)
			continue;
		ptr = strchr(ptr, ':');
		if(!ptr)
			return 0xFFFF;
		ptr = strchr(ptr, ',');
		if(!ptr)
			return 0xFFFF;
		ptr = strchr(ptr, ',');
		if(!ptr)
			return 0xFFFF;
		ptr++;
		if(isdigit(*ptr) == false)
			return 0xFFFF;
		voltage = strtoul(ptr, 0, 10);
		if(voltage<=0xFFFF)
			return (Ushort)voltage;
		else
			return 0xFFFF;
	}
}

//*******************************************************************************************************************

char send_sms(char *str, char *phone)
{
	char rez;
	
	if(mdm_is_registered() == false)
		return false;
	delay_ms(100);
	sprintf_P(mdm_data, PSTR("AT+CMGS=\"%s\"\r"), phone);
	uart_send_str(mdm_data);
	if(mdm_wait_prompt_s(20) == false)
		return false;
	uart_send_str(str);
	uart_send_byte(0x1A);
	rez = mdm_wait_ok_s(11);
	return rez;
}

//*******************************************************************************************************************

char send_sms_p(__flash const char *str, char *phone)
{
	char rez;
	
	if(mdm_is_registered() == false)
		return false;
	delay_ms(100);
	sprintf_P(mdm_data, PSTR("AT+CMGS=\"%s\"\r"), phone);
	uart_send_str(mdm_data);
	if(mdm_wait_prompt_s(20) == false)
		return false;
	uart_send_str_p(str);
	uart_send_byte(0x1A);
	rez = mdm_wait_ok_s(11);
	return rez;
}

//*******************************************************************************************************************

char get_message_from_mdm(void)
{
	char *str;
	char *ptr;
	
	str = gsm_poll_for_string();
	if(str)
	{
		// асинхронные сообщения обрабатываем прямо здесь
		if(strstr_P(str, PSTR("+CMTI"))) // пришла асинхронная индикация о принятой СМСке
		{
			unread_sms = true;
		}
		else if(strstr_P(str, PSTR("+CLIP:")))   // пришла асинхронная индикация о звонке
		{
			ptr = strchr(mdm_data, ':');
			ptr = strchr(ptr, '+');
			if(find_phone_in_phone_list(ptr, USER_LIST)) // ищем телефон в списке юзеров
			{
				call_from_user = true;
				memcpy(phone_of_incomong_call, ptr, 12); // сохраняем номер телефона, с которого произошел звонок
			}
			incoming_call = true; // вызов будет сброшен в основном цикле программы
		}
		return true;
	}
	return false;
}


//*******************************************************************************************************************
// проверка строки, содержащей телефонный номер. 
char check_phone_string(char *ptr)
{
	char i;
	
	if(*ptr++ != '+')
		return false;
	for(i=0;i<11;i++)
	{
		if(!isdigit(*ptr++))
			return false;
	}
	return true;
}

//*******************************************************************************************************************

char get_sms(void)
{
	char rez, i;
	char *ptr;
	Ulong index;
	
	if(unread_sms == false)
		return false;
	
	delay_ms(100);
	uart_send_str_p(PSTR("AT+CMGL=\"ALL\"\r")); // запрашиваем список всех СМСок
	while(1)
	{
		rez = mdm_wait_sms_header_ms(5000);
		if(rez==true)
		{
			ptr = strchr(mdm_data, ':');
			if(ptr)
			{
				for(i=0;i<5;i++)
				{
					if(isdigit(*++ptr))
						break;
				}
				if(i==5)
					continue;
				index = strtoul(ptr, 0, 10);
				if(index>0xFFFF)
					continue;
			}
			else
				continue;
			ptr = strchr(ptr, ','); // ищем первую запятую в строке
			if(ptr)
			{
				ptr = strchr(ptr, ','); // ищем вторую запятую в строке
				if(ptr)
				{
					ptr = strchr(ptr, '+');
					if(ptr)
					{
						if(check_phone_string(ptr))
						{
							memcpy(sms_rec_phone_number, ptr, 12);
							sms_rec_phone_number[12] = 0;
						}
						else
							continue;
					}
				}
				else
					continue;
			}
			else
				continue;
			rez = mdm_wait_str(5000);
			if(rez==true)
			{
				wait_the_end_of_flow_from_mdm_ms(1000); // ждем окончания потока данных от модема
				process_sms_body(mdm_data);
				for(i=0;i<3;i++)
				{
					delay_ms(100);
					sprintf_P(mdm_data, PSTR("AT+CMGD=%d\r"), (Ushort)index); // удаляем обработанную СМСку
					uart_send_str(mdm_data);
					rez = mdm_wait_ok_ms(5000);
					if(rez == true)
						break;
				}
				delay_ms(100);
				return true;
			}
			else
			{
				delete_all_sms(); // попытка обойти глюк модема
				unread_sms = false;
				return false;
			}
		}
		else
		{
			delete_all_sms(); // попытка обойти глюк модема
			unread_sms = false;
			return false;
		}
	}
}

//*******************************************************************************************************************

char delete_all_sms(void)
{
	char rez;
	delay_ms(100);
	uart_send_str_p(PSTR("AT+CMGDA=\"DEL ALL\"\r"));
	rez = mdm_wait_ok_s(10);
	return rez;
}

//*******************************************************************************************************************

char send_str_to_server(char *str, char *domen, Ushort port, char break_connection, void (*tcp_data_processing)(char *ptr))
{
	char rez, i;
	char *ptr;
	short time_stamp;
	static Uchar count_of_errors=0;
	static short time_of_first_error;
	
	if((domen[0]==0)||(port==0))
		return false;
	
	if(!mdm_is_registered())
		return 19;
	
	for(i=0;i<3;i++)
	{
		delay_ms(100);
		uart_send_str_p(PSTR("AT+CIPSTATUS\r"));
		rez = mdm_wait_ok_ms(3000);
		if(rez == true)
			break;
	}
	if(rez == false)
		return 2;
	rez = wait_message_from_mdm_ms(3000);
	if(rez==false)
		return 3;
	
	if(strstr_P(mdm_data, PSTR("IP CONFIG")) || strstr_P(mdm_data, PSTR("IP IND")))
		return 12;
	
	if(strstr_P(mdm_data, PSTR("CONNECTING")))
		return 20;
	
	if(strstr_P(mdm_data, PSTR("IP INITIAL")))
	{
		// стартуем задачу IP соединения
		for(i=0;i<3;i++)
		{
			delay_ms(100);
			uart_send_str_p(PSTR("AT+CSTT\r"));
			rez = mdm_wait_ok_ms(6000);
			if(rez == true)
				break;
		}
		
		time_stamp = get_time_s() + 10; // даем 10 секунд на запуск задачи	
		while(1)
		{
			delay_ms(1000);
			for(i=0;i<3;i++)
			{
				delay_ms(100);
				uart_send_str_p(PSTR("AT+CIPSTATUS\r"));
				rez = mdm_wait_ok_ms(3000);
				if(rez == true)
					break;
			}
			if(rez == false)
				return 4;
			rez = wait_message_from_mdm_ms(3000);
			if(rez==false)
				return 5;
			if(strstr_P(mdm_data, PSTR("IP START")))
				break;
			if((get_time_s() - time_stamp) > 0)
				return 6;
		}
	}
	
	if(strstr_P(mdm_data, PSTR("IP START")))
	{
		// даем команду на установку GPRS соединения
		for(i=0;i<3;i++)
		{
			delay_ms(100);
			uart_send_str_p(PSTR("AT+CIICR\r"));
			rez = mdm_wait_ok_ms(6000);
			if(rez == true)
				break;
		}
		if(rez == false)
			return 7;
		
		time_stamp = get_time_s() + 10; // даем 10 секунд на установку соединения	
		while(1)
		{
			delay_ms(1000);
			for(i=0;i<3;i++)
			{
				delay_ms(100);
				uart_send_str_p(PSTR("AT+CIPSTATUS\r"));
				rez = mdm_wait_ok_ms(5000);
				if(rez == true)
					break;
			}
			if(rez == false)
				return 8;
			rez = wait_message_from_mdm_ms(5000);
			if(rez==false)
				return 9;
			ptr = strstr_P(mdm_data, PSTR("IP GPRSACT"));
			if(ptr)
				break;
			if((get_time_s() - time_stamp) > 0)
				return 10;
		}
	}
	
	if(strstr_P(mdm_data, PSTR("IP GPRSACT")))
	{
		for(i=0;i<3;i++)
		{
			rez = false;
			delay_ms(100);
			uart_send_str_p(PSTR("AT+CIFSR\r")); // запрашиваем локальный IP (без этого SIM300 не работает)
			rez = wait_message_from_mdm_ms(5000);
			if(rez == true)
				break;
		}
		if(rez == false)
			return 11;
	}
	
	if(strstr_P(mdm_data, PSTR("CONNECT OK")) == false) // нет текущего соединения
	{
		for(i=0;i<3;i++)
		{
			delay_ms(100);
			if(isdigit(config.domen[0]))
				uart_send_str_p(PSTR("AT+CDNSORIP=0\r")); // IP адрес сервера вводится напрямую числом xxx.xxx.xxx.xxx
			else
				uart_send_str_p(PSTR("AT+CDNSORIP=1\r")); // адрес сервера вводится в виде домена 
			rez = mdm_wait_ok_ms(5000);
			if(rez == true)
				break;
		}
		if(rez == false)
			return 13;
	
		for(i=0;i<3;i++)
		{
			delay_ms(100);
			sprintf_P(mdm_data, PSTR("AT+CIPSTART=\"TCP\",\"%s\",\"%d\"\r"), domen, port); // стартуем TCP соединение
			uart_send_str(mdm_data);
			rez = mdm_wait_ok_ms(5000);
			if(rez == true)
				break;
		}
		if(rez == false)
			return 14;
	
		rez = wait_connect_ok_s(10);
		if(rez == false)
		{
			rez = 15;
			goto exit;
		}
	}
	
	delay_ms(100);
	uart_send_str_p(PSTR("AT+CIPSEND\r")); // запрашиваем предложение от модема для отправки данных >
	rez = mdm_wait_prompt_s(5);
	
	if(rez == true)
	{
		uart_send_str(str);
		uart_send_byte(0x1A);
		rez = wait_send_ok_s(10);
		if(rez == true)
		{
			rez = mdm_wait_data_from_tcp_s(15);
			if(rez==true)
			{
				if(tcp_data_processing)
					tcp_data_processing(TCP_data_ptr);
				rez = true;
			}
			else
				rez = 16;
		}
		else
			rez = 17;
	}
	else
		rez = 18;
	

exit:
	if(rez!=true)
	{
		if(count_of_errors < 255)
			count_of_errors++;
		if(count_of_errors==1)
			time_of_first_error = get_time_s();
		if((get_time_s()-time_of_first_error) >= 120)
		{
			count_of_errors = 0;
			delay_ms(100);
			uart_send_str_p(PSTR("AT+CIPSHUT\r")); // Disconnect wireless connection
			mdm_wait_ok_ms(5000);
		}
		else
		{
			delay_ms(100);
			uart_send_str_p(PSTR("AT+CIPCLOSE\r")); // закрываем TCP соединение
			mdm_wait_ok_s(10);
		}
	}
	else
	{
		count_of_errors = 0;
		if(break_connection)
		{
			delay_ms(100);
			uart_send_str_p(PSTR("AT+CIPCLOSE\r")); // закрываем TCP соединение
			mdm_wait_ok_s(10);
		}
	}
	
	return rez;
}

//*******************************************************************************************************************

void close_connection(void)
{
	delay_ms(100);
	uart_send_str_p(PSTR("AT+CIPCLOSE\r")); // закрываем TCP соединение
	mdm_wait_ok_s(10);
}

//*******************************************************************************************************************

















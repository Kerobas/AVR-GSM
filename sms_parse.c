

#include "main.h"


#define SIZE_OF_SMS_TEXT_BUF 320
#define MAX_SMS_LENGTH       160

__flash static const char help_text[MAX_SMS_LENGTH+1] = {
"serverup;\r\nget:set:\r\nuserphones;adminphones;adminmode;serverstate;version;ip;domen;port;token;longpress;lastevent;voltage;netparams;id;"};

static char* set_phones(char *phones, char *dest, char max_num);

char command_to_wake_up_server = false;
char reset_command_accepted = false;


char find_phone_in_phone_list(char *phone, char list)
{
	Uchar i;
	
	if(list==ADMIN_LIST)
	{
		for(i=0;i<TOTAL_ADMIN_NUMBER;i++)
		{
			if(config.admin_phone[i][0] == '+')
				if(memcmp(phone, &config.admin_phone[i][0], 12) == 0)
					return true; // телефон найден в списке админов
		}
	}
	
	if(list==USER_LIST)
	{
		for(i=0;i<TOTAL_USER_NUMBER;i++)
		{
			if(config.user_phone[i][0] == '+')
				if(memcmp(phone, &config.user_phone[i][0], 12) == 0)
					return true; // телефон найден в списке юзеров
		}
	}
	
	if((list==DEVELOPER_LIST)||(list==ADMIN_LIST))
	{
		for(i=0;i<TOTAL_DEVELOPER_NUMBER;i++)
		{
			if(config.developer_phone[i][0] == '+')
			if(memcmp(phone, &config.developer_phone[i][0], 12) == 0)
				return true; // телефон найден в списке разработчиков
		}
	}
	return false;
}

//*******************************************************************************************************************

void process_sms_body(char *ptr)
{
	Uchar i, error;
	
	beep_ms(10);
	
	if(memcmp_P(ptr, PSTR("set:"), 4) == 0)
	{
		if((find_phone_in_phone_list(sms_rec_phone_number, ADMIN_LIST) == false) && (config.admin_mode == true))
		{
			send_sms_p(PSTR("Access denied"), sms_rec_phone_number);
			return;
		}
		ptr += 4;
		i=0;
		error = false;
		while(ptr && *ptr)
		{
			ptr = set_param(ptr);
			if(ptr)
				i++;
			else
				error = true;
		}
		if(i && !error)
		{
			eeprom_save_config();
			send_sms_p(PSTR("ok"), sms_rec_phone_number);
		}
		else
		{
			eeprom_read_config(); // возвращаем всё в зад
			send_sms_p(PSTR("error"), sms_rec_phone_number);
		}
	}
	

	else if(memcmp_P(ptr, PSTR("get:"), 4) == 0)
	{
		char sms_text[SIZE_OF_SMS_TEXT_BUF];
		if((find_phone_in_phone_list(sms_rec_phone_number, ADMIN_LIST) == false) && (config.admin_mode == true))
		{
			send_sms_p(PSTR("Access denied"), sms_rec_phone_number);
			return;
		}
		ptr += 4;
		sms_text[0] = 0; // терминируем строку
		i=0;
		error = false;
		while(ptr && *ptr)
		{
			ptr = get_param(ptr, &sms_text[strlen(sms_text)]);
			if(ptr)
				i++;
			else
			{
				error = 1;
				break;
			}
			if(strlen(sms_text) > MAX_SMS_LENGTH)
			{
				error = 2;
				break;
			}
		}
		if(i && !error)
			send_sms(sms_text, sms_rec_phone_number);
		else if(error==1)
			send_sms_p(PSTR("error"), sms_rec_phone_number);
		else if(error==2)
			send_sms_p(PSTR("resulting SMS text is too long"), sms_rec_phone_number);
	}
	
	else if(memcmp_P(ptr, PSTR("resetadminmode!!"), 16) == 0)
	{
		config.admin_mode = false;
		eeprom_save_config();
		send_sms_p(PSTR("ok"), sms_rec_phone_number);
	}
	
	else if(memcmp_P(ptr, PSTR("serverup;"), 9) == 0)
	{
		if(find_phone_in_phone_list(sms_rec_phone_number, ADMIN_LIST) == false)
		{
			send_sms_p(PSTR("Access denied"), sms_rec_phone_number);
			return;
		}
		if(server_state == SERVER_STATE_UP)
			send_sms_p(PSTR("Server is already up."), sms_rec_phone_number);
		else
		{
			if(is_net_configured())
			{
				send_sms_p(PSTR("Command accepted. Success will be reported."), sms_rec_phone_number);
				command_to_wake_up_server = true;
			}
			else
				send_sms_p(PSTR("Network params not configured."), sms_rec_phone_number);
		}
	}
	
	else if(memcmp_P(ptr, PSTR("resetmcu;"), 9) == 0)
	{
		if((find_phone_in_phone_list(sms_rec_phone_number, ADMIN_LIST) == false) && (config.admin_mode == true))
		{
			send_sms_p(PSTR("Access denied"), sms_rec_phone_number);
			return;
		}
		send_sms_p(PSTR("Reset command accepted."), sms_rec_phone_number);
		reset_command_accepted = true;
	}
	
	else if(memcmp_P(ptr, PSTR("SMS channel test"), 16) == 0)
	{
		if(memcmp(sms_rec_phone_number, &config.my_phone[0], 12) == 0)
			time_of_last_sms_test_m = get_time_m();
	}
	
	else if(memcmp_P(ptr, PSTR("help;"), 5) == 0)
	{
		if((find_phone_in_phone_list(sms_rec_phone_number, ADMIN_LIST) == false) && (config.admin_mode == true))
		{
			send_sms_p(PSTR("Access denied"), sms_rec_phone_number);
			return;
		}
		send_sms_p(help_text, sms_rec_phone_number);
	}
}


//*******************************************************************************************************************

char* get_param(char *str, char *sms_text)
{
	
	if(memcmp_P(str, PSTR("time;"), 5) == 0)
	{
		Ulong time;
		Ushort d, h, m, s;
		str += 5;
		time = get_val(time_from_start_s);
		d = time/(3600UL*24UL);
		time -= d*(3600UL*24UL);
		h = time/3600UL;
		time -= h*3600UL;
		m = time/60;
		s = time - m*60;
		sprintf_P(sms_text, PSTR("time=%ud%uh%um%us;"), d, h, m, s);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("errors;"), 7) == 0)
	{
		str += 7;
		sprintf_P(sms_text, PSTR("errors=%lu;"), errors_from_reset);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("id;"), 3) == 0)
	{
		str += 3;
		sprintf_P(sms_text, PSTR("id=%s;"), config.id);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("version;"), 8) == 0)
	{
		str += 8;
		sms_text += sprintf_P(sms_text, PSTR("version="));
		sms_text += sprintf_P(sms_text, PSTR(SOFTWARE_VERSION));
		sms_text += sprintf_P(sms_text, PSTR(";"));
		return str;
	}
	
	else if(memcmp_P(str, PSTR("netparams;"), 10) == 0)
	{
		str += 10;
		if(isdigit(config.domen[0]))
			sms_text += sprintf_P(sms_text, PSTR("ip"));
		else
			sms_text += sprintf_P(sms_text, PSTR("domen"));
		sprintf_P(sms_text, PSTR("=%s;port=%u;token=%s;"), config.domen, config.port, config.token);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("ip;"), 3) == 0)
	{
		str += 3;
		if(isdigit(config.domen[0]))
			sms_text += sprintf_P(sms_text, PSTR("ip"));
		else
			sms_text += sprintf_P(sms_text, PSTR("domen"));
		sprintf_P(sms_text, PSTR("=%s;"), config.domen);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("domen;"), 6) == 0)
	{
		str += 6;
		if(isdigit(config.domen[0]))
			sms_text += sprintf_P(sms_text, PSTR("ip"));
		else
			sms_text += sprintf_P(sms_text, PSTR("domen"));
		sprintf_P(sms_text, PSTR("=%s;"), config.domen);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("testdomen;"), 10) == 0)
	{
		str += 10;
		sprintf_P(sms_text, PSTR("testdomen=%s;"), config.test_domen);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("port;"), 5) == 0)
	{
		str += 5;
		sprintf_P(sms_text, PSTR("port=%u;"), config.port);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("rstcount;"), 9) == 0)
	{
		str += 9;
		sprintf_P(sms_text, PSTR("rstcount=%lu;mdm_rstcount=%lu;"), config.reset_count, config.unable_to_turn_on_modem);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("operatornumber;"), 15) == 0)
	{
		str += 15;
		sprintf_P(sms_text, PSTR("operatornumber=%lu;"), config.operator_number);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("apn;"), 4) == 0)
	{
		str += 4;
		sprintf_P(sms_text, PSTR("apn=%s;"), config.apn);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("pwd;"), 4) == 0)
	{
		str += 4;
		sprintf_P(sms_text, PSTR("pwd=%s;"), config.pwd);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("uname;"), 6) == 0)
	{
		str += 6;
		sprintf_P(sms_text, PSTR("uname=%s;"), config.uname);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("token;"), 6) == 0)
	{
		str += 6;
		sprintf_P(sms_text, PSTR("token=%s;"), config.token);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("adminmode;"), 10) == 0)
	{
		str+=10;
		sprintf_P(sms_text, PSTR("adminmode=%d;"), config.admin_mode);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("userphones;"), 11) == 0)
	{
		Uchar n, i;
		
		str += 11;
		sms_text += sprintf_P(sms_text, PSTR("userphones="));
		n = 0;
		for(i=0;i<TOTAL_USER_NUMBER;i++)
		{
			if(config.user_phone[i][0] != '+')
				break;
			sms_text += sprintf_P(sms_text, PSTR("%s,"), &config.user_phone[i][0]);
			n++; // количество напечатанных телефонов
		}
		if(n)
			sms_text--;
		sprintf_P(sms_text, PSTR(";"));
		return(str);
	}
	
	else if(memcmp_P(str, PSTR("adminphones;"), 12) == 0)
	{
		Uchar n, i;
		
		str += 12;
		sms_text += sprintf_P(sms_text, PSTR("adminphones="));
		n=0;
		for(i=0;i<TOTAL_ADMIN_NUMBER;i++)
		{
			if(config.admin_phone[i][0] != '+')
				break;
			sms_text += sprintf_P(sms_text, PSTR("%s,"), &config.admin_phone[i][0]);
			n++; // количество напечатанных телефонов
		}
		if(n)
			sms_text--;
		sprintf_P(sms_text, PSTR(";"));
		return str;
	}
	
	else if(memcmp_P(str, PSTR("developerphones;"), 16) == 0)
	{
		Uchar n, i;
		
		str += 16;
		sms_text += sprintf_P(sms_text, PSTR("developerphones="));
		n=0;
		for(i=0;i<TOTAL_DEVELOPER_NUMBER;i++)
		{
			if(config.developer_phone[i][0] != '+')
				break;
			sms_text += sprintf_P(sms_text, PSTR("%s,"), &config.developer_phone[i][0]);
			n++; // количество напечатанных телефонов
		}
		if(n)
			sms_text--;
		sprintf_P(sms_text, PSTR(";"));
		return str;
	}
	
	else if(memcmp_P(str, PSTR("myphone;"), 8) == 0)
	{
		str += 8;
		sprintf_P(sms_text, PSTR("myphone=%s;"), config.my_phone);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("serverstate;"), 12) == 0)
	{
		str += 12;
		sms_text += sprintf_P(sms_text, PSTR("serverstate="));
		switch(server_state)
		{
			case SERVER_STATE_UP:
				sms_text += sprintf_P(sms_text, PSTR("UP;"));
				break;
			case SERVER_STATE_DOWN:
				sms_text += sprintf_P(sms_text, PSTR("DOWN;"));
				break;
			case SERVER_STATE_UNKNOWN:
			default:
				sms_text += sprintf_P(sms_text, PSTR("UNKNOWN;"));
				break;
		}
		return str;
	}
	
	else if(memcmp_P(str, PSTR("voltage;"), 8) == 0)
	{
		float voltage;
		
		str += 8;
		voltage = get_pwr_voltage_f();
		sms_text += sprintf_P(sms_text, PSTR("voltage=%.1fV"), voltage);
		if(voltage > 14.5)
			sms_text += sprintf_P(sms_text, PSTR("(overvoltage);"));
		else if(is_external_pwr())
			sms_text += sprintf_P(sms_text, PSTR("(ok);"));
		else
		{
			Ulong time;
			Ushort d, h, m, s;
			time = get_val(time_without_power);
			d = time/(3600UL*24UL);
			time -= d*(3600UL*24UL);
			h = time/3600UL;
			time -= h*3600UL;
			m = time/60;
			s = time - m*60;
			sms_text += sprintf_P(sms_text, PSTR("(low during %ud%uh%um%us);"), d, h, m, s);
		}
		return str;
	}
	
	else if(memcmp_P(str, PSTR("resetperiod;"), 12) == 0)
	{
		str += 12;
		sprintf_P(sms_text, PSTR("resetperiod=%u;"), config.reset_period_m);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("smsinterval;"), 12) == 0)
	{
		str += 12;
		sprintf_P(sms_text, PSTR("smsinterval=%u;"), config.interval_of_sms_test_m);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("smsresetcount;"), 14) == 0)
	{
		str += 14;
		sprintf_P(sms_text, PSTR("smsresetcount=%u;"), config.sms_reset_count);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("longpress;"), 10) == 0)
	{
		str += 10;
		sprintf_P(sms_text, PSTR("longpress=%u;"), config.long_press_ms);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("testmode;"), 9) == 0)
	{
		str += 9;
		sprintf_P(sms_text, PSTR("testmode=%u;"), config.test_mode);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("voice;"), 6) == 0)
	{
		str += 6;
		sprintf_P(sms_text, PSTR("voice=%u;"), config.debug_voice_enable);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("report;"), 7) == 0)
	{
		str += 7;
		sprintf_P(sms_text, PSTR("report=%u;"), config.reports_en);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("relay;"), 6) == 0)
	{
		str += 6;
		sprintf_P(sms_text, PSTR("relay=%u;"), config.relay_enable);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("lastevent;"), 10) == 0)
	{
		char d,h,m,s;
		Ulong t = get_val(time_from_event_s);
		str += 10;
		if((t < MAX_TIME_FROM_EVENT) && (t != 0))
		{
			d = t/(3600UL*24UL);
			t -= (Ulong)d*(3600UL*24UL);
			h = t/(3600UL);
			t -= (Ulong)h*3600UL;
			m = t/60;
			s = t - m*60;
			sprintf_P(sms_text, PSTR("lastevent=%s %dd%dh%dm%ds ago;"), config.last_event, d, h, m, s);
		}
		else
			sprintf_P(sms_text, PSTR("no events during past month;"));
		return str;
	}
	
	else if(memcmp_P(str, PSTR("signal;"), 7) == 0)
	{
		str += 7;
		sprintf_P(sms_text, PSTR("signal=%ddBm;"), signal_strength);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("answer;"), 7) == 0)
	{
		str += 7;
		sprintf_P(sms_text, PSTR("answer=%ds;"), config.time_to_wait_answer_s);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("interval;"), 9) == 0)
	{
		str += 9;
		sprintf_P(sms_text, PSTR("period=%ds;"), config.interval_of_test_s);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("error;"), 6) == 0)
	{
		str += 6;
		sprintf_P(sms_text, PSTR("error1=%d,error2=%d;"), error_code1, error_code2);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("prompt;"), 7) == 0)
	{
		str += 7;
		sprintf_P(sms_text, PSTR("prompt=%d;"), config.time_to_wait_prompt_s);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("connect;"), 8) == 0)
	{
		str += 8;
		sprintf_P(sms_text, PSTR("connect=%d;"), config.time_to_wait_connect_s);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("send;"), 5) == 0)
	{
		str += 5;
		sprintf_P(sms_text, PSTR("send=%d;"), config.time_to_wait_send_ok_s);
		return str;
	}
	
	return false;
}

//*******************************************************************************************************************

char* set_param(char *ptr)
{
	Uchar i;
	
	if(memcmp_P(ptr, PSTR("domen="), 6) == 0)
	{
		char dot_found = false;
		ptr+=6;
		if((memcmp_P(ptr, PSTR("WWW."), 4) != 0) && (memcmp_P(ptr, PSTR("www."), 4) != 0))
			return false;
		for(i=4;i<(DOMEN_LENGTH_STR-1);i++)
		{
			if(ptr[i] == ' ')
				return false;
			if(ptr[i] == '.')
				dot_found = true;
			if(ptr[i] == ';')
				break;
		}
		if(i==(DOMEN_LENGTH_STR-1))
			return false;
		if(dot_found == false)
			return false;
		memcpy(config.domen, ptr, i);
		config.domen[i] = 0;
		return &ptr[i+1];
	}
	
	if(memcmp_P(ptr, PSTR("testdomen="), 10) == 0)
	{
		char dot_found = false;
		ptr+=10;
		if((memcmp_P(ptr, PSTR("WWW."), 4) != 0) && (memcmp_P(ptr, PSTR("www."), 4) != 0))
			return false;
		for(i=4;i<(DOMEN_LENGTH_STR-1);i++)
		{
			if(ptr[i] == ' ')
				return false;
			if(ptr[i] == '.')
				dot_found = true;
			if(ptr[i] == ';')
				break;
		}
		if(i==(DOMEN_LENGTH_STR-1))
			return false;
		if(dot_found == false)
			return false;
		memcpy(config.test_domen, ptr, i);
		config.test_domen[i] = 0;
		return &ptr[i+1];
	}
	
	if(memcmp_P(ptr, PSTR("port="), 5) == 0)
	{
		Ulong port;
		ptr+=5;
		if(isdigit(*ptr) == false)
			return false;
		port = strtoul(ptr, &ptr, 10);
		if(port > 0xFFFF)
			return false;
		if(*ptr != ';')
			return false;
		config.port = port;
		ptr++;
		return ptr;
	}
	
	if(memcmp_P(ptr, PSTR("longpress="), 10) == 0)
	{
		Ulong press;
		ptr+=10;
		if(isdigit(*ptr) == false)
		return false;
		press = strtoul(ptr, &ptr, 10);
		if(press > 10000)
		return false;
		if(*ptr != ';')
		return false;
		config.long_press_ms = press;
		ptr++;
		return ptr;
	}
	
	if(memcmp_P(ptr, PSTR("resetperiod="), 12) == 0)
	{
		Ulong period;
		ptr+=12;
		if(isdigit(*ptr) == false)
		return false;
		period = strtoul(ptr, &ptr, 10);
		if( (period > 60000) || ((period<60)&&(period!=0)) )
		return false;
		if(*ptr != ';')
		return false;
		config.reset_period_m = period;
		ptr++;
		return ptr;
	}
	
	if(memcmp_P(ptr, PSTR("smsinterval="), 12) == 0)
	{
		Ulong interval;
		ptr+=12;
		if(isdigit(*ptr) == false)
			return false;
		interval = strtoul(ptr, &ptr, 10);
		if( (interval>60000) || ((interval<60)&&(interval!=0)) )
			return false;
		if(*ptr != ';')
			return false;
		config.interval_of_sms_test_m = interval;
		ptr++;
		return ptr;
	}
	
	if(memcmp_P(ptr, PSTR("operatornumber="), 15) == 0)
	{
		Ulong n;
		ptr+=15;
		if(isdigit(*ptr) == false)
			return false;
		n = strtoul(ptr, &ptr, 10);
		if(n > 99999)
			return false;
		if(*ptr != ';')
			return false;
		config.operator_number = n;
		ptr++;
		return ptr;
	}
	
	if(memcmp_P(ptr, PSTR("apn="), 4) == 0)
	{
		char *p;
		ptr+=4;
		p = strchr(ptr, ';');
		if(p)
		{
			if((Ushort)(p-ptr) <= APN_MAX_LEN)
			{
				memcpy(config.apn, ptr, (Uchar)(p-ptr));
				config.apn[(Uchar)(p-ptr)] = 0; // терминируем строку нулём
				ptr = p + 1;
				return ptr;
			}
			else
				return false;
		}
		else
			return false;
	}
	
	if(memcmp_P(ptr, PSTR("uname="), 6) == 0)
	{
		char *p;
		ptr+=6;
		p = strchr(ptr, ';');
		if(p)
		{
			if((Ushort)(p-ptr) <= UNAME_MAX_LEN)
			{
				memcpy(config.uname, ptr, (Uchar)(p-ptr));
				config.uname[(Uchar)(p-ptr)] = 0; // терминируем строку нулём
				ptr = p + 1;
				return ptr;
			}
			else
				return false;
		}
		else
			return false;
	}
	
	if(memcmp_P(ptr, PSTR("pwd="), 4) == 0)
	{
		char *p;
		ptr+=4;
		p = strchr(ptr, ';');
		if(p)
		{
			if((Ushort)(p-ptr) <= PWD_MAX_LEN)
			{
				memcpy(config.pwd, ptr, (Uchar)(p-ptr));
				config.pwd[(Uchar)(p-ptr)] = 0; // терминируем строку нулём
				ptr = p + 1;
				return ptr;
			}
			else
				return false;
		}
		else
			return false;
	}
	
	if(memcmp_P(ptr, PSTR("adminmode="), 10) == 0)
	{
		Ushort mode;
		ptr+=10;
		if(*ptr == '0')
			mode = 0;
		else if(*ptr == '1')
			mode = 1;
		else
			return false;
		if(*++ptr != ';')
			return false;
		config.admin_mode = mode;
		ptr++;
		return ptr;
	}
	
	if(memcmp_P(ptr, PSTR("ip="), 3) == 0)
	{
		Uchar ip[4];
		Ulong tempUL;
		ptr+=3;
		for(i=0;i<4;i++)
		{
			if(isdigit(*ptr) == false)
				return false;
			tempUL = strtoul(ptr, &ptr, 10);
			if(tempUL > 255)
				return false;
			ip[i] = (Uchar)tempUL;
			if((i < 3) && (*ptr != '.'))
				return false;
			ptr++;
		}
		if(*(ptr-1) != ';')
			return false;
		sprintf_P(config.domen, PSTR("%u.%u.%u.%u"), ip[0], ip[1], ip[2], ip[3]);
		return ptr;
	}
	
	if(memcmp_P(ptr, PSTR("userphones="), 11) == 0)
	{
		ptr+=11;
		ptr = set_phones(ptr, &config.user_phone[0][0], TOTAL_USER_NUMBER);
		return ptr;
	}
	
	if(memcmp_P(ptr, PSTR("adminphones="), 12) == 0)
	{
		ptr+=12;
		ptr = set_phones(ptr, &config.admin_phone[0][0], TOTAL_ADMIN_NUMBER);
		return ptr;
	}
	
	if(memcmp_P(ptr, PSTR("developerphones="), 16) == 0)
	{
		ptr+=16;
		ptr = set_phones(ptr, &config.developer_phone[0][0], TOTAL_DEVELOPER_NUMBER);
		return ptr;
	}
	
	if(memcmp_P(ptr, PSTR("myphone="), 8) == 0)
	{
		ptr+=8;
		ptr = set_phones(ptr, &config.my_phone[0], 1);
		return ptr;
	}
	
	if(memcmp_P(ptr, PSTR("token="), 6) == 0)
	{
		char *p;
		ptr+=6;
		p = strchr(ptr, ';');
		if(p)
		{
			if((Ushort)(p-ptr) <= TOKEN_MAX_LENGTH)
			{
				memcpy(config.token, ptr, (Uchar)(p-ptr));
				config.token[(Uchar)(p-ptr)] = 0; // терминируем строку нулём
				ptr = p + 1;
				return ptr;
			}
			else
				return false;
		}
		else
			return false;
	}
	
	if(memcmp_P(ptr, PSTR("id="), 3) == 0)
	{
		char *p;
		ptr+=3;
		p = strchr(ptr, ';');
		if(p)
		{
			if((Ushort)(p-ptr) <= ID_MAX_LENGTH)
			{
				memcpy(config.id, ptr, (Uchar)(p-ptr));
				config.id[(Uchar)(p-ptr)] = 0; // терминируем строку нулём
				ptr = p + 1;
				return ptr;
			}
			else
				return false;
		}
		else
			return false;
	}
	
	if(memcmp_P(ptr, PSTR("testmode="), 9) == 0)
	{
		Ushort mode;
		
		if(!find_phone_in_phone_list(sms_rec_phone_number, DEVELOPER_LIST))
			return false;
		ptr+=9;
		if(*ptr == '0')
			mode = 0;
		else if(*ptr == '1')
			mode = 1;
		else
			return false;
		if(*++ptr != ';')
			return false;
		config.test_mode = mode;
		ptr++;
		return ptr;
	}
	
	if(memcmp_P(ptr, PSTR("voice="), 6) == 0)
	{
		Ushort mode;
		
		if(!find_phone_in_phone_list(sms_rec_phone_number, DEVELOPER_LIST))
			return false;
		ptr+=6;
		if(*ptr == '0')
			mode = 0;
		else if(*ptr == '1')
			mode = 1;
		else
			return false;
		if(*++ptr != ';')
			return false;
		config.debug_voice_enable = mode;
		ptr++;
		return ptr;
	}
	
	if(memcmp_P(ptr, PSTR("report="), 7) == 0)
	{
		Ulong val;
		
		if(!find_phone_in_phone_list(sms_rec_phone_number, DEVELOPER_LIST))
			return false;
		ptr+=7;
		if(isdigit(*ptr) == false)
			return false;
		val = strtoul(ptr, &ptr, 10);
		if(val > 0xFF)
			return false;
		if(*ptr != ';')
			return false;
		config.reports_en = val;
		ptr++;
		return ptr;
	}
	
	if(memcmp_P(ptr, PSTR("interval="), 9) == 0)
	{
		Ulong val;
		
		if(!find_phone_in_phone_list(sms_rec_phone_number, DEVELOPER_LIST))
			return false;
		ptr+=9;
		if(isdigit(*ptr) == false)
			return false;
		val = strtoul(ptr, &ptr, 10);
		if(val > 0xFF)
			return false;
		if(*ptr != ';')
			return false;
		config.interval_of_test_s = val;
		ptr++;
		return ptr;
	}
	
	if(memcmp_P(ptr, PSTR("answer="), 7) == 0)
	{
		Ulong val;
		
		if(!find_phone_in_phone_list(sms_rec_phone_number, DEVELOPER_LIST))
			return false;
		ptr+=7;
		if(isdigit(*ptr) == false)
			return false;
		val = strtoul(ptr, &ptr, 10);
		if(val > 0xFF)
			return false;
		if(*ptr != ';')
			return false;
		config.time_to_wait_answer_s = val;
		ptr++;
		return ptr;
	}
	
	if(memcmp_P(ptr, PSTR("prompt="), 7) == 0)
	{
		Ulong val;
		
		if(!find_phone_in_phone_list(sms_rec_phone_number, DEVELOPER_LIST))
			return false;
		ptr+=7;
		if(isdigit(*ptr) == false)
			return false;
		val = strtoul(ptr, &ptr, 10);
		if(val > 0xFF)
			return false;
		if(*ptr != ';')
			return false;
		config.time_to_wait_prompt_s = val;
		ptr++;
		return ptr;
	}
	
	if(memcmp_P(ptr, PSTR("connect="), 8) == 0)
	{
		Ulong val;
		
		if(!find_phone_in_phone_list(sms_rec_phone_number, DEVELOPER_LIST))
			return false;
		ptr+=8;
		if(isdigit(*ptr) == false)
			return false;
		val = strtoul(ptr, &ptr, 10);
		if(val > 0xFF)
			return false;
		if(*ptr != ';')
			return false;
		config.time_to_wait_connect_s = val;
		ptr++;
		return ptr;
	}
	
	if(memcmp_P(ptr, PSTR("send="), 5) == 0)
	{
		Ulong val;
		
		if(!find_phone_in_phone_list(sms_rec_phone_number, DEVELOPER_LIST))
			return false;
		ptr+=5;
		if(isdigit(*ptr) == false)
			return false;
		val = strtoul(ptr, &ptr, 10);
		if(val > 0xFF)
			return false;
		if(*ptr != ';')
			return false;
		config.time_to_wait_send_ok_s = val;
		ptr++;
			return ptr;
	}
	
	if(memcmp_P(ptr, PSTR("relay="), 6) == 0)
	{
		Ushort relay;
		
		if(!find_phone_in_phone_list(sms_rec_phone_number, DEVELOPER_LIST))
			return false;
		ptr+=6;
		if(*ptr == '0')
			relay = 0;
		else if(*ptr == '1')
			relay = 1;
		else
			return false;
		if(*++ptr != ';')
			return false;
		config.relay_enable = relay;
		ptr++;
		return ptr;
	}
	
	return false;
}

//*******************************************************************************************************************
// принимает список телефонов через запятую, адрес назначения и максимальный размер области назначения
static char* set_phones(char *phones, char *dest, char max_num)
{
	Uchar n, i;
	char *ptr = phones;
	n=0;
	while(check_phone_string(ptr))
	{
		n++;
		if(n > max_num)
			return false;
		ptr+=12;
		if(*ptr == ';')
			break;
		else if(*ptr == ',')
		{
			ptr++;
			continue;
		}
		else
			return false;
	}
	ptr = phones;
	for(i=0;i<n;i++)
	{
		memcpy(&dest[i*13], ptr, 12);
		dest[i*13 + 12] = 0;
		ptr+=13;
	}
	memset(&dest[n*13], 0, 13*(max_num-n));
	return ptr;
}
//*******************************************************************************************************************

void incoming_call_processing(void)
{
	if(incoming_call == false)
		return;
	hang_up_call();
	incoming_call = false;
	if(call_from_user)
	{
		switch(server_state)
		{
		case SERVER_STATE_DOWN:
			send_sms_p(PSTR("Server is already down."), phone_of_incomong_call);
			call_from_user = false;
			break;
		case SERVER_STATE_UNKNOWN:
		case SERVER_STATE_UP:
		default:
			if(is_net_configured())
			{
				switch_off_from_call = true;
				send_sms_p(PSTR("Shutting down server. Success will be reported."), phone_of_incomong_call);
			}
			else
				send_sms_p(PSTR("Network params not configured."), phone_of_incomong_call);
			break;
		}
	}
}

//*******************************************************************************************************************




























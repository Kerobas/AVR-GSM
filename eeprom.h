

#ifndef EEPROM_H_
#define EEPROM_H_

#define TOTAL_USER_NUMBER       5
#define TOTAL_ADMIN_NUMBER      5
#define TOTAL_DEVELOPER_NUMBER  2
#define DOMEN_LENGTH_STR        32
#define TOKEN_MAX_LENGTH        32
#define ID_MAX_LENGTH           64
#define MAX_TIME_FROM_EVENT     (3600UL*24UL*31)
#define APN_MAX_LEN             32
#define UNAME_MAX_LEN           20
#define PWD_MAX_LEN             20

typedef struct{
	char admin_mode;
	Ulong operator_number;
	char apn[APN_MAX_LEN+1];
	char uname[UNAME_MAX_LEN+1];
	char pwd[PWD_MAX_LEN+1];
	char admin_phone[TOTAL_ADMIN_NUMBER][13];
	char user_phone[TOTAL_USER_NUMBER][13];
	char developer_phone[TOTAL_DEVELOPER_NUMBER][13];
	char domen[DOMEN_LENGTH_STR+1];
	char test_domen[DOMEN_LENGTH_STR+1];
	Ushort port;
	char token[TOKEN_MAX_LENGTH+1];
	char id[ID_MAX_LENGTH+1];
	char last_event[100];
	Ulong time_from_event_s;
	Ushort long_press_ms;
	Ulong unable_to_turn_on_modem;
	Ulong reset_count;
	Uchar reports_en;
	char debug_voice_enable;
	char test_mode; // по величине этого параметра определяется чистота EEPROM
	char relay_enable;
	Uchar time_to_wait_answer_s; // тюнинг работы модема
	Uchar interval_of_test_s; // тюнинг работы модема
	Uchar time_to_wait_prompt_s; // тюнинг работы модема
	Uchar time_to_wait_connect_s; // тюнинг работы модема
	Uchar time_to_wait_send_ok_s; // тюнинг работы модема
	char my_phone[13];
	Ushort reset_period_h;
	Ushort sms_reset_count;
	Ushort interval_of_sms_test_h;
} config_t;



extern config_t config;


void EEPROM_write(Ushort address, Uchar data);
Uchar EEPROM_read(Ushort address);
void EEPROM_write_buf(char *buf, Ushort len, Ushort address);
void EEPROM_read_buf(char *buf, Ushort len, Ushort address);
void eeprom_read_config(void);
void eeprom_save_config(void);
void EEPROM_save_reset_count(void);
void EEPROM_save_time_from_event(void);
void EEPROM_save_report_to_developer(void);
void check_rst_source(void);






#endif /* EEPROM_H_ */



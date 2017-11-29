﻿

#ifndef EEPROM_H_
#define EEPROM_H_

#define TOTAL_USER_NUMBER       5
#define TOTAL_ADMIN_NUMBER      5
#define TOTAL_DEVELOPER_NUMBER  2
#define DOMEN_LENGTH_STR        32
#define TOKEN_MAX_LENGTH        32
#define ID_MAX_LENGTH           64
#define MAX_TIME_FROM_EVENT     (3600UL*24UL*7)
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
	char debug_voice_enable;
	char test_mode;
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
void check_rst_source(void);






#endif /* EEPROM_H_ */



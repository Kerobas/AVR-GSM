


#ifndef _GSM_H_
#define _GSM_H_


#define SERVER_STATE_UNKNOWN  0
#define SERVER_STATE_UP       1
#define SERVER_STATE_DOWN     2

#define SERVER_COMMAND_DOWN   0
#define SERVER_COMMAND_UP     1

char mdm_setup_apn(void);
char* gsm_wait_for_string(unsigned short time_to_wait_ms);
char* gsm_poll_for_string(void);
void gsm_mdm_power_up_down_seq(void);
void wait_status_s(char state, unsigned short time_to_wait_s);
char gsm_mdm_inter_pin(void);
char gsm_mdm_init(void);
char hang_up_call(void);
char mdm_wait_prompt_s(char time_to_wait_s);
char mdm_wait_ok_s(char time_to_wait_s);
char mdm_wait_ok_ms(unsigned short time_to_wait_ms);
char wait_connect_ok_s(char time_to_wait_s);
char wait_send_ok_s(char time_to_wait_s);
char mdm_wait_data_from_tcp_s(char time_to_wait_s);
char wait_message_from_mdm_ms(short time_to_wait_ms);
char mdm_get_operator_name(void);
char mdm_wait_registration_s(short time_to_wait_s);
char send_sms(char *str, char *phone);
char send_sms_p(__flash const char *str, char *phone);
char check_phone_string(char *ptr);
char get_message_from_mdm(void);
void put_sms_in_queue(char *str);
char send_str_to_server(char *str, char *domen, Ushort port, char break_connection, void (*tcp_data_processing)(char *ptr));
char mdm_wait_sms_header_ms(short time_to_wait_ms);
char get_sms(void);
char delete_all_sms(void);
Uchar mdm_get_battery_level(void);
Ushort get_battery_voltage(void);


extern char sms_rec_phone_number[];
extern char phone_of_incomong_call[];
extern char server_state;
extern char switch_off_from_call;
extern char call_from_user;
extern char incoming_call;
extern char registered_in_gsm_network;
extern char apn_set_up_ok;

#endif

﻿
#ifndef APP_H_
#define APP_H_

#define USER_LIST					0
#define ADMIN_LIST					1
#define DEVELOPER_LIST				2

#define MAX_COUNT_OF_ERRORS			6


char update_server_state(void);
char send_command_to_server(char command);
char is_net_configured(void);
void switch_off_server_if_needed(void);
void turn_on_server_if_needed(void);
void update_server_state_if_needed(void);
char test_gprs_connection(void);
char send_report_to_developer_p(__flash const char *str);
void test_sms_channel_if_needed(void);
void reset_if_needed_by_schedule(void);

extern Ulong errors_from_reset;
extern char command_to_wake_up_server;
extern char reset_command_accepted;

extern char error_code1;
extern char error_code2;
short time_of_last_sms_test_m;


#endif /* APP_H_ */


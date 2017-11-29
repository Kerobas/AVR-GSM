
#ifndef SMS_PARSE_H_
#define SMS_PARSE_H_





char find_phone_in_phone_list(char *phone, char list);
void process_sms_body(char *str);
char* set_param(char *ptr);
char* get_param(char *str, char *sms_text);
void incoming_call_processing(void);











#endif /* SMS_PARSE_H_ */

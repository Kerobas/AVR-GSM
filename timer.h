

#ifndef TIMER_H_
#define TIMER_H_

void timer0_init(void);
short get_time_ms(void);
short get_time_s(void);
void delay_ms(short delay);
void delay_s(short delay);
void timer1_init(void);
void start_buzzer(void);
void stop_buzzer(void);
void beep_ms(Ushort time_ms);
void reset_soft_wdt(void);
Ulong get_time_from_start(void);
void inc_time_from_last_event(void);
void reset_time_from_last_event(void);
void set_time_from_last_event(Ulong val);
void beep_non_block(Ushort time_to_beep);


extern char switch_off_from_button;
extern Ulong time_from_start_s;
extern Ulong time_from_event_s;




#endif /* TIMER_H_ */

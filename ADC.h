

#ifndef ADC_H_
#define ADC_H_


void ADC_init(void);
void ADC_result_processing(void);
char is_external_pwr(void);
float get_pwr_voltage_f(void);
void check_power(void);
void power_control(void);



 
extern Ulong time_without_power;





#endif /* ADC_H_ */


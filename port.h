

#ifndef PORT_H_
#define PORT_H_


void port_init(void);
void set_pwr_key_as_output(void);
void set_pwr_key_as_input(void);
void set_pwr_key(char val);
char get_status(void);
char Is_usb_connected(void);
char is_main_button_pushed(void);
char SwitchCommChanell_USB_MCU(void);
void external_communication(void);
void led_management(void);
void check_button(void);


#endif /* PORT_H_ */

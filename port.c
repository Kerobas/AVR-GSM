
#include "main.h"

void set_pwr_key_as_output(void)
{
	DDRA |= (1<<2);
}

//*******************************************************************************************************************

void set_pwr_key_as_input(void)
{
	DDRA &= ~(1<<2);
}

//*******************************************************************************************************************

void set_pwr_key(char val)
{
	if(val)
		PORTA |= (1<<2);
	else
		PORTA &= ~(1<<2);
}

//*******************************************************************************************************************

char Is_usb_connected(void)
{
	if(PIND & (1<<5))
		return 1;
	else
		return 0;
}

//*******************************************************************************************************************

void main_button_port_init(void)
{
	DDRD &= ~(1<<3);
	PORTD |= 1<<3;
}

//*******************************************************************************************************************

void led_port_init(void)
{
	PORTB &= ~((1<<7)|(1<<6)|(1<<5));
	DDRB |= (1<<7)|(1<<6)|(1<<5);
}

//*******************************************************************************************************************
// зеленый светодиод
void server_state_led_on(void) 
{
	PORTB |= (1<<5);
}

//*******************************************************************************************************************
// зеленый светодиод
void server_state_led_off(void)
{
	PORTB &= ~(1<<5);
}

//*******************************************************************************************************************
// красный светодиод
void pwr_error_led_on(void)
{
	PORTB |= (1<<6);
}

//*******************************************************************************************************************
// красный светодиод
void pwr_error_led_off(void)
{
	PORTB &= ~(1<<6);
}

//*******************************************************************************************************************
// оранжевый светодиод
void pwr_led_on(void)
{
	PORTB |= (1<<7);
}

//*******************************************************************************************************************
// оранжевый светодиод
void pwr_led_off(void)
{
	PORTB &= ~(1<<7);
}

//*******************************************************************************************************************

char is_main_button_pushed(void)
{
	static char state = false;
	
	if(state==false) // кнопка была не нажата
	{
		if(!(PIND & (1<<3)) && !(PIND & (1<<3)) && !(PIND & (1<<3)))
			state = true;
	}
	else
	{
		if((PIND & (1<<3)) && (PIND & (1<<3)) && (PIND & (1<<3)))
			state = false;
	}
	return state;
}

//*******************************************************************************************************************

char SwitchCommChanell_USB_MCU(void)
{
	static char usbConnectedPrev = -2;
	char usbConnected;

	usbConnected = Is_usb_connected() && Is_usb_connected() && Is_usb_connected();

	if(usbConnected != usbConnectedPrev) {
		if(usbConnected == 1) {
			// deinitialize the UART
			init_uart(0);

			PORTA &= 0x0F;	//RTS,CTS,DTR,DCD - inputs
			DDRA &= 0x0F;

			PORTD &= ~((1<<6)|(1<<1)|(1<<0));   //RI,RTX,DTX- inputs
			DDRD  &= ~((1<<6)|(1<<1)|(1<<0));
			
			// release FT232RL from reset
			PORTB &= ~(1<<2);
			DDRB &= ~(1<<2);
		}
		else
		{
			// hold FT232RL in reset
			PORTB &= ~(1<<2);
			DDRB |= 1<<2;

			PORTA &= ~((1<<7)|(1<<5)); //disable SLEEP mode with RTS=0; DTR =0
			DDRA |=  (1<<7)|(1<<5);

			PORTD &= ~(1<<0);
			DDRD  = (1<<0); // TXD

			init_uart(1);
		}

		usbConnectedPrev = usbConnected;

	}

	return usbConnected;
}

//*******************************************************************************************************************

void external_communication(void)
{
	while(SwitchCommChanell_USB_MCU())
		reset_soft_wdt();
}

//*******************************************************************************************************************

void port_init(void)
{
	PORTB = 0;
	DDRB = 1<<2; // hold FT232RL in reset

	PORTA = 0; //disable SLEEP mode with RTS=0; DTR =0
	DDRA =  (1<<7)|(1<<5);

	PORTD = 0;
	DDRD  = (1<<4)|(1<<1); // TXD, BUZ, LED
	
	main_button_port_init();
	led_port_init();
}

//*******************************************************************************************************************
// управление светодиодами. Вызывается с периодом 10 мс
void led_management(void)
{
	static char led_flash=0;
	
	led_flash++;
	if(led_flash>=75)
		led_flash=0;
	
	// зеленый светодиод
	switch(server_state)
	{
		case SERVER_STATE_UP:
			server_state_led_on();
			break;
		case SERVER_STATE_DOWN:
			if(led_flash == 0)
				server_state_led_on();
			else if(led_flash==8)
				server_state_led_off();
			break;
		case SERVER_STATE_UNKNOWN:
		default:
			server_state_led_off();
			break;
	}
	
	if(is_external_pwr())
	{
		pwr_led_on(); // оранжевый светодиод
		pwr_error_led_off(); // красный светодиод
	}
	else
	{
		pwr_led_off(); // оранжевый светодиод
		if(led_flash==0)
			pwr_error_led_on(); // красный светодиод
		else if(led_flash==8)
			pwr_error_led_off();
	}
}

//*******************************************************************************************************************

// функция вызывается с периодом 10 мс
void check_button(void)
{
	static Ushort press_button_ms = 0;
	static char button_state=false;
	
	if(is_main_button_pushed())
	{
		if(press_button_ms <= config.long_press_ms)
			press_button_ms +=10;
		else if(button_state == false)
		{
			if((server_state != SERVER_STATE_DOWN) && is_net_configured())
				switch_off_from_button = true;
			button_state = true;
			beep_non_block(100);
		}
	}
	else
	{
		press_button_ms = 0;
		button_state = false;
	}
}




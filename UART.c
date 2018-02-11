
#include "main.h"

#define UART_BUF_SIZE 64

static char uart_in_buf[UART_BUF_SIZE];
static Uchar head;
static Uchar tail;

void init_uart(char init)
{
	if(init)
	{
		UCSRA=0x00;
		UCSRB=0x98;
		UCSRC=0x86;
		UBRRH=0x00;
		UBRRL=0x03;
	}
	else // deinit
	{
		UCSRA=0x00;
		UCSRB=0x00;
		UCSRC=0x00;
		UBRRH=0x00;
		UBRRL=0x00;
	}
	head = tail = 0;
}

//*******************************************************************************************************************

// USART Receiver interrupt service routine
ISR(USART_RXC_vect)
{
	char status,data;
	
	status=UCSRA;
	data=UDR;
	if ((status & ((1<<FE) | (1<<PE) | (1<<DOR)))==0)
	{
		uart_in_buf[head] = data;
		head = (head+1)&(UART_BUF_SIZE-1);
	}
}

//*******************************************************************************************************************

char get_byte_from_queue(void)
{
	char data;
	data = uart_in_buf[tail];
	tail = (tail+1)&(UART_BUF_SIZE-1);
	return data;
}

//*******************************************************************************************************************

char is_queue_not_empty(void)
{
	if(head!=tail)
		return true;
	else
		return false;
}

//*******************************************************************************************************************

void uart_send_buf(char* buf, char len)
{
	while(len--)
	{
		while( (UCSRA & (1<<UDRE)) == 0 ); // ждем готовности буферного регистра
		UDR = *(buf++);
	}
}

//*******************************************************************************************************************

void uart_send_str(char *str)
{
	while(*str)
	{
		while( (UCSRA & (1<<UDRE)) == 0 ); // ждем готовности буферного регистра
		UDR = *str++;
	}
}

//*******************************************************************************************************************
// отправить строку из ПЗУ
void uart_send_str_p(__flash const char *str)
{
	while(*str)
	{
		while( (UCSRA & (1<<UDRE)) == 0 ); // ждем готовности буферного регистра
		UDR = *str++;
	}
}

//*******************************************************************************************************************

void uart_send_byte(char data)
{
	while( (UCSRA & (1<<UDRE)) == 0 ); // ждем готовности буферного регистра
	UDR = data;
}

//*******************************************************************************************************************

void reset_uart_queue(void)
{
	tail = head = 0;
}















/*
 * uart_api.h
 */

#ifndef __UART_API_H__
#define __UART_API_H__

//#define _REIEVE_MSG_ 

#define FALSE 0
#define TRUE 1


#define UART_PARNONE	0	/* Defines for setting parity */
#define UART_PARODD		1
#define UART_PAREVEN	2

#define UART_STOP_BITS_1    0
#define UART_STOP_BITS_2    1

#define UART_NO_ERR		0	/* Function call was successful */
#define UART_BAD_CH		-1	/* Invalid communications port channel*/
#define UART_RX_EMPTY	-2	/* Rx buffer is empty, no character available */
#define UART_TX_FULL	-3	/* Tx buffer is full, could not deposit char */
#define UART_TX_EMPTY	-4	/* If the Tx buffer is empty. */

#define FIRST_STATE			0
#define RECIEVED_STATE		1
#define SENDED_STATE		2

extern int recv_status;

/* UART API Function Prototypes */
int user_uart_open (char *device);
void user_uart_close (void);
void user_uart_config(int baud);
int user_uart_write(unsigned char *ubuf, int size);
void *receive_threadst(void *arg);
int user_uart_read(unsigned char *ubuf, int size);
void setSendValue(bool cm,unsigned char x, unsigned char y);
#endif // __UART_API_H__

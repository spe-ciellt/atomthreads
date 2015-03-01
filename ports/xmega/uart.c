/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <joerg@FreeBSD.ORG> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.        Joerg Wunsch
 * ----------------------------------------------------------------------------
 *
 * Stdio demo, UART implementation
 *
 * $Id: uart.c,v 1.1 2005/12/28 21:38:59 joerg_wunsch Exp $
 */

#include <stdint.h>
#include <stdio.h>

#include <avr/io.h>

#include "atom.h"
#include "atommutex.h"
#include "atomport-private.h"
#include "uart.h"

/*
 * Set register-access macros for the UART used across various Xmega devices.
 */
#define USART_PORT  PORTC
#define USART USARTC0

#define USART_RX_PIN_bm PIN2_bm
#define USART_TX_PIN_bm PIN3_bm


/* Local data */

/*
 * Semaphore for single-threaded access to UART device
 */
static ATOM_MUTEX uart_mutex;


/*
 * Initialize the UART to requested baudrate, tx/rx, 8N1.
 */
int
uart_init(uint32_t baudrate)
{
    int status;
    uint8_t bsel;
    int8_t bscale;

    /* 1. Set the TxD pin value high, and optionally the XCK pin low. */
    USART_PORT.OUTSET = (USART_TX_PIN_bm);

    /* 2. Set the TxD and optionally the XCK pin as output. */
    USART_PORT.DIRSET = USART_TX_PIN_bm;
    USART_PORT.DIRCLR = USART_RX_PIN_bm;

    /* 3. Set the baud rate and frame format. */
    /* 9600 */
    /* bsel = 12, bscale = 4; */
    /* 38400 */
    bsel = 12, bscale = 2;
    /* 115200 */
    /* bsel = 131, bscale = -3; */
    /* 230400 */
    /* bsel = 123, bscale = -4; */
    USART.BAUDCTRLA = (bsel & 0xff);
    USART.BAUDCTRLB = (bscale << 4) | (bsel >> 8);

    /* 4. Set mode of operation (enables the XCK pin output in sync mode). */
    USART.CTRLC = (USART_CHSIZE_8BIT_gc | USART_PMODE_DISABLED_gc);

    /* 5. Enable the Transmitter and receiver. */
    USART.CTRLB = (USART_TXEN_bm);


    /* Create a mutex for single-threaded putchar() access */
    if (atomMutexCreate (&uart_mutex) != ATOM_OK) {
        status = -1;
    } else {
        status = 0;
    }

    /* Finished */
    return (status);
}

/*
 * Send character c down the UART Tx, wait until tx holding register
 * is empty.
 */
int
uart_putchar(char c, FILE *stream)
{

  /* Block on private access to the UART */
  if (atomMutexGet(&uart_mutex, 0) == ATOM_OK)
  {
    /* Convert \n to \r\n */
    if (c == '\n')
      uart_putchar('\r', stream);

    /* Wait until the UART is ready then send the character out */
    loop_until_bit_is_set(USART.STATUS, USART_DREIF_bp);
    USART.DATA = c;

    /* Return mutex access */
    atomMutexPut(&uart_mutex);
  }

  return 0;
}

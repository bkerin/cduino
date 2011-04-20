/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <joerg@FreeBSD.ORG> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.        Joerg Wunsch
 * ----------------------------------------------------------------------------
 *
 * General stdiodemo defines
 *
 * $Id: defines.h 2002 2009-06-25 20:21:16Z joerg_wunsch $
 */

/* CPU frequency */
// Defined in Makefile now:
//#define F_CPU 1000000UL

/* UART baud rate */
#define UART_BAUD  9600

/* HD44780 LCD port connections */
#define HD44780_RS C, 6
#define HD44780_RW C, 4
#define HD44780_E  C, 5
/* The data bits have to be in ascending order. */
#define HD44780_D4 C, 0

/* Whether to read the busy flag, or fall back to
   worst-time delays. */
#define USE_BUSY_BIT 1

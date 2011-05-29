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

// F_CPU is supposed to be defined in the Makefile (because that's where
// the other part and programmer specs go).
#ifndef F_CPU
#  error "F_CPU not defined"
#endif

// UART baud rate.
#define UART_BAUD  9600

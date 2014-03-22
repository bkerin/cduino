// Interface to ST Microelectronics LIS331(HH/DLH) solid state accelerometers.
//
// Test driver: accelerometer_test.c    Implementation: accelerometer.h
//
// Some of the underlying code we grabbed from the ST Microelectronics site
// was written for the LIS331DLH, so everything should work fine with that
// device.  We have an LIS331HH, which is almost identical register-wise (see
// the start of lis331dlh_driver.c for a description of the difference).
// This interface will probably work almost unchanged for a number of
// ST Microelectronics accelerometers; the major task when considering a
// different one would be to look through lis331dlh_driver.h and compare
// it to the datasheet for the device you're considering and ensure that
// the registers used are the same.

#ifndef ACCELEROMETER_H
#define ACCELEROMETER_H

#include "lis331dlh_driver.h"

// Initialize the accelerometer.  This must be done first, but once its
// done all the functions from lis331dlh_driver.h can be used.
void
accelerometer_init (void);

#endif // ACCELEROMETER_H

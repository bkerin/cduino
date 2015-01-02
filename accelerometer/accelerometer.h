// Interface to ST Microelectronics LIS331(HH/DLH) solid state accelerometers.
//
// Test driver: accelerometer_test.c    Implementation: accelerometer.c
//
// Only a few features of the underlying lis331dlh_driver_private.h are
// brought out directly in this interface.  See that file for interrupt
// generation, high-pass filtering etc.
//
// Some of the underlying code we grabbed from the ST Microelectronics
// site was written for the LIS331DLH, so everything should work fine
// with that device.  We have an LIS331HH, which is almost identical
// register-wise (see the start of lis331dlh_driver_private.c for a
// description of the difference).  This interface will probably work
// almost unchanged for a number of ST Microelectronics accelerometers;
// the major task when considering a different one would be to look through
// lis331dlh_driver_private.h and compare it to the datasheet for the device
// you're considering and ensure that the registers used are the same.

#ifndef ACCELEROMETER_H
#define ACCELEROMETER_H

#include <inttypes.h>

// WARNING: this header pollutes the namespace with a variety of types with
// fairly short names.  I don't want to go through and make a zillion tiny
// changes to the file, since that would make it hard to track the upstream
// source from which it came.
#include "lis331dlh_driver_private.h"

// Initialize the accelerometer and put it in normal (not power-down mode).
// Other than selecting normal mode instead of power-down mode, all the
// device defaults are used.  This function must be called first, but once
// its done all the functions from lis331dlh_driver_private.h can be used.
void
accelerometer_init (void);

// Power down the accelerometer.  This puts the device fully to sleep.
// For other low-power modes with periodic sampling and interrupts see the
// datasheet and/or underlying lis331dlh_driver_private.h interface.
void
accelerometer_power_down (void);

// Put the accelerometer in fully operational normal mode.  See
// accelerometer_power_down().
void
accelerometer_power_up (void);

typedef enum {
  ACCELEROMETER_FULLSCALE_TYPE_6G  = LIS331HH_FULLSCALE_6,
  ACCELEROMETER_FULLSCALE_TYPE_12G = LIS331HH_FULLSCALE_12,
  ACCELEROMETER_FULLSCALE_TYPE_24G = LIS331HH_FULLSCALE_24,
} accelerometer_fullscale_t;

// Set the full-scale (and corresponding sensitivity) setting.
void
accelerometer_set_fullscale (accelerometer_fullscale_t fs);

typedef enum {
  ACCELEROMETER_DATA_RATE_50HZ   = LIS331DLH_ODR_50Hz,
  ACCELEROMETER_DATA_RATE_100HZ  = LIS331DLH_ODR_100Hz,
  ACCELEROMETER_DATA_RATE_400HZ  = LIS331DLH_ODR_400Hz,
  ACCELEROMETER_DATA_RATE_1000HZ = LIS331DLH_ODR_1000Hz
} accelerometer_data_rate_t;

// Set the output data rate.
void
accelerometer_set_data_rate (accelerometer_data_rate_t dr);

// Block until new acceleration data is ready, then return it in *ax, *ay,
// *az.  FIXME: The exact choice of units is not clear to me, and seems to
// depend on the selected full-scale setting.  You're always supposed to
// get 1g pointing down :).
void
accelerometer_get_accel (int16_t *ax, int16_t *ay, int16_t *az);

#endif // ACCELEROMETER_H

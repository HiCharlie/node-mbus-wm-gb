//------------------------------------------------------------------------------
// Copyright (C) 2011, Robert Johansson, Raditex AB
// All rights reserved.
//
// rSCADA
// http://www.rSCADA.se
// info@rscada.se
//
//------------------------------------------------------------------------------

/**
 * @file   mbus-serial.h
 *
 * @brief  Functions and data structures for sending M-Bus data via RS232.
 *
 */

#ifndef MBUS_SERIAL_H
#define MBUS_SERIAL_H

#ifdef _WIN32
#include "win/termiWin.h"
#else
#include <termios.h>
#endif

#include "mbus.h"
#include "mbus-protocol.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct _mbus_serial_data
{
    char *device;
    speed_t speed;
    struct termios t;
} mbus_serial_data;

int  mbus_serial_connect(mbus_handle *handle);
int  mbus_serial_disconnect(mbus_handle *handle);
int  mbus_serial_send_frame(mbus_handle *handle, mbus_frame *frame);
int  mbus_serial_recv_frame(mbus_handle *handle, mbus_frame *frame);
void mbus_serial_data_init(mbus_handle *handle);
void mbus_serial_data_free(mbus_handle *handle);

char* mbus_serial_get_device(mbus_handle *handle);
int  mbus_serial_set_device(mbus_handle *handle, const char *device);

long mbus_serial_get_baudrate(mbus_handle *handle);
int  mbus_serial_set_baudrate(mbus_handle *handle, long baudrate);

int  mbus_serial_set_format(mbus_handle *handle, int databits, int stopbits, int parity);

#ifdef __cplusplus
}
#endif

#endif /* MBUS_SERIAL_H */

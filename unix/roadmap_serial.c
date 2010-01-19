/* roadmap_serial.c - a module to open/read/close a serial IO device.
 *
 * LICENSE:
 *
 *   Copyright 2005 Paul Fox
 *
 *   This file is part of RoadMap.
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   RoadMap is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with RoadMap; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 * DESCRIPTION:
 *
 *   This module hides the OS specific API to access a serial device.
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>

#include "roadmap.h"
#include "roadmap_serial.h"

/*
 * mapping of system baudrate tokens to bits/sec values.
 */
static struct {
    speed_t token;
    int integer;
} RoadMapSerialBauds[] = {
#ifdef B460800
    {B460800,   460800},
#endif
#ifdef B230400
    {B230400,   230400},
#endif
#ifdef B115200
    {B115200,   115200},
#endif
#ifdef B57600
    {B57600,    57600},
#endif
#ifdef B38400
    {B38400,    38400},
#endif
    {B19200,    19200},
    {B9600,     9600},
    {B4800,     4800},
    {B2400,     2400},
    {B1200,     1200},
    {B600,      600},
    {B300,      300},
    {B110,      110},
};

#define NUMBAUDS (sizeof(RoadMapSerialBauds)/sizeof(RoadMapSerialBauds[0]))

static struct termios RoadMapSerialOldParameters, RoadMapSerialNewParameters;

/*
 * convert a baudrate, given in bits/sec,
 * to an index in the table.
 */
static int roadmap_serial_rate2index(int rate) {
    unsigned int i = 0;
    while (i < NUMBAUDS && rate < RoadMapSerialBauds[i].integer)
        i++;

    /* they're all too fast, return the slowest */
    if (i == NUMBAUDS)
        return NUMBAUDS - 1;

    return i;
}

/*
 * return the actual baudrate corresponding to an index
 * (e.g., for printing)
 */
int roadmap_serial_index2rate(int bindex) {
    return RoadMapSerialBauds[bindex].integer;
}

/*
 * return the token corresponding to an index
 */
speed_t roadmap_serial_index2token(int bindex) {
    return RoadMapSerialBauds[bindex].token;
}

int setup_tty(int tty, int speed) {

    int s;
    struct termios *newsb = &RoadMapSerialNewParameters;
    int bindex = roadmap_serial_rate2index(speed);

    s = tcgetattr(tty, &RoadMapSerialOldParameters);
    if (s < 0) {
            return -1;
    }

    *newsb = RoadMapSerialOldParameters;

#ifndef VDISABLE
# ifdef _POSIX_VDISABLE
#  define VDISABLE _POSIX_VDISABLE
# else
#  define VDISABLE '\0'
# endif
#endif

    newsb->c_oflag = 0; /* no output flags at all */

    newsb->c_lflag = ICANON;

    newsb->c_cflag &= ~PARENB;  /* disable parity, both in and out */
    newsb->c_cflag |= CLOCAL|CS8|CREAD;   /* one stop bit on transmit */
                                            /* no modem control, 8bit chars, */
                                            /* receiver enabled, */

    newsb->c_iflag = IGNBRK | IGNPAR;    /* ignore break, ignore parity errors */

    newsb->c_cc[VEOL] = VDISABLE;
    newsb->c_cc[VEOF] = VDISABLE;
#ifdef  VSWTCH
    newsb->c_cc[VSWTCH] = VDISABLE;
#endif
    newsb->c_cc[VSUSP]  = VDISABLE;
#if defined (VDSUSP) && defined(NCCS) && VDSUSP < NCCS
    newsb->c_cc[VDSUSP]  = VDISABLE;
#endif
    newsb->c_cc[VSTART] = VDISABLE;
    newsb->c_cc[VSTOP]  = VDISABLE;

    s = cfsetospeed (newsb, RoadMapSerialBauds[bindex].token);
    if (s < 0) {
            return -1;
    }
    s = cfsetispeed (newsb, RoadMapSerialBauds[bindex].token);
    if (s < 0) {
            return -1;
    }
    s = tcsetattr(tty, TCSAFLUSH, newsb);
    if (s < 0) {
            return -1;
    }
    s = tcflush(tty, TCIOFLUSH);
    if (s < 0) {
            return -1;
    }

    { char buf[10];
      int n;
      while ((n = read(tty, buf, 10)) > 0)
	fprintf(stderr, "got %d\n", n);
    }
    return 0;
}

RoadMapSerial roadmap_serial_open  (const char *name,
                                    const char *mode,
                                    int speed) {

    int ttyfd;

    ttyfd = open(name, O_RDWR|O_NONBLOCK);
    if (ttyfd < 0) {
       return -1;
    }

    if (setup_tty(ttyfd, speed) < 0) {
       close(ttyfd);
       return -1;
    }

    /* turn off non-blocking */
    fcntl(ttyfd, F_SETFL, 0);

    return (RoadMapSerial)ttyfd;
}

int roadmap_serial_read  (RoadMapSerial serial, void *data, int size) {

    return read  ((int)serial, data, (size_t)size);

}

int roadmap_serial_write (RoadMapSerial serial, const void *data, int length) {

    return read  ((int)serial, (void *)data, (size_t)length);

}

void  roadmap_serial_close (RoadMapSerial serial) {

    if (ROADMAP_SERIAL_IS_VALID(serial)) close(serial);

}


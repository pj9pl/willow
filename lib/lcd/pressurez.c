/* lcd/pressurez.c */

/* Copyright (c) 2024 Peter Welch
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

   * Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
   * Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in
     the documentation and/or other materials provided with the
     distribution.
   * Neither the name of the copyright holders nor the names of
     contributors may be used to endorse or promote products derived
     from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
*/

/* LCD pressure secretary.
 * 
 * This accepts a PRESSURE_NOTIFY message containing an mtype byte and an
 * unsigned long. The mtype byte describes the unsigned long.
 */

#include <avr/io.h>

#include "sys/defs.h"
#include "sys/ioctl.h"
#include "sys/msg.h"
#include "net/twi.h"
#include "lcd/plcd.h"
#include "lcd/lcache.h"
#include "lcd/pressurez.h"

/* I am .. */
#define SELF PRESSUREZ
#define this pressurez

/* The row/col position within the display.
 *     ----------------
 *     xxxxxxx---------
 */
#define XBUFLEN 7             /* '1001.56' */
#define PRESSUREZ_DADDR 64     /* The row/col position within the display. */

typedef enum {
    IDLE = 0,
    ENSLAVED,
    WRITING_DATA
} __attribute__ ((packed)) state_t;

typedef struct {
    state_t state;
    char buf[XBUFLEN];
    uchar_t cnt;
    dbuf_t sm; /* service message */
    union {
        twi_info twi;
        lcache_info lcache;
    } info;
} pressurez_t;

/* I have .. */
static pressurez_t this;

/* I can .. */
PRIVATE void resume(void);
PRIVATE void print_pressure(void);
PRIVATE void bputc(char c);
PRIVATE void printl(long n);
PRIVATE void get_request(void);

PUBLIC uchar_t receive_pressurez(message *m_ptr)
{
    switch (m_ptr->opcode) {
    case REPLY_INFO:
        if (this.state) {
            resume();
        }
        break;

    case INIT:
        {
            uchar_t result = EBUSY;
            if (this.state == IDLE) {
                get_request();
                result = EOK;
            }
            send_REPLY_RESULT(m_ptr->sender, result);
        }
        break;

    default:
        return ENOMSG;
    }
    return EOK;
}

PRIVATE void resume(void)
{
    switch (this.state) {
    case IDLE:
        break;

    case ENSLAVED:
        print_pressure();
        break;

    case WRITING_DATA:
        get_request();
        break;
    }
}

PRIVATE void print_pressure(void)
{
    this.state = WRITING_DATA;
    this.cnt = 0;
    printl((long)this.sm.res);

    this.info.lcache.p = this.buf;
    this.info.lcache.n = this.cnt;
    this.info.lcache.instr = LCD_SET_DDRAM_ADDR | PRESSUREZ_DADDR;
    send_JOB(LCACHE, &this.info.lcache);
}

PRIVATE void bputc(char c)
{
    if (this.cnt < XBUFLEN) {
        this.buf[this.cnt++] = c;
    }
}

PRIVATE void printl(long n)
{
    char s[12];
    long i = 0;
    uchar_t len;

    if (n < 0) {
        bputc('-');
        n = -n;
    }

    do {
        s[i++] = n % 10 + '0';
    } while ((n /= 10) > 0);

    len = i;
    for (; len < XBUFLEN -1; len++) {
        bputc(' ');
    }
    while (--i >= 0) {
        if (i == 1)
          bputc('.');
        bputc(s[i]);
    }
}

PRIVATE void get_request(void)
{
    this.state = ENSLAVED;
    this.sm.taskid = ANY;
    sae2_TWI_GCSR(this.info.twi, PRESSURE_NOTIFY, this.sm);
}

/* end code */

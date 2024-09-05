/* iowa/main.c */

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

/* iowa: file system utilities */

#define _MAIN_

#include <string.h>
#include <time.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include "sys/defs.h"
#include "sys/msg.h"
#include "sys/clk.h"
#include "sys/ser.h"
#include "sys/tty.h"
#include "sys/en_dst.h"
#include "net/twi.h"
#include "net/memz.h"
#include "net/memp.h"
#include "net/istream.h"
#include "net/ostream.h"
#include "hc05/bc4.h"
#include "sys/sysinit.h"
#include "sys/syscon.h"
#include "sys/inp.h"
#include "sys/serin.h"
#include "isp/icsp.h"
#include "isp/icsd.h"
#include "cli/fsu.h"
#include "cli/ls.h"
#include "cli/mv.h"
#include "cli/pwd.h"
#include "cli/rm.h"
#include "cli/mk.h"

int main (void)
{
    extern uchar_t lost_msgs;
    message msg;
    extern char __heap_start;
    MsgProc fp;

    static const MsgProc __flash proctab[] = {
        [SER] = receive_ser,
        [SYSINIT] = receive_sysinit,
        [SYSCON] = receive_syscon,
        [INP] = receive_inp,
        [TTY] = receive_tty,
        [CLK] = receive_clk,
        [TWI] = receive_twi,
        [MEMZ] = receive_memz,
        [MEMP] = receive_memp,
        [ISTREAM] = receive_istream,
        [OSTREAM] = receive_ostream,
        [FSU] = receive_fsu,
        [LS] = receive_ls,
        [MV] = receive_mv,
        [PWD] = receive_pwd,
        [RM] = receive_rm,
        [MK] = receive_mk,
        [SERIN] = receive_serin,
        [HC05] = receive_bc4, /* generic */
        [ICSP] = receive_icsp,
        [ICSD] = receive_icsd
    };

    /* initialize stack memory with 0xAA to record the extent of its descent. */
    memset(&__heap_start, 0xAA, (SP - 4) - (ushort_t) &__heap_start);

    config_sysinit();
    config_msg();
    config_ser();
    config_bc4();
    config_twi();

    /* set timezone and daylight saving time function */
    set_zone(0);
    set_dst(en_dst);

    sei(); /* enable interrupts. */

    /* Fudge the sender ID so the reply goes to INP. */
    send_m1(INP, SYSINIT, INIT);

    /* Loop forever */
    for (;;) {
        extract_msg(&msg);
        if (msg.receiver && msg.receiver < NR_TASKS &&
                  (fp = (MsgProc) pgm_read_word_near(proctab + msg.receiver)))
            if ((fp) (&msg) == ENOMSG)
                lost_msgs++;
    } 
}

/* end code */

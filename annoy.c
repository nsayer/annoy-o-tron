/*

 Annoy-o-tron Tiny
 Copyright 2014 Nicholas W. Sayer
 
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License along
 with this program; if not, write to the Free Software Foundation, Inc.,
 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#define F_CPU (500000UL)

#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <util/atomic.h>

#include <util/delay.h>

volatile uint16_t tick_cnt;
volatile uint8_t beeping;

ISR(TIM0_COMPA_vect) {
  tick_cnt++;
  if (beeping) PINB |= _BV(02); // toggle
}

// Found this at http://uzebox.org/forums/viewtopic.php?f=3&t=250
static uint32_t seed = 0x71e1d07d; // sigh. Lame.
#define M (0x7fffffffL)

// Generate a random int between 1 and max, inclusive.
static inline uint16_t __attribute__ ((always_inline)) q_rand(uint16_t max) {
  seed = (seed >> 16) + ((seed << 15) & M) - (seed >> 21) - ((seed << 10) & M);
  if (seed < 0) seed += M;
  return (seed % max) + 1;
}

static inline uint16_t __attribute__ ((always_inline)) ticks() {
  uint16_t out;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    out = tick_cnt;
  }
  return out;
}

void __ATTR_NORETURN__ main() {
  ACSR = _BV(ACD); // Turn off analog comparator
  PRR = _BV(PRADC); // turn off the ADC's power, which probably is a no-op on the tiny9.

  // set up 500 kHz system clock
  CCP = 0xd8; // double-secret probation
  CLKPSR = _BV(CLKPS2); // divide by 16. The clock source is the 8 MHz RC osc.

  PUEB = _BV(0) | _BV(1); // force unused pins to a definite state
  DDRB = _BV(2); // B2 is the speaker
  PORTB = 0; // start off

  TCCR0A = 0;
  TIMSK0 = _BV(OCIE0A); // interrupt on compare

  beeping = 0;

  set_sleep_mode(SLEEP_MODE_IDLE);

  sei();

  while(1) {

    uint16_t start, duration;

    // First, we beep

    uint16_t freq = q_rand(5);
    TCCR0B = 0; // stop
    TCNT0 = 0; // reset count
    switch(freq) {
      case 1: OCR0A = 500 - 1; duration = 250; break; // 500 Hz.
      case 2: OCR0A = 250 - 1; duration = 500; break; // 1 kHz
      case 3: OCR0A = 167 - 1; duration = 750; break; // 1.5 kHz
      case 4: OCR0A = 125 - 1; duration = 1000; break; // 2 kHz
      case 5: OCR0A = 100 - 1; duration = 1250; break; // 2.5 kHz
    }
    beeping = 1;
    TCCR0B = _BV(WGM02) | _BV(CS00); // CTC mode, divide by 1
    for(start = ticks(); ticks() - start < duration;) {
      sleep_mode();
    }
    TCCR0B = 0; // stop
    beeping = 0;
    PORTB &= ~(_BV(2)); // force low for idle

    // Next, we wait

    TCNT0 = 0;
    OCR0A = 7812 - 1; // one seconds per interrupt (well, 7812.5, but...)
    TCCR0B = _BV(WGM02) | _BV(CS01) | _BV(CS00); // CTC mode, divide by 64
    duration = 3600 - q_rand(1800); // anywhere between 1/2 and 1 hour
    for(start = ticks(); ticks() - start < duration;) {
      sleep_mode();
    }
    TCCR0B = 0; // stop

  }

  __builtin_unreachable();
}


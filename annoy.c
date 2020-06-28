/*

 Annoy-o-tron Tiny
 Copyright 2020 Nicholas W. Sayer
 
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

#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <util/atomic.h>

volatile uint16_t tick_cnt;
volatile uint8_t beeping;

ISR(TIM0_COMPA_vect) {
  tick_cnt++;
  if (beeping) PINB |= _BV(2) | _BV(1); // toggle the two pins
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

  PUEB = _BV(0); // force unused pin to a definite state
  DDRB = _BV(2) | _BV(1); // B1 / B2 is the speaker
  PORTB = 0; // start off

  TCCR0A = 0;
  TIMSK0 = _BV(OCIE0A); // interrupt on compare

  beeping = 0;

  set_sleep_mode(SLEEP_MODE_IDLE);

  sei();

  while(1) {

    uint16_t duration;

    // First, we beep

    // 1 MHz system clock
    CCP = 0xd8; // double-secret probation
    CLKMSR = 0; // select the 8 MHz clock
    CCP = 0xd8;
    CLKPSR = _BV(CLKPS0) | _BV(CLKPS1); // divide by 8.

    uint16_t freq = q_rand(5);
    TCCR0B = 0; // stop
    TCNT0 = 0; // reset count
    switch(freq) { // all durations are 1/4 sec - so half the frequency
      case 1: OCR0A = 142 - 1; duration = 440; break; // 3520 Hz (A)
      case 2: OCR0A = 119 - 1; duration = 523; break; // 4186 Hz (C)
      case 3: OCR0A = 100 - 1; duration = 622; break; // 4978 Hz (Eflat)
      case 4: OCR0A = 84 - 1; duration = 739; break; // 5919 Hz (Gflat)
      case 5: OCR0A = 71 - 1; duration = 880; break; // 7040 Hz (A)
    }
    beeping = 1;
    PORTB |= _BV(1); // turn one pin on to start alternating
    TCCR0B = _BV(WGM02) | _BV(CS00); // CTC mode, divide by 1
    for(tick_cnt = 0; ticks() < duration;) {
      sleep_mode();
    }
    TCCR0B = 0; // stop
    beeping = 0;
    PORTB &= ~(_BV(2) | _BV(1)); // force both low for idle

    // Next, we wait

    // 128 kHz system clock
    CCP = 0xd8;
    CLKPSR = 0; // divide by 1.
    CCP = 0xd8;
    CLKMSR = _BV(CLKMS0); // select the 128 kHz clock

    TCNT0 = 0;
    OCR0A = 1250 - 1; // ten seconds per interrupt
    TCCR0B = _BV(WGM02) | _BV(CS02) | _BV(CS00); // CTC mode, divide by 1024
    duration = 360 - q_rand(180); // anywhere between 1/2 and 1 hour
    for(tick_cnt = 0; ticks() < duration;) {
      sleep_mode();
    }
    TCCR0B = 0; // stop

  }

  __builtin_unreachable();
}


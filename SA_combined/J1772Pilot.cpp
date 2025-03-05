/*
 * Copyright (c) 2023-2025 -  2MuchSun.com by Bobby Chung
 * Parts of this software is based on Open EVSE - Copyright (c) 2011-2019 Sam C. Lin
 * This software is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.

 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
 
#include "SA_combined.h"

#define TOP 1023

void J1772Pilot::Init()
{
  analogWriteRange(1023);       // set resolution for PWM in case it changes
  analogWriteFreq(1000);        // set frequency for PWM in case it changes from default
  pinMode(PILOT, OUTPUT);
  SetState(PILOT_STATE_P12); // turns the pilot on 12V steady state
}

// no PWM pilot signal - steady state
// PILOT_STATE_P12 = steady +12V (ADAPTER_STATE_A - VEHICLE NOT CONNECTED)
// PILOT_STATE_N12 = steady -12V (ADAPTER_STATE_F - FAULT) 

void J1772Pilot::SetState(PILOT_STATE state)
{
  if (state == PILOT_STATE_P12) {
    digitalWrite(PILOT,HIGH);
  }
  else {
    digitalWrite(PILOT,LOW);
  }
  m_State = state;
}


// set EVSE current capacity in Amperes
// duty cycle 
// outputting a 1KHz square wave to digital pin PILOT
//
int J1772Pilot::SetPWM(uint8_t amps)
{
  unsigned cnt;
  if ((amps >= MIN_CURRENT_CAPACITY_J1772) && (amps <= 51)) { 
    cnt = amps * (TOP/60);
  }
  else if ((amps > 51) && (amps <= 80)) {
    cnt = (amps * (TOP/250)) + (64*(TOP/100));
  }
  else {
    return 1;
  }
  analogWrite (PILOT,cnt);
  m_State = PILOT_STATE_PWM;
  return 0;
}

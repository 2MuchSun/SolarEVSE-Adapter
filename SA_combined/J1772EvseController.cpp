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

#ifdef PROTOTYPE
THRESH_DATA g_DefaultThreshData = {875-128-37,780-128-37,690-128-37,0,260-128-37}; //{875,780,690,0,260}; 
#else
#ifdef FIRST_ARTICLE
THRESH_DATA g_DefaultThreshData = {825,716,590,0,190}; //{875,780,690,0,260}  for first articles
#else
THRESH_DATA g_DefaultThreshData = {800,700,600,0,220}; //{875,780,690,0,260}  for final production version
#endif
#endif

J1772EVSEController g_EvseController;

 void J1772EVSEController::SaveEvseFlags()
 {
  writeEEPROMbyte(ADAPTER_FLAGS_START,m_wFlags);
 }

void J1772EVSEController::chargingOn()
{  // add the 1300 ohm resistor to indicate ready to charge
   digitalWrite(CHARGING, HIGH);
   m_bVFlags |= ECVF_CHARGING_ON;  
   m_ChargeOnTime = millis()/1000;
   SetLimitSleep(0);  //clears sleeping due to time limit reached in case it was set
}

void J1772EVSEController::chargingOff()
{ // remove the 1300 ohm resistor to indicate stop charging
    digitalWrite(CHARGING, LOW);
    m_bVFlags &= ~ECVF_CHARGING_ON;
    m_ChargeOffTime = millis()/1000; 
} 

void J1772EVSEController::SetStartSleep(uint8_t tf)
{ 
   if (tf) m_wFlags |= ECF_POWER_UP_IN_SLEEP; 
   else m_wFlags &= ~ECF_POWER_UP_IN_SLEEP; 
   SaveEvseFlags(); 
}

void J1772EVSEController::SetAutomaticMode(uint8_t tf, uint8_t no_save)
{ 
   immediate_status_update = true; 
   if (tf) 
   {
      m_wFlags &= ~ECF_AUTOMATIC;
      SetCurrentCapacity(MIN_CURRENT_CAPACITY_J1772,1); //set pilot to minimum
      if (EVSE_READY && plugged_in && allow_on_temporarily){ //turn on power when plugged in so that auto tracking will adjust to proper priority
        forced_reset_priority = true;
        Enable();
        SetCurrentCapacity(MIN_CURRENT_CAPACITY_J1772,1);
       }
   } 
   else 
   {
      m_wFlags |= ECF_AUTOMATIC;
      SetCurrentCapacity(GetMaxCurrentCapacity(),1);  //restore saved pilot value
   }
   if (!no_save)
      SaveEvseFlags();
}

void J1772EVSEController::SetAdapterBypass(uint8_t tf)
{ 
   if (tf){
       m_wFlags |= ECF_ADAPTER_BYPASS;
       digitalWrite(USE_SA_PILOT,LOW);
       m_PrevEvseState = m_AdapterState;
       m_AdapterState = ADAPTER_STATE_BYPASS;
       digitalWrite(PLUG_IN, LOW);  //need to make sure the Plug_In and Charging resistors are not engaged for bypass mode
       chargingOff(); 
   }
   else {
      m_wFlags &= ~ECF_ADAPTER_BYPASS;
      digitalWrite(USE_SA_PILOT,HIGH);
      m_AdapterState = m_PrevEvseState;
      chargingOff();
   }
   immediate_status_update = true;
   SaveEvseFlags(); 
}

void J1772EVSEController::EnableVentReq(uint8_t tf)
{
  if (tf) {
    m_wFlags &= ~ECF_VENT_REQ_DISABLED;
  }
  else {
    m_wFlags |= ECF_VENT_REQ_DISABLED;
  }
  SaveEvseFlags();
}

void J1772EVSEController::EnableDiodeCheck(uint8_t tf)
{
  if (tf) {
    m_wFlags &= ~ECF_DIODE_CHK_DISABLED;
  }
  else {
    m_wFlags |= ECF_DIODE_CHK_DISABLED;
  }
  SaveEvseFlags();
}

void J1772EVSEController::Enable()
{
  if ((m_AdapterState == ADAPTER_STATE_SLEEPING)) {
    m_PrevEvseState = ADAPTER_STATE_SLEEPING;
    m_AdapterState = ADAPTER_STATE_UNKNOWN;
    m_Pilot.SetState(PILOT_STATE_P12);
  }
  adapter_enabled = 1;
  limits_reached = false;   //allow limits if set
  if (IsBypassMode())
    m_AdapterState = ADAPTER_STATE_BYPASS;
}

void J1772EVSEController::Sleep()
{
  if (m_AdapterState != ADAPTER_STATE_SLEEPING) {
    m_Pilot.SetState(PILOT_STATE_P12);
    m_AdapterState = ADAPTER_STATE_SLEEPING;
  }
  adapter_enabled = 0;
  immediate_status_update = true;
  if (IsBypassMode())
    m_AdapterState = ADAPTER_STATE_BYPASS;
}

void J1772EVSEController::LoadThresholds()
{
  memcpy(&m_ThreshData,&g_DefaultThreshData,sizeof(m_ThreshData));
}

void J1772EVSEController::SetSvcLevel(uint8_t svclvl)
{
  if (svclvl == 2) {
    m_wFlags |= ECF_L2; // set to Level 2
  }
  else {
    svclvl = 1; // force invalid value to L1
    m_wFlags &= ~ECF_L2; // set to Level 1
  }
  SaveEvseFlags();
  immediate_status_update = true;
}

uint8_t J1772EVSEController::GetMaxCurrentCapacity()
{
  uint8_t ampacity =  readEEPROMbyte(PILOT_START);

  if ((ampacity == 0xff) || (ampacity == 0))
    ampacity = MIN_CURRENT_CAPACITY_J1772;
  
  if (ampacity < MIN_CURRENT_CAPACITY_J1772)
    ampacity = MIN_CURRENT_CAPACITY_J1772; 
  else if (ampacity > MAX_CURRENT_CAPACITY_L2)
    ampacity = MAX_CURRENT_CAPACITY_L2;
      
  return ampacity;
}

void J1772EVSEController::Init()
{
  m_AdapterState = ADAPTER_STATE_UNKNOWN;
  m_PrevEvseState = ADAPTER_STATE_UNKNOWN;

  // read settings from EEPROM
  uint8_t rflgs = readEEPROMbyte(ADAPTER_FLAGS_START);

  chargingOff();

  m_Pilot.Init(); // init the pilot

  uint8_t svclvl = (uint8_t)DEFAULT_SERVICE_LEVEL;

  if (rflgs == 0xffff) { // uninitialized EEPROM
    m_wFlags = ECF_DEFAULT;
    SetSvcLevel(svclvl);
  }
  else {
    m_wFlags = rflgs;
  }

  Sleep();   //start off in sleep, will change to user set adapter mode in setup if start up in sleep flag is not set
   
  if (IsBypassMode())
    SetAdapterBypass(1);

  m_bVFlags = ECVF_DEFAULT;
  LoadThresholds();
  if (IsAutomaticMode()){
    SetCurrentCapacity(MIN_CURRENT_CAPACITY_J1772,1);
  }
  else {
    uint8_t ampacity = GetMaxCurrentCapacity();
    SetCurrentCapacity(ampacity,1);
  }
}

void J1772EVSEController::ReadPilot(uint16_t *plow,uint16_t *phigh,int loopcnt)
{
  uint16_t pl = 1023, lh = 0;
  uint16_t ph = 0, hl = 1023;
  uint8_t hi_filter = 0, lo_filter = 0;

  if (adc_address != 0){
    for (uint16_t i=0; i < loopcnt; i++) {
      Wire.requestFrom(adc_address, (size_t)2,true);
      uint16_t reading;
      while (Wire.available()){
        reading = (((uint16_t)Wire.read() << 8) | (uint16_t)Wire.read()) >> 2;  // reads pilot voltage
      }
      if (reading > ph) {
        if (reading < hl)
          hl = reading; //find the lowest of the set of highs
        hi_filter++;
        if (hi_filter > 3) {
          ph = hl;   //use the lowest of the highs to filter out any temporary spikes from PWM
          hi_filter = 0;
          hl = 1023;
        }
      }
      else if (reading < pl) {
        if (reading > lh)
          lh = reading; //find the highest of the set of highs
        lo_filter++;
        if (lo_filter > 3) {
          lo_filter = 0;
          pl = lh;   //use the highest of the lows to filter out any temporary spikes from PWM
          lh = 0;
        }
      }   
    }
    *plow = pl;
    *phigh = ph;
  }
  else{
    *plow = 1023;
    *phigh = 1023;
  }
}


//TABLE A1 - PILOT LINE VOLTAGE RANGES (recommended.. adjust as necessary
//                           Minimum Nominal Maximum 
//Positive Voltage, State A  11.40 12.00 12.60 
//Positive Voltage, State B  8.36 9.00 9.56 
//Positive Voltage, State C  5.48 6.00 6.49 
//Positive Voltage, State D  2.62 3.00 3.25 
//Negative Voltage - States B, C, D, and F -11.40 -12.00 -12.60 
void J1772EVSEController::Update()
{
  uint16_t plow;
  uint16_t phigh = 0xffff;
  unsigned long curms = millis();
  if (skip_pp || solar_evse)
    sample_pp = 0;
  else
    sample_pp = analogRead(A0);
  ReadPilot(&plow,&phigh);  //this always updates the status of the connector whether it is plugged in even if the EVSE is sleeping, diabled or faulted. used as reminder
  if (phigh >= m_ThreshData.m_ThreshAB) {  //connector disconnected
      SetConnectedFlag(0);
      if (unplugged_hi < phigh)
         unplugged_hi = phigh;
      if (unplugged_lo > phigh)
         unplugged_lo = phigh;
    }
  else if (phigh >= m_ThreshData.m_ThreshCD){  //connected which includes charging since you have to be connected if charging
      SetConnectedFlag(1);
      if (phigh >= m_ThreshData.m_ThreshBC){
        if (plugged_in_hi < phigh)
          plugged_in_hi = phigh;
        if (plugged_in_lo > phigh)
          plugged_in_lo = phigh;
      }
    }  

  if (m_AdapterState == ADAPTER_STATE_BYPASS) {
    if (sample_pp < (PP_NOT_PRESS + 20) || sample_pp > (NO_PP - 20)){
      digitalWrite(USE_SA_PILOT,LOW);
      adapter_enabled = 0;
      if (chargingIsOn())
        chargingOff();  
    }
    else {
       digitalWrite(USE_SA_PILOT,HIGH);
       chargingOff(); 
    }
    return;
  }

  if (m_AdapterState == ADAPTER_STATE_SLEEPING) {
    adapter_enabled = 0;
    if (chargingIsOn()) {
      chargingOff();  
    }
    m_PrevEvseState = m_AdapterState; // cancel state transition
    return;
  }
  else
    adapter_enabled = 1;
  
  if (m_AdapterState == ADAPTER_STATE_D){
     m_PrevEvseState = m_AdapterState; // cancel state transition
    return;
  }
  
  if (m_AdapterState == ADAPTER_STATE_DIODE_CHK_FAILED){
    if ((m_Pilot.GetState() == PILOT_STATE_PWM) && (plow < m_ThreshData.m_ThreshDS)) { // got better or unplugged
      m_AdapterState = ADAPTER_STATE_UNKNOWN;  //reset to try again
     }
    m_PrevEvseState = m_AdapterState; // cancel state transition
    return;
  }

  uint8_t pre_state = m_AdapterState;
  uint8_t tmp_state = ADAPTER_STATE_UNKNOWN;
  uint8_t prevpilotstate = m_PilotState;
  uint8_t tmppilotstate = ADAPTER_STATE_UNKNOWN;
 
  if ((pre_state >= ADAPTER_FAULT_STATE_BEGIN) &&
	    (pre_state <= ADAPTER_FAULT_STATE_END)) {
      // just got out of fault state - pilot back on
      m_Pilot.SetState(PILOT_STATE_P12);
      pre_state = ADAPTER_STATE_UNKNOWN;
      m_AdapterState = ADAPTER_STATE_UNKNOWN;
  }
  if (sample_pp < (PP_NOT_PRESS + 20) || sample_pp > (NO_PP - 20)){
    if ((m_Pilot.GetState() == PILOT_STATE_PWM) && (plow >= m_ThreshData.m_ThreshDS) && DiodeCheckEnabled()) {
        valid_vent = 0;
        // diode check failed
        tmp_state = ADAPTER_STATE_DIODE_CHK_FAILED;
        tmppilotstate = ADAPTER_STATE_DIODE_CHK_FAILED;
    }
    else if (phigh >= m_ThreshData.m_ThreshAB) {
        valid_vent = 0;
        // 12V EV not connected
        tmp_state = ADAPTER_STATE_A;
        tmppilotstate = ADAPTER_STATE_A;
    }
    else if (phigh >= m_ThreshData.m_ThreshBC) {
        valid_vent = 0;
        // 9V EV connected, waiting for ready to charge
        tmp_state = ADAPTER_STATE_B;
        tmppilotstate = ADAPTER_STATE_B;
    }
    else if (phigh >= m_ThreshData.m_ThreshCD) {
        valid_vent = 0;
        // 6V ready to charge
        tmppilotstate = ADAPTER_STATE_C;
        if (charge_hi < phigh)
          charge_hi = phigh;
        if (charge_lo > phigh)
          charge_lo = phigh;
        if (m_Pilot.GetState() == PILOT_STATE_PWM) {
  	      tmp_state = ADAPTER_STATE_C;
        }
        else {
  	    // PWM is off so we can't charge.. force to State B
  	      tmp_state = ADAPTER_STATE_B;
        }
    }
    else if (phigh > m_ThreshData.m_ThreshD) {
        valid_vent++;
        if (valid_vent > 100)
          valid_vent = 100;
        // 3V ready to charge vent required
        if (VentReqEnabled()) {
          if (valid_vent > 4){    //must detect valid vent 5 times in a row to consider valid
            tmppilotstate = ADAPTER_STATE_D;
  	        tmp_state = ADAPTER_STATE_D;
          }        
        }
        else {
          tmppilotstate = ADAPTER_STATE_D;
  	      tmp_state = ADAPTER_STATE_C;
        }
    }
    else {
        tmp_state = ADAPTER_STATE_UNKNOWN;
    }
  }
  else {
    tmp_state = ADAPTER_STATE_B;
    tmppilotstate = ADAPTER_STATE_B;
  }
  
  // debounce state transitions
  if (tmp_state != pre_state) {
      /*if (tmp_state != m_TmpEvseState) {
        m_TmpEvseStateStart = curms;
      }
      else if ((curms - m_TmpEvseStateStart) >= ((tmp_state == ADAPTER_STATE_A) ? DELAY_STATE_TRANSITION_A : DELAY_STATE_TRANSITION)) {*/
        m_AdapterState = tmp_state;
     // }
  }
    
 // m_TmpEvseState = tmp_state;

  // check request for state transition to A/B/C
  if (m_StateTransitionReqFunc && (m_AdapterState != pre_state) &&
      ((m_AdapterState >= ADAPTER_STATE_A) && (m_AdapterState <= ADAPTER_STATE_C))) {
      m_PilotState = tmppilotstate;
      uint8_t newstate = (*m_StateTransitionReqFunc)(prevpilotstate,m_PilotState,pre_state,m_AdapterState);
      if (newstate) {
      m_AdapterState = newstate;
      }
  }

  // state transition
  if (m_AdapterState != pre_state) {
    if (m_AdapterState == ADAPTER_STATE_A) { // EV not connected
      unplugged_count++;
      chargingOff(); // turn off charging current
      m_Pilot.SetState(PILOT_STATE_P12);  
    }
    else if (m_AdapterState == ADAPTER_STATE_B) { // connected
      plugged_in_count++;
      chargingOff(); // turn off charging current
      m_Pilot.SetPWM(m_CurrentCapacity);
      //Serial.print("Pilot is set at ");
      //Serial.println (m_CurrentCapacity);
    }
    else if (m_AdapterState == ADAPTER_STATE_C) {
      charge_count++;
      m_Pilot.SetPWM(m_CurrentCapacity);
      chargingOn(); // turn on charging current
    }
    else if (m_AdapterState == ADAPTER_STATE_D) {
      // vent required not supported
      chargingOff(); // turn off charging current
      m_Pilot.SetState(PILOT_STATE_P12);
    }
    else if (m_AdapterState == ADAPTER_STATE_DIODE_CHK_FAILED) {
      chargingOff(); // turn off charging current
      // must leave pilot on so we can keep checking
      // N.B. J1772 specifies to go to State F (-12V) but we can't do that
      // and keep checking
      m_Pilot.SetPWM(m_CurrentCapacity);
      //m_Pilot.SetState(PILOT_STATE_P12);
    }
    else {
      m_Pilot.SetState(PILOT_STATE_P12);
      chargingOff(); // turn off charging current
    }
    immediate_status_update = true;
#ifdef SERDBG
    Serial.print("state: ");
    switch (m_Pilot.GetState()) {
    case PILOT_STATE_P12: Serial.print("P12"); break;
    case PILOT_STATE_PWM: Serial.print("PWM"); break;
    case PILOT_STATE_N12: Serial.print("N12"); break;
    }
    Serial.print(" ");
    Serial.print((uint8_t)pre_state);
    Serial.print("->");
    Serial.print((uint8_t)m_AdapterState);
    Serial.print(" p ");
    Serial.print(plow);
    Serial.print(" ");
    Serial.println(phigh);
  }
#endif //#ifdef SERDBG
  }
 
  m_PrevEvseState = pre_state;
  if (m_AdapterState == ADAPTER_STATE_C) {
    m_ElapsedChargeTimePrev = m_ElapsedChargeTime;
    m_ElapsedChargeTime = millis()/1000 - m_ChargeOnTime;
  }
}

#ifdef CALIBRATE
// read ADC values and get min/max/avg for pilot steady high/low states
void J1772EVSEController::Calibrate(PCALIB_DATA pcd)
{
  uint16_t pmax,pmin,pavg,nmax,nmin,navg;

  for (uint8_t l=0;l < 2;l++) {
    int reading;
    uint32_t tot = 0;
    uint16_t plow = 1023;
    uint16_t phigh = 0;
    uint16_t avg = 0;
    m_Pilot.SetState(l ? PILOT_STATE_N12 : PILOT_STATE_P12);

    delay(250); // wait for stabilization

    // 1x = 114us 20x = 2.3ms 100x = 11.3ms
    int i;
    for (i=0;i < 1000;i++) {
      reading = adcPilot.read();  // measures pilot voltage

      if (reading > phigh) {
        phigh = reading;
      }
      else if (reading < plow) {
        plow = reading;
      }

      tot += reading;
    }
    avg = tot / i;

    if (l) {
      nmax = phigh;
      nmin = plow;
      navg = avg;
    }
    else {
      pmax = phigh;
      pmin = plow;
      pavg = avg;
    }
  }
  pcd->m_pMax = pmax;
  pcd->m_pAvg = pavg;
  pcd->m_pMin = pmin;
  pcd->m_nMax = nmax;
  pcd->m_nAvg = navg;
  pcd->m_nMin = nmin;
}
#endif // CALIBRATE

uint8_t J1772EVSEController::SetCurrentCapacity(uint8_t amps,uint8_t nosave)
{
  uint8_t rc = 0, desired_max;
  immediate_status_update = true;
  if ((set_current >= MIN_CURRENT_CAPACITY_J1772) && (set_current <= MAX_CURRENT_CAPACITY_L2)) {
    desired_max = set_current;
  }
  else {
    desired_max = MAX_CURRENT_CAPACITY_L1;
  }
  if ((amps >= MIN_CURRENT_CAPACITY_J1772) && (amps <= desired_max)) {
    m_CurrentCapacity = amps;
  }
  else if (amps < MIN_CURRENT_CAPACITY_J1772) {
    m_CurrentCapacity = MIN_CURRENT_CAPACITY_J1772;
    rc = 1;
  }
  else {
    m_CurrentCapacity = desired_max;
    rc = 2;
  }

  if (!nosave) {
    writeEEPROMbyte(PILOT_START, m_CurrentCapacity);
  }

  if (m_Pilot.GetState() == PILOT_STATE_PWM) {
    m_Pilot.SetPWM(m_CurrentCapacity);
  }
  return rc;
}

//-- end J1772EVSEController

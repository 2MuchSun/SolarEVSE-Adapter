#pragma once
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

//states for m_AdapterState
#define ADAPTER_STATE_UNKNOWN 0x00
#define ADAPTER_STATE_A       0x01 // vehicle state A 12V - not connected
#define ADAPTER_STATE_B       0x02 // vehicle state B 9V - connected, ready
#define ADAPTER_STATE_C       0x03 // vehicle state C 6V - charging
#define ADAPTER_STATE_D       0x04 // vehicle state D 3V - vent required
#define ADAPTER_FAULT_STATE_BEGIN ADAPTER_STATE_D
#define ADAPTER_STATE_DIODE_CHK_FAILED 0x05 // diode check failed
#define ADAPTER_FAULT_STATE_END ADAPTER_STATE_DIODE_CHK_FAILED
#define ADAPTER_STATE_BYPASS 0x0B
#define ADAPTER_STATE_SLEEPING 0xfe // standby

typedef struct threshdata {
  uint16_t m_ThreshAB; // state A -> B
  uint16_t m_ThreshBC; // state B -> C
  uint16_t m_ThreshCD; // state C -> D
  uint16_t m_ThreshD;  // state D
  uint16_t m_ThreshDS; // diode short
} THRESH_DATA,*PTHRESH_DATA;

typedef struct calibdata {
  uint16_t m_pMax;
  uint16_t m_pAvg;
  uint16_t m_pMin;
  uint16_t m_nMax;
  uint16_t m_nAvg;
  uint16_t m_nMin;
} CALIB_DATA,*PCALIB_DATA;

// called whenever EVSE wants to transition state from curEvseState to newEvseState
// return 0 to allow transition to newEvseState
// else return desired state
typedef uint8_t (*EvseStateTransitionReqFunc)(uint8_t curPilotState,uint8_t newPilotState,uint8_t curEvseState,uint8_t newEvseState);

// J1772EVSEController m_wFlags bits - saved to EEPROM
#define ECF_L2                 0x01 // service level 2
#define ECF_DIODE_CHK_DISABLED 0x02 // no diode check
#define ECF_VENT_REQ_DISABLED  0x04 // no vent required
#define ECF_NOT_USED           0x08 // spare
#define ECF_NOT_USED_2         0x10 // spare
#define ECF_ADAPTER_BYPASS     0x20 // Use EVSE's control pilot instead, bypass SA
#define ECF_AUTOMATIC          0x40 // auto tracking mode
#define ECF_POWER_UP_IN_SLEEP  0x80 // start up in sleep mode
#define ECF_DEFAULT            0x0000

// J1772EVSEController volatile m_bVFlags bits - not saved to EEPROM
#define ECVF_SPARE              0x01 // spare
#define ECVF_SPARE_1            0x02 // spare
#define ECVF_LIMIT_SLEEP        0x04 // currently sleeping after reaching time/charge limit
#define ECVF_CONNECTED          0X08 // plugged in EV
#define ECVF_SPARE_3            0x10 // spare
#define ECVF_SPARE_4            0x20 // spare
#define ECVF_CHARGING_ON        0x40 // charging relay is closed
#define ECVF_SPARE_5            0x80 // spare
#define ECVF_DEFAULT            0x00

class J1772EVSEController {
  J1772Pilot m_Pilot;

  uint8_t m_wFlags; // ECF_xxx
  uint8_t m_bVFlags; // ECVF_xxx
  THRESH_DATA m_ThreshData;
  uint8_t m_AdapterState;
  uint8_t m_PrevEvseState;
  //uint8_t m_TmpEvseState;
  uint8_t m_PilotState;
  //unsigned long m_TmpEvseStateStart;
  uint8_t m_CurrentCapacity; // max amps we can output
  time_t m_ChargeOnTime; // unixtime when relay last closed
  time_t m_ChargeOffTime;   // unixtime when relay last opened
  time_t m_ElapsedChargeTime;
  time_t m_ElapsedChargeTimePrev;
  time_t Timer_1;
  EvseStateTransitionReqFunc m_StateTransitionReqFunc;

  void chargingOn();
  void chargingOff();
  uint8_t chargingIsOn() { return m_bVFlags & ECVF_CHARGING_ON; }
  void setFlags(uint16_t flags) { 
    m_wFlags |= flags; 
  }
  void clrFlags(uint16_t flags) { 
    m_wFlags &= ~flags; 
  }

#ifdef TIME_LIMIT
  uint8_t m_timeLimit; // minutes * 15
#endif

public:
  //J1772EVSEController();
  void Init();
  void Update(); // read sensors
  void Enable();
  void Sleep(); // graceful stop - e.g. waiting for timer to fire- give the EV time to stop charging first
  void LoadThresholds();

  uint8_t GetFlags() { 
    return m_wFlags; 
  }
  uint8_t GetVFlags() { 
    return m_bVFlags; 
  } 
  uint8_t GetState() { 
    return m_AdapterState;
  }
  uint8_t GetPrevState() { 
    return m_PrevEvseState; 
  }
  int StateTransition() { 
    return (m_AdapterState != m_PrevEvseState) ? 1 : 0; 
  }

  uint8_t GetCurrentCapacity() { 
    return m_CurrentCapacity; 
  }
  uint8_t GetMaxCurrentCapacity();
  uint8_t SetCurrentCapacity(uint8_t amps,uint8_t nosave=0);
  time_t GetElapsedChargeTime() { 
    return m_ElapsedChargeTime; 
  }
  time_t GetElapsedChargeTimePrev() { 
    return m_ElapsedChargeTimePrev; 
  }
  time_t GetChargeOffTime() { 
    return m_ChargeOffTime; 
  }
  void Calibrate(PCALIB_DATA pcd);
  uint8_t GetCurSvcLevel() { 
    return (m_wFlags & ECF_L2) ? 2 : 1; 
  }
  void SetSvcLevel(uint8_t svclvl);
  PTHRESH_DATA GetThreshData() { 
    return &m_ThreshData; 
  }
  uint8_t DiodeCheckEnabled() { 
    return (m_wFlags & ECF_DIODE_CHK_DISABLED) ? 0 : 1;
  }
  void EnableDiodeCheck(uint8_t tf);
  
  uint8_t VentReqEnabled() { 
    return (m_wFlags & ECF_VENT_REQ_DISABLED) ? 0 : 1;
  }
  void EnableVentReq(uint8_t tf);

  void SetStartSleep(uint8_t tf);

  void SetAutomaticMode(uint8_t tf, uint8_t no_save = 0);

  void SetAdapterBypass(uint8_t tf);
 
  void ReadPilot(uint16_t *plow,uint16_t *phigh,int loopcnt=PILOT_LOOP_CNT);
  void SaveEvseFlags();
  
  void SetStateTransitionReqFunc(EvseStateTransitionReqFunc statetransitionreqfunc) {
    m_StateTransitionReqFunc = statetransitionreqfunc;
  }
  
  void SetConnectedFlag(uint8_t tf) { 
    if (tf) m_bVFlags |= ECVF_CONNECTED;
    else m_bVFlags &= ~ECVF_CONNECTED; 
  }
  
  void SetLimitSleep(int8_t tf) {
    if (tf) m_bVFlags |= ECVF_LIMIT_SLEEP;
    else m_bVFlags &= ~ECVF_LIMIT_SLEEP;
  }
  int8_t LimitSleepIsSet() { return (int8_t)(m_bVFlags & ECVF_LIMIT_SLEEP); }
  
  uint8_t IsStartSleep() { return (m_wFlags & ECF_POWER_UP_IN_SLEEP) ? 1 : 0; }
  uint8_t IsAutomaticMode() { return (m_wFlags & ECF_AUTOMATIC) ? 0 : 1; }
  uint8_t IsBypassMode() { return (m_wFlags & ECF_ADAPTER_BYPASS) ? 1 : 0; }

};


extern J1772EVSEController g_EvseController;

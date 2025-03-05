#pragma once
/*
 * Copyright (c) 2023-2025 -  2MuchSun.com by Bobby Chung
 * Parts of this software is based on Open EVSE - Copyright (c) 2011-2023 Sam C. Lin, Copyright (c) 2011-2014 Chris Howell <chris1howell@msn.com>
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
 
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <ESP8266httpUpdate.h>
#include <ArduinoJson.h>
#include <time.h>

//#define PROTOTYPE    // prototype has different pin assignments and thresholds
//#define FIRST_ARTICLE

#define THIS_DEVICE "SA"
#define VERSION "2.94"  //2.94 change wait times after a power adjustment and added downscan as criteria to change power adjustment in automatic tracking algorithm
                        //2.93 update devices quicker and made extra power indicator better - released
                        //2.92 disabled alt set current in debug page
                        //2.91 figured out the increment is incorrect causing behavior noticed when creating 2.9.  Should be adding not subtracting i.e. extra_power + offset
                        //2.9 added alterate set_current in debug page to troubleshoot problem with set_current changing?
                        //2.8 fixed bug that was caused by accidently deleting maxamp in autotracking in ver 2.7
                        //2.7 moved offset setting to PowerMC where it belongs
                        //2.6 2.5 doesnt work, this fixes it temporarily
                        //2.5 temoparaily fix a bug with high offset and downscan
                        //2.4 fixed bug to not limit the adjusted offset to accomodate negative offsets
                        //2.3 changed the definition of to getting extra power data from PowerMC to co-exist efficiently with 2nd Generatior only if using JSON format, also fixed kWh accumalation bug and minor typos
                        //2.2 added timer, get time from time server and increased offset to be +- 30000 W input box instead of dropdown
                        //2.1 added WPS to be automatically started
                        //2.0 initial public release based off off smart outlet ui version 1.5 (esp8266 v3.0.2) + SAadapter 1.0x (atmel 328p)
#ifdef PROTOTYPE
#define THIS_DEVICE_UPDATE "SAP"
#else
#ifdef FIRST_ARTICLE
#define THIS_DEVICE_UPDATE "SAFA"
#else
#define THIS_DEVICE_UPDATE "SA"
#endif
#endif

#define MY_NTP_SERVER "us.pool.ntp.org"           
#define MY_TZ "EST5EDT,M3.2.0,M11.1.0"

#define DEFAULTSSID  "powermc"    //default to connect to powermc AP
#define DEFAULTPASS  "password"   //default to connect to powermc AP
#define DEFAULTMSERVER "powermc0.local"
#define POWERMC_MSERVER "192.168.4.1"
#define DEFAULTMPORT "1883"
#define APSSID "solaradapter"  //default AP ssid
#define APSSID_E "solarEVSE"
#define APPASS "password"      //defautl AP password
#define DEFAULT_BASE "2ms/PM/0/"  //default is 2ms/PM/0/
#define DEFAULT_CONSUMED "main"   //default is main

// for EEPROM map
#define EEPROM_SIZE 510  // erasable - total eeprom allocated is 600 bytes

#define START_HOUR_START 1
#define START_MIN_START 2
#define STOP_HOUR_START 3
#define STOP_MIN_START 4
#define TIMER_ENABLED_START 5
#define DOUBLE_TIME_LIMIT_START 6
#define NO_MQTT_START 7
#define TIME_ZONE_START 8
#define HALF_CHARGE_LIMIT_START 9
#define SCHEDULE_AT_START 10        // 1 byte   for scheduling autotracking
#define START_AT_START 11           // 1 byte   start hour for autotracking
#define STOP_AT_START 12            // 1 byte   stop hour for autotracking
#define LOCATION_START 94
#define LOCATION_MAX_LENGTH 32
#define SW_CONFIG_START 126
#define SW_CONFIG_MAX_LENGTH 1
#define START_AP_START 127
#define START_AP_MAX_LENGTH 1
#define MQTTBROKER_START 128
#define MQTTBROKER_MAX_LENGTH 50
#define UNAME_START 178
#define UNAME_MAX_LENGTH 32
#define PWORD_START 210
#define PWORD_MAX_LENGTH 32
#define PM_MAC_START 242
#define PM_MAC_MAX_LENGTH 18
#define CONSUMED_START 260
#define CONSUMED_MAX_LENGTH 20
#define GENERATED_START 312
#define GENERATED_MAX_LENGTH 20
#define MQTT_PORT_START 464
#define MQTT_PORT_MAX_LENGTH 5
#define MODE_START 475
#define MODE_MAX_LENGTH 1
#define DEVICE_START 476
#define DEVICE_MAX_LENGTH 1
#define ADAPTER_FLAGS_START 477
#define ADAPTER_FLAGS_MAX_LENGTH 1
#define PILOT_START 478
#define PILOT_MAX_LENGTH 1
#define DEVICE_PRIORITY_START 480
#define DEVICE_PRIORITY_MAX_LENGTH 1
#define HI_AMP_SETPOINT_START 481
#define HI_AMP_SETPOINT_MAX_LENGTH 1
#define MEMORY_CHECK_START 510
#define MEMORY_CHECK_MAX_LENGTH 1
#define SOLAR_EVSE_START 511
#define SOLAR_EVSE_MAX_LENGTH 1
#define SKIP_PP_START 512
#define SKIP_PP_MAX_LENGTH 1
#define BOOT_START 513
#define BOOT_MAX_LENGTH 1
#define IPOWER_START 514
#define IPOWER_MAX_LENGTH 2
#define ALLOW_ON_START 516
#define ALLOW_ON_MAX_LENGTH 1

#define SERVER_UPDATE_RATE 15000 //update data every 15 seconds
#define WAIT_FOR_POWER_CHANGE 10000  //10 seconds default
#define PLUG_IN_MASK 0x08
#define PLUG_IN 12
#define CHARGING 13
#define EVSE_PILOT 3
#define DEVICE_CYCLE_TIME 40000
#define REPORTING_IN_RATE 35000 //35 secs device sends status to pm devices topic
#define PM_CYCLE_TIME 11000 //10 + 1 sec
#define DELAY_START DEVICE_CYCLE_TIME
#define MAX_NUM_DEVICES 30
#define MAX_NUM_PMS 5
#define PLUG_OUT_TIMEOUT 3000 //was 20 secs
#define LOWER_PRIORITY_CONDITION ((adapter_enabled == 0) || (plugged_in == 0) || g_EvseController.IsBypassMode() || g_EvseController.IsAutomaticMode() == 0) //define conditions to decrease to the next device here
#define HIGHER_PRIORITY_CONDITION ((plugged_in == 0) || g_EvseController.IsBypassMode() || (g_EvseController.IsAutomaticMode() == 0) || ((pilot >= maxamp) && (adapter_enabled == 1)))  //define conditions to increase to the next device here
#define EVSE_READY (set_current >= MIN_CURRENT_CAPACITY_J1772)
#define DOUBLE_TIME_LIMIT_MAX  48       // x30 minutes
#define HALF_CHARGE_LIMIT_MAX  75     // x2 kWh
#define NO_PP 400
#define PP_PRESS 98      //100 for standard J1772 for 480 ohms, 90 for Tesla mobile for 430 ohms
#define PP_NOT_PRESS 55  //35 for standard J1772 for 150 ohm, 65 for Tesla mobile for 380 ohms

#ifdef PROTOTYPE
#define USE_SA_PILOT 16 //not used
#define PILOT 14
#else
#define USE_SA_PILOT 14 
#define PILOT 2
#endif

#define RED_LED_0 0
#define YELLOW_LED 16
#define SA_STATUS_UPDATE_RATE 10000
#define SLOW_BLINK_RATE 1000
#define FAST_BLINK_RATE 500
#define FIRMWARE_UPDATE_CHECK 18000000    //check network every 300 minutes for updates
#define TIME_TO_RESET_ESP 1800000         //if can't auto connect in 30 min then try resetting

#define UPDATE_HOST "<myhost.com>"
#define UPDATE_URL  "<https://myhost.com:1880/update/>"
#define UPDATE_PORT 1880

#define NONE 0
#define AP 1
#define STARTED_IN_AP 2
#define WIFI 3
#define WIFI_MQTT 4
// #define SERDBG

// n.b. DEFAULT_SERVICE_LEVEL is ignored if ADVPWR defined, since it's autodetected
#define DEFAULT_SERVICE_LEVEL 2 // 1=L1, 2=L2

// current capacity in amps
#define DEFAULT_CURRENT_CAPACITY_L1 12
#define DEFAULT_CURRENT_CAPACITY_L2 12
#define SPLIT_PHASE_VOLTAGE 120

// minimum allowable current in amps
#define MIN_CURRENT_CAPACITY_J1772 6 // J1772 min = 6
#define MIN_CURRENT_CAPACITY_L1 MIN_CURRENT_CAPACITY_J1772
#define MIN_CURRENT_CAPACITY_L2 MIN_CURRENT_CAPACITY_J1772 

// maximum allowable current in amps solar adapter does not distinguish between L1 or L2, should be the same
#define MAX_CURRENT_CAPACITY_L1 40 //  not used
#define MAX_CURRENT_CAPACITY_L2 80 // 

// for J1772.ReadPilot()
#define PILOT_LOOP_CNT 500

#include "J1772Pilot.h"
#include "J1772EvseController.h"

extern void saveWord(int16_t value, int16_t location);
extern void writeEEPROMbyte(uint16_t start_byte, uint8_t contents);
extern uint8_t readEEPROMbyte(uint16_t start_byte);
extern bool immediate_status_update, forced_reset_priority, limits_reached;
extern uint8_t adapter_enabled, adc_address, set_current, plugged_in, skip_pp, solar_evse, allow_on_temporarily;
extern uint16_t unplugged_hi, unplugged_lo, plugged_in_hi, plugged_in_lo, charge_hi, charge_lo, unplugged_count, plugged_in_count, charge_count, sample_pp;

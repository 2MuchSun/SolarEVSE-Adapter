 /*
 * Copyright (c) 2023-2026 2MuchSun.com by Bobby Chung
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

#include "SA_combined.h"

ESP8266WebServer server(80);

WiFiClient wifiClient;
PubSubClient client(wifiClient);
byte* pm_st_payload;
byte* pm_next_payload;
String pilot_message;
String auto_message;
String level_message;
String enabled_message;
String hiamp_message;
String bypass_message;
String priority_message;
String charge_limit_message;
String time_limit_message;
String schedule_AT_message;
String timer_message;
bool received_timer = false;
bool received_schedule_AT = false;
bool received_time_limit = false;
bool received_charge_limit = false;
bool received_pm_st = false;
bool received_pm_next = false;
bool received_priority = false;
bool received_bypass = false;
bool received_hiamp = false;
bool received_enabled = false;
bool received_level = false;
bool received_auto = false;
bool received_pilot = false;
bool found_other_pm = false;
bool change_network = false;

String received_error = "", received_from_evse = "", evse_tempS = "", header_received = "";
String lastConnectionResults = "";
String initialConnectionResults = "";
uint8_t unplug_time = 5, valid_vent = 0, evse_state = 0, override_evse_set_pilot = false, received_time_on = 255;
uint8_t boot_count, reboot = 0, allow_on_temporarily = 0, factor = 0;
uint8_t adc_address = 0, connection_mode = NONE, consumed_data_status = 0;
uint16_t sample_pp, evse_voltageX10, evse_currentX10, evse_pfX1000;
uint16_t on_off_wait_time = 15;
int8_t evse_tempC;
bool immediate_status_update = false, publish_immediately = false, no_consumed_data_received = true, reset_sleep = false, start_timeout = false;
uint8_t solar_evse, skip_pp, evse_max = 32, one_second =  0;
uint8_t wps_check = 0;
long lastReconnectAttempt = 0;
unsigned long start_time, recorded_elapsed_time = 0;
float kilowatt_seconds_accum = 0;
uint16_t interval_time, wait_for_power_change = WAIT_FOR_POWER_CHANGE;
uint32_t lifetime_energy = 0;
IPAddress currentAddress = 0;

const char device_name[] PROGMEM = {"SolarAdapter"};
const char device_name1[] PROGMEM = {"SolarEVSE"};
const char header_1[] PROGMEM = {"<!DOCTYPE html><HTML lang='en'><HEAD><TITLE>Solar_TYPE_ UI</TITLE><META NAME='viewport' CONTENT='width=device-width, initial-scale=1.0'><STYLE> H1 {font-family:Arial;font-size:175%;}H3 {font-family:Arial;font-size:200%;line-height:10%}P {font-family:Arial;}.P3 {font-family:Arial;font-size:80%;}</STYLE>"};                 
const char header_2[] PROGMEM = {"</HEAD><BODY><H3><SPAN STYLE='color:#FF9000'>Solar</SPAN>_TYPE_</H3><P><B>by 2 Much Sun, LLC</B></P><H1>_LOCATION_</H1>"};
const char m_show[] PROGMEM = {"<P><I>The page is intended for advanced users to troubleshoot or set up external software to communicate with this device</I></P>"};
const char m_wps[] PROGMEM = {" AP is still a network choice.</P>"
                               "<P>If it is, then reconnect to it and go back to your browser to see the error to troubleshoot or try again.</P>"
                               "<P>If not, it means WPS was successful and this device"
                               " has connected to your network.  To verify or make changes, either type in "};
const char m_wpsr[] PROGMEM = {"<P>WPS Failed. Please try again or troubleshoot</P>"
                              "<FORM ACTION='/'>"  
                              "<P><INPUT TYPE=submit VALUE='Continue'></P>"
                              "</FORM></P>"
                              "</BODY></HTML>"};
const char m_settings1[] PROGMEM = {"<P>You can press the start WPS button on your router to start the process."
                                    "Then press the WPS button below within 2 minutes to finish. If successful, you will be connected to your network automatically</P>"
                                    "<P>Or fill in your network information manually.</P>"
                                    "<FORM ACTION='/wps'>"
                                    "<INPUT TYPE=submit VALUE='  WPS Button  '>"
                                    "</FORM>"};
const char m_settings2[] PROGMEM = {"</P>"
                                    "<P><LABEL><I>&nbsp;&nbsp;&nbsp;&nbsp;or enter SSID manually:</I></LABEL><INPUT SIZE='32' NAME='hssid' MAXLENGTH='32' VALUE='empty'></P>"
                                    "<P><LABEL><I>Password:</I></LABEL><INPUT TYPE='password' SIZE = '64' NAME='pass' MAXLENGTH='64' VALUE='"};

const char m_settings3[] PROGMEM = {" by selecting 'Home' page).</P>"
                                    "<P>If you do want to send data, then please fill in the info above and select 'Save'.</P>"
                                    "<P>After you successfully connected to your network, "
                                    "please select 'Network Configuration' and fill in "
                                    "the appropiate information about the database servers.</P>"};
const char m_settings4[] PROGMEM = {"<P><I>Detected PowerMC that does not match saved one. Please either save a new one or check on the saved one "
                                    "<SELECT NAME='pmbase'>"
                                    "<OPTION VALUE='1'SELECTED>Use originally saved one</OPTION>"
                                    "<OPTION VALUE='0'>PowerMC("};
const char m_rapi[] PROGMEM = {"<P>Common Commands - case sensitive</P>"
                                "<P>Enable - $FE</P>"
                                "<P>Sleep - $FS</P>" 
                                "<P>Reset - $FR</P>"
                                "<P>Current - $SC XX Y (Y = N for don't save)</P>"
                                "<P>Service Level - $SL 1 - $SL 2</P>"
                                "<TABLE><TR>"
                                "<TD><FORM ACTION='/r'><P><LABEL><B><I>Send RAPI Command (press return to see results):</I></B></LABEL><INPUT NAME='rapi' MAXLENGTH='32'></P></FORM></TD></TR><TR>"
                                "<TD><FORM ACTION='/'><INPUT TYPE=submit VALUE='    Home   '></FORM></TD>"
                                "</TR></TABLE>"};                       
const char m_confirm[] PROGMEM = {"<P>You are about to erase some of the network configuration and advanced settings and reset to default settings!</P>"
                                  "<P>The last connection's SSID and password will remain as this is not stored in the user memory</P>"
                                  "&nbsp;<TABLE><TR>"
                                  "<TD><FORM ACTION='/reset'><INPUT TYPE=submit VALUE='    Continue    '></FORM></TD>"
                                  "<TD><FORM ACTION='/'><INPUT TYPE=submit VALUE='    Cancel    '></FORM></TD>"
                                  "</TR></TABLE>"
                                  "</BODY></HTML>"};
const char m_confirm2[] PROGMEM = {"</P>"
                                  "&nbsp;<TABLE><TR>"
                                  "<TD><FORM ACTION='/reset2'><INPUT TYPE=submit VALUE='    Continue    '></FORM></TD>"
                                  "<TD><FORM ACTION='/'><INPUT TYPE=submit VALUE='    Cancel    '></FORM></TD>"
                                  "</TR></TABLE>"
                                  "</BODY></HTML>"};
const char m_rst[] PROGMEM = {" will reboot and reconnect automatically to last connected network</P>"
                              "<P>If unsuccessful, the following AP will be activated so that one can update the credentials</P>"
                              "<P>SSID:"};
const char m_success[] PROGMEM = {"<FORM ACTION='/'>" 
                                  "<P>Success!</P>"
                                  "<P><INPUT TYPE=submit VALUE='     OK     '></P>"
                                  "</FORM>"
                                  "</BODY></HTML>"};
const char m_home[] PROGMEM = {"&nbsp;<TABLE><TR>"
                              "<TD><FORM ACTION='/set'><INPUT TYPE=submit VALUE='Network Configuration'></FORM></TD>"
                              "<TD><FORM ACTION='/adv'><INPUT TYPE=submit VALUE='   Advanced   '></FORM></TD>"
                              "</TR></TABLE>"
                              "<BR><FORM ACTION='/ab'>"
                              "<INPUT TYPE=submit VALUE='    About    '>"
                              "</FORM>"
                              "<BR><FORM ACTION='/confirm2'>"
                              "<INPUT TYPE=submit VALUE='    Reboot    '>"
                              "</FORM>"
                              "</BODY></HTML>"};         
const char m_submit[] PROGMEM = {"<INPUT TYPE=submit VALUE='    Submit    '></FORM>"
                              "&nbsp;<TABLE><TR>"
                              "<TD><FORM ACTION='/'><INPUT TYPE=submit VALUE='    Cancel    '></FORM></TD>"
                              "</TR></TABLE>"
                              "</BODY></HTML>"};
const char m_about[] PROGMEM = {"<P>SolarAdapter/EVSE is owned, developed and maintained by 2 Much Sun, LLC in Atlanta, Georgia - Copyright (c) 2024-2026 by 2 Much Sun, LLC</P>"
                                "<P>SolarAdapter/EVSE is an original work using the following open source software unmodified and licensed under the MIT License</P>"
                                "<P>PubSubClient - Copyright (c) 2008-2020 Nicholas O'Leary</P>"
                                "<P>ArduinoJson - Copyright (c) 2014-2024 Benoit Blanchon</P>"
                                "<P>The following open source software is modified and licensed under the GNU General Public License (GPL)<P>"
                                "<P>Open EVSE Firmware - Copyright (c) 2011-2023 Sam C. Lin, Copyright (c) 2011-2014 Chris Howell <chris1howell@msn.com>"
                                "<P>The rest of the open source software is unmodified and licensed under the GNU Lesser General Public License (LGPL)<P>"
                                "<P>ESP8266 Arduino Core</P>"
                                "<P>Arduino Core Libraries</P>"
                                "<P><A href = 'https://github.com/2MuchSun/SolarEVSE-Adapter.git'>Source Code<P>"
                                "<BR><FORM ACTION='/'>"
                                "<INPUT TYPE=submit VALUE='Home'>"
                                "</FORM>"
                                "</BODY></HTML>"};
const char m_updates[] PROGMEM = {"<BR><INPUT TYPE=submit VALUE='    Save    '>"
                                  "</FORM><BR><FORM ACTION='/dbg'><P>&nbsp;&nbsp;<INPUT TYPE=submit NAME='submit' VALUE='Check for Updates'></P>"
                                  "<P STYLE='COLOR:#00A000'>Note. If it hangs after pressing this button, then it means an update is being installed. Please wait about 2 minutes before selecting 'Done'.</P>"
                                  "</FORM>"
                                  "&nbsp;<TABLE><TR>"
                                  "<TD><FORM ACTION='/'><INPUT TYPE=submit VALUE='    Done    '></FORM></TD>"
                                  "</TR></TABLE>"};
const char m_show_data[] PROGMEM = {" or when any settings change</I></P>"
                                    "<P><B>Topic: </B>_BASE2_st<B> Payload: </B>JSON DICT _OUT1_</P>"
                                    "<P><I>The following actual data is published to the MQTT Broker at _MBROKER_ every _RATE_"
                                    " secs or when the priority changes</I></P>"
                                    "<P><B>Topic: </B>_BASE_devs<B> Payload: </B>JSON DICT _OUT2_</P>"
                                    "<P><B>Topic: </B>_BASE_slow_load<B> Payload: </B> _SLOWLOAD_</P>"
                                    "<P><I>The following actual data is published to the MQTT Broker at _MBROKER_ when this device has completed its actions</I></P>"
                                    "<P><B>Topic: </B>_BASE_done<B> Payload: </B>_AINDEX_ (assigned index)</P>"
                                    "<P><I>The following data is published to the MQTT Broker at _MBROKER_ when this device requests for a priority scan reset</I></P>"
                                    "<P><B>Topic: </B>_BASE_rp<B> Payload: </B> 1 for reset</P>"
                                    "<FORM ACTION='/'><P><INPUT TYPE=submit VALUE='  Home  '></P></FORM>"
                                    "</BODY></HTML>"};

const char m_show2[] PROGMEM = {"<P><B>Topic: </B>2ms/PM/devs<B> Payload: </B>JSON DICT ip:[string],dnum:[int 0-9], mac:[string]</P>"
                                "<P><B>Topic: </B>_BASE_s<B> Payload: </B> (int 0-1) </P>"
                                "<P><B>Topic: </B>_BASE_next<B> Payload: </B>JSON DICT NOD:[int 1-_MAX_ number of devices found], idx:[int (0-"
                                "_MAX1_) device's assigned index], m:[string device's MAC address], "
                                "d:[int 0 - above reference power, 1 - below reference power] (reference power is -offset), o:[-32,768 to + 32,768 - offset] "
                                "limit to import or export to grid, to:[int 0-255]</P>"
                                "<P><B>Topic: </B>_BASE_devs<B> Payload: </B>JSON DICT dpriority:[int 1-254], dname:[string], dnumber:[int 0-"
                                "_MAX1_], dtype:[string], mdns:[string], mac:[string], ipower:[int 0 - 65000], ip:[string]</P>"
                                "<P><B>Topic: </B>_BASE2_c/auto<B> Payload: </B> (int 1-autotracking 0-disable autotracking)</P>"  
                                "<P><B>Topic: </B>_BASE2_c/priority<B> Payload: </B> (int 1-254, device's priority)</P>"
                                "<P><B>Topic: </B>_BASE2_c/enabled<B> Payload: </B> (int 0-sleep, 1-enabled)</P>"
                                "<P><B>Topic: </B>_BASE2_c/bypass<B> Payload: </B> (int 0-use adapter, 1-bypass adapter/pass through)</P>"
                                "<P><B>Topic: </B>_BASE2_c/schedAT<B> Payload: </B> (int 0-turn off, 1-turn on auto tracking schedule, on if valid times are set in the web UI)</P>"
                                "<P><B>Topic: </B>_BASE2_c/timer<B> Payload: </B> (int 0-turn off, 1-turn on delay timer, on if valid times are set in the web UI)</P>"};

// SERVER variables
// determines server state
bool server2_down = true;    //MQTT server
bool ready_to_start = false; 
uint8_t server2_down_reconnect_attempt = 0;

//MQTT Broker
String base = DEFAULT_BASE;  
String consumed = DEFAULT_CONSUMED;
String generated = "";
String consumedTopic = "";
String generatedTopic = "";
String base2 = "";
String out1, out2;

uint8_t device = 0, no_mqtt = 0;

// for Adapter basic info
uint8_t adapter_enabled = 0;  //current mode - enabled or not
uint8_t evse_type = 0;        //0 is generic, other numbers are specific ones like 1 for JuiceBox
int32_t extra_power = 0;
float consumed_power = 0.0;
float generated_power = 0.0;
uint8_t pilot = 0, pilot_pin;
uint8_t adapter_flag = 0;
uint8_t plugged_in = 0;
uint8_t pre_plugged_in = 0;
int16_t offset = 0; //amount allowed to be over
uint16_t unplugged_hi = 0, unplugged_lo = 1023,plugged_in_hi = 0, plugged_in_lo = 1023, charge_hi = 0, charge_lo = 1023, unplugged_count = 0, plugged_in_count= 0, charge_count = 0;
uint8_t set_current = 0, alt_set_current = 0;
uint8_t pre_set_current = 0;
uint16_t ipower;
bool forced_reset_priority = false;
uint8_t timer_enabled = 0, double_time_limit = 0, start_hour, start_min, stop_hour, stop_min, time_zone, half_charge_limit = 0, previously_auto_mode = 0;
bool timer_started = false, limits_reached = false;

// for setting up own network and autotracking
uint8_t transition = 0;
String sw_config;
uint8_t hi_amp_setpoint;
uint8_t saved_hi_amp_setpoint;
uint8_t number_of_devices = 0;
uint8_t device_priority;
String slow_load = "0";
bool publish_done = false;
uint8_t down_scan = 0, temporarily_done = 0;
String location_name = "";
uint8_t current_done_index;
uint8_t assigned_index = 0;
bool pre_low = false;

uint8_t number_of_pms_found = 0;
uint8_t current_num_pm = 0;

uint8_t rec_pm_dnum[MAX_NUM_PMS];
String rec_pm_mac[MAX_NUM_PMS];
String rec_pm_ip[MAX_NUM_PMS];
uint8_t pub_pm_dnum[MAX_NUM_PMS];
String pub_pm_mac[MAX_NUM_PMS];
String pub_pm_ip[MAX_NUM_PMS];
uint8_t processed_pm_dnum[MAX_NUM_PMS];
String processed_pm_mac[MAX_NUM_PMS];
String processed_pm_ip[MAX_NUM_PMS];

String selected_pm_mac_address = "";
String selected_device_mac_address;
String received_device_mac_address;
String received_device_ip_address;
unsigned long minutes_charging = 0;

bool blink_LED1 = false;
bool blink_LED2 = false;

uint8_t first_ip_digit;
uint8_t second_ip_digit;
uint8_t third_ip_digit;
uint8_t digits;
uint8_t ip_position;
uint8_t total_ip;

uint8_t schedule_AT, start_AT, stop_AT;

int16_t turn_off_threshold;
int16_t turn_on_threshold;
int16_t increase_threshold;
int16_t decrease_threshold;

unsigned long Timer;
unsigned long Timer2;
unsigned long Timer3;
unsigned long Timer4;
unsigned long Timer6;
unsigned long Timer8;
unsigned long Timer_zero;
unsigned long Timer10;
unsigned long Timer11;
unsigned long Timer12;
unsigned long Timer20;
unsigned long m_LastCheck;
unsigned long Timeout_start_time;
uint8_t user_set_adapter_mode;   //when the user sets the adapter to either sleep or enabled
uint8_t adapter_state;
uint8_t vflag;

//for getting the time
time_t now;                         // this are the seconds since Epoch (1970) - UTC
tm tm;                              // the structure tm holds time information in a more convenient way

void header1(String& s){
  s = "";
  uint16_t Length = strlen_P(header_1);
  for (uint16_t k = 0; k < Length; k++) {
    s += char(pgm_read_byte_near(header_1 + k));
  }
  if (solar_evse)
    s.replace("_TYPE_","EVSE");
  else
    s.replace("_TYPE_","Adapter");
}

void header2(const String& page, String& s){
  String location_name;
  s = "";
  location_name = readEEPROM(LOCATION_START, LOCATION_MAX_LENGTH);
  if (location_name == "")
    location_name = "My Device";
  processReservedCharacters(location_name);
  uint16_t Length = strlen_P(header_2);
  for (uint16_t k = 0; k < Length; k++) {
    s += char(pgm_read_byte_near(header_2 + k));
  }
  s.replace("_LOCATION_",location_name + " " + page);
  if (solar_evse)
    s.replace("_TYPE_","EVSE");
  else
    s.replace("_TYPE_","Adapter");
}

String deviceName(){
  String s = "";
  uint8_t Length;
  if (solar_evse){
    Length = strlen_P(device_name1);
    for (byte k = 0; k < Length; k++) {
      s += char(pgm_read_byte_near(device_name1 + k));
    }
  }
  else {
    Length = strlen_P(device_name);
    for (byte k = 0; k < Length; k++) {
      s += char(pgm_read_byte_near(device_name + k));
    }
  }
  return s;
}

void getDateTimeString(uint8_t type, String& str) {
  // type 0 - both, 1 - date only, 2 - time only (24-hour format), 3 - time only (12-hour format)
  //get date and time
  String dst, hour, noon, monthS, dayS, hourS, minS, secS;
  time(&now);                       // read the current time
  localtime_r(&now, &tm);
  if (tm.tm_hour > 12){
    hour = String(tm.tm_hour - 12);
    noon = "PM";
  }
  else {
    hour = String(tm.tm_hour);
    if (tm.tm_hour == 12)
      noon = "PM";
    else
      noon = "AM";
    if (tm.tm_hour == 0)
      hour = "12";
  }
  if (tm.tm_isdst == 1)             // Daylight Saving Time flag
    dst = "(DST)";
  else
    dst = " ";
  nameOfDay(tm.tm_wday, dayS);
  nameOfMonth(tm.tm_mon, monthS);
  addLeadingZero(tm.tm_min, minS);
  addLeadingZero(tm.tm_sec, secS);
  addLeadingZero(tm.tm_hour, hourS);
  if (type == 0)
    str =  dayS + ", " + monthS + " " + String(tm.tm_mday) + ", " + String(tm.tm_year + 1900) + ", " + hour + ":" + minS + ":" + secS + " " + noon + " " + dst; // format Friday, June 3, 2016, 1:20:34 PM (DST)
  if (type == 1)
    str = dayS + ", " + monthS + " " + String(tm.tm_mday) + ", " + String(tm.tm_year + 1900) ; // format Friday, June 3, 2016
  if (type == 2)
    str = hourS + ":" + minS +  " " + dst; // format 14:20 (DST)
  if (type == 3)
    str = hour + ":" + minS +  " " + noon + " " + dst; // format 1:20 PM (DST)
}

//Starts the WPS configuration
bool startWPS() {
  //Serial.println("WPS Configuration Started");
  digitalWrite(RED_LED_0,LOW); 
  bool wpsSuccess = WiFi.beginWPSConfig();   
  if (wpsSuccess) {       //Doesn't always have to be successful! After a timeout, the SSID is empty       
    String newSSID = WiFi.SSID();       
    if (newSSID.length() > 0) {         //Only when an SSID was found were we successful          
      //Serial.printf("WPS done. Successfully logged in to SSID '%s'", newSSID.c_str());
      uint8_t attempt = 0;
      while ((WiFi.status() != WL_CONNECTED) && (attempt < 80)) {   // try for about 20 secs
        delay(250);
        attempt++;
      }
      if (!WiFi.isConnected())
        wpsSuccess = false;
    } 
    else {         
      wpsSuccess = false;
      //Serial.println("WPS Configuration failed");   
    }
  }
  return wpsSuccess; 
}

void processReceivedNextData(char* payload) {
  char* tmpj;
  JsonDocument doc;
  tmpj = payload;
  deserializeJson(doc,tmpj);
  number_of_devices = doc["NOD"];
  current_done_index = doc["idx"];
  down_scan = doc["d"];
  received_device_mac_address = String(doc["m"]);
  selected_device_mac_address = received_device_mac_address;
  received_device_ip_address = String(doc["ip"]);
  offset = doc["o"];
  received_time_on = doc["to"];
}

void processReceivedDeviceData(char* payload) {
  char* tmpj;
  JsonDocument doc;
  tmpj = payload;
  deserializeJson(doc,tmpj);
  String tmpStr = doc["mdns"];
  String tmpStr2 = doc["mac"];
  checkDuplicateDeviceNumbers(tmpStr, tmpStr2);
}

void bootOTA() {
  //Serial.println("<BOOT> OTA Update");
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
   String mDNS;
   if (solar_evse)
    mDNS = APSSID_E;
   else
    mDNS = APSSID;
   mDNS += device;
   ArduinoOTA.setHostname(mDNS.c_str());

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() {
    //Serial.println("Start");
    boot_count--;  // don't count OTA updates as real reboots
    writeEEPROMbyte(BOOT_START,boot_count);  
  });
  ArduinoOTA.onEnd([]() {
    //Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    //Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    /*Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");*/
  });
  ArduinoOTA.begin();
  //Serial.println("Ready");
  //Serial.print("IP address: ");
  //Serial.println(WiFi.localIP());
}

void addLeadingZero(int number, String& string_name) {
  if (number < 10){
    switch (number) {
      case 0:
        string_name = "00";
        break;
      case 1:
        string_name = "01";
        break;
      case 2:
        string_name = "02";
        break;
      case 3:
        string_name = "03";
        break;
      case 4:
        string_name = "04";
        break;
      case 5:
        string_name = "05";
        break;
      case 6:
        string_name = "06";
        break;
      case 7:
        string_name = "07";
        break;
      case 8:
        string_name = "08";
        break;
      case 9:
        string_name = "09";
        break;
    }
  }
  else
    string_name = String(number);
}

void displayConnectionStatus(String& s1) {
  String tmpS;
  char tmpStr[40];
  IPAddress myAddress = WiFi.localIP();
  String mDNS;
  if (solar_evse)
    mDNS = APSSID_E;
  else
    mDNS = APSSID;
  mDNS += device;
  if (WiFi.isConnected()) {
      getDateTimeString(0, tmpS);
      s1 = "<P CLASS='P3'>Today is " + tmpS + "</P>";
      s1 += "<P CLASS='P3'>Connected to ";
      long rssi = WiFi.RSSI();
      sprintf(tmpStr,"(%d.%d.%d.%d) with strength ",myAddress[0],myAddress[1],myAddress[2],myAddress[3]);
      s1 += WiFi.SSID();
      s1 += " at ";
      if (WiFi.SSID() != DEFAULTSSID)
        s1 += mDNS + ".local ";
      s1 += tmpStr;
      if (rssi >= -72)
        s1 += "<SPAN STYLE='COLOR:#00A000'>"; //green
      else if (rssi >= -78)
        s1 += "<SPAN STYLE='COLOR:#FFB000'>"; //yellow
      else 
        s1 += "<SPAN STYLE='COLOR:#FF0000'>"; //red
      s1 += rssi;
      s1 += " dBm</SPAN>";
      s1 += "</P><P CLASS='P3'>";
      if (!no_mqtt){
        if (server2_down)
          s1 += "<SPAN STYLE='COLOR:#FFB000'>MQTT Broker is not reachable (Error code #" + String(client.state()) + ")  Trying again...</SPAN>";
        else {
          if (number_of_pms_found == 0)
            s1 += "<SPAN STYLE='COLOR:#00A000'>MQTT Broker is connected with no PowerMC found</SPAN>";
          else {
            s1 += "<SPAN STYLE='COLOR:#00A000'>MQTT Broker is connected and updated every ";
            s1 += String(wait_for_power_change/2000.0,1);
            if (wait_for_power_change == 2000)
              s1 += " sec";
            else
              s1 += " secs";
            s1 += "</SPAN>";
          }
        }
      }
      else
        s1 += "MQTT is not being used.  To use, see Network Configuration";
      s1 += "</P>";   
  }
  else {
    s1 = "<P CLASS='P3'>";
    s1 += deviceName();
    s1 += " is in AP mode - SSID:";
    if (solar_evse)
      s1 += APSSID_E;
    else
      s1 += APSSID;
    s1 += " at 192.168.4.1";
    s1 += "</P>";
  }
}

void networkScan(String& st) {
  uint8_t n = WiFi.scanNetworks();
  char tmpStr[32];
  //Serial.print(n);
  //Serial.println(" networks found");
  st = "<SELECT NAME='ssid'>";
  uint8_t found_match = 0;
  String tmp;
  for (uint8_t i = 0; i < n; ++i) {
    st += "<OPTION VALUE='";
    tmp = WiFi.SSID(i);
    processReservedCharacters(tmp);
    st += tmp + "'";
    if (WiFi.SSID(i) == WiFi.SSID()) {
      found_match = 1;
      //Serial.println("found match");
      st += " selected";
    }
    sprintf(tmpStr," (%d dBm) %s", WiFi.RSSI(i), WiFi.encryptionType(i) == ENC_TYPE_NONE ? "open" : "");
    st += "> " + tmp + String(tmpStr);
    st += " </OPTION>";
  }
  if (!found_match){
    //Serial.println("inside no match found");
    if (!n)
        st += "<OPTION VALUE='not chosen' selected> No Networks Found!  Select Rescan or Manually Enter SSID</OPTION>";
    else
        st += "<OPTION VALUE='not chosen' selected> Choose One </OPTION>";
  }
  st += "</SELECT>";
}

void connectWiFi() {
  // 1st try to connect to connect last saved network, if unsuccessful, try default network, takes about 20 secs, If unsuccessful, activate WPS which takes about 150 seconds, finally if unsuccessful reboot into AP mode
  processWiFiConnectionAttempt();
  //if not successful, then try default
  if (!WiFi.isConnected()) {
    initialConnectionResults = lastConnectionResults;
    //Serial.println("auto connect unsuccessful, trying default");
    WiFi.persistent(false);
    WiFi.begin(DEFAULTSSID,DEFAULTPASS);
    processWiFiConnectionAttempt();
  }
/*  Serial.println();
  Serial.print("Number of attempts is ");
  Serial.println(attempt);*/
  if (!WiFi.isConnected()){
    WiFi.mode(WIFI_STA);  //must be WIFI_STA for wps to work
    if (!startWPS()){
      //Serial.println("finished startWPS");
      rebootIntoAPMode();  //failed
    }
    else{
      reboot = 1;  //passed
    }
  }      
}

void processReservedCharacters(String& s1) {
  s1.replace("&","&#38;");
  s1.replace("\"","&#34;");
  s1.replace("'","&#39;");
  s1.replace("<","&#60;");
  s1.replace(">","&#62;");
}

void startAPMode() {
  connection_mode = STARTED_IN_AP; 
  writeEEPROMbyte(START_AP_START,NONE);
  WiFi.mode(WIFI_AP_STA);
  if (solar_evse)
    WiFi.softAP(APSSID_E, APPASS);
  else
    WiFi.softAP(APSSID, APPASS);
  //Serial.println("Now in AP Mode");
  //Serial.println(WiFi.status());
}

void processWiFiConnectionAttempt() {
  uint8_t attempt = 0;
  //turn on yellow LED as status of trying to connect
  digitalWrite(YELLOW_LED,LOW);
  bool blink_on = false;
  //Serial.println(".");
  while ((WiFi.status() != WL_CONNECTED) && (WiFi.status() !=  WL_WRONG_PASSWORD) && (attempt < 30)) {   // try for about 30 secs
    delay(SLOW_BLINK_RATE);
    if (blink_on){
      digitalWrite(YELLOW_LED,LOW);
      blink_on = false;
    }
    else {
      digitalWrite(YELLOW_LED,HIGH);
      blink_on = true;
    }
    //Serial.print(WiFi.status());
    attempt++;
  }
  //Serial.println("done trying");
  digitalWrite(YELLOW_LED,HIGH);
  switch (WiFi.status()){
    case  WL_CONNECTED:
       lastConnectionResults = "";
       //Serial.println("The status is connected");
      break;
    case WL_CONNECT_FAILED:  //connection failed
      lastConnectionResults = "The last connection attmept failed";
      //Serial.println("The status is connection failed");
      break;
    case WL_WRONG_PASSWORD:  //password is incorrect
      lastConnectionResults = "The last connection attempt used an incorrect password";
      //Serial.println("The password is incorrect");
      break;
    case WL_NO_SSID_AVAIL:
      lastConnectionResults = "The last connection attempt used an unavailable SSID";
      //Serial.println("The status is no SSID Available");
      break;
    case WL_IDLE_STATUS: //in process of changing status
      lastConnectionResults = "The last connection attempt was idling";
      //Serial.println("The status is idling");
      break;
    case WL_DISCONNECTED:
      lastConnectionResults = "The last connection attempt was disconnected";
      //Serial.println("The status is disconnected");
      break;
    case -1:
    default:
      lastConnectionResults = "The last connection attempt timed out";
      //Serial.println("Connection timeout");
      break;
  }
}

void setDelayTimer(uint8_t setting) {
  if (setting != timer_enabled){
    if (setting){
      if (IsTimerValid())
        timer_enabled = 1;
    }
    else {
      timer_enabled = 0;
    }
    if (IsTimerValid())
      writeEEPROMbyte(TIMER_ENABLED_START,timer_enabled);
    immediate_status_update = true;
  }
}

void setAutoSchedule(uint8_t setting) {
  if (setting != schedule_AT){
    if (setting){
      if (start_AT != stop_AT)
        schedule_AT = 1;
    }
    else {
      schedule_AT = 0;
    }
    immediate_status_update = true;
    writeEEPROMbyte(SCHEDULE_AT_START,schedule_AT);
  }
}

void publishResetPriority() {
   String tmp,tmp2;
   if (forced_reset_priority){
     publish_done = false;
     forced_reset_priority = false;
     tmp2 = base + "rp";
     tmp = "1";
     if (!server2_down){
       if (!client.publish(tmp2.c_str(),tmp.c_str()))
          server2_down = true;
     }    
   }
}

void publishDone(){
   String tmp2;
   uint8_t a_index;
   if (publish_done){
     publish_done = false;
     selected_device_mac_address = "0";
     tmp2 = base + "done";
     if (temporarily_done == 1)    //switching only but temporarily give up control to other loads while switching service level
      a_index = assigned_index + 100;
     else
      a_index = assigned_index;   
     if (!server2_down){
        if (!client.publish(tmp2.c_str(),String(a_index).c_str()))
          server2_down = true;
     }
   }
}

uint8_t calculateEVSEPilot(uint16_t usec_time) {
  uint8_t evse_pilot;
  if (usec_time <= 850)
    evse_pilot = int(0.6*usec_time/10.0 + 0.5);
  else
    evse_pilot = int(2.5*(usec_time - 640)/10.0 + 0.5);
  if (evse_pilot < MIN_CURRENT_CAPACITY_J1772 || evse_pilot > 90)
    evse_pilot = 0;
  return evse_pilot;
}

uint8_t checkEVSEStatus() {
  uint16_t p_interval_time;
  uint8_t set_pilot = 0;
  unsigned long base_time = millis();
  interval_time = 0;

  digitalWrite(PLUG_IN, HIGH);
  attachInterrupt(digitalPinToInterrupt(pilot_pin), EvsePilotISR, CHANGE);  
  delay(5);
  while (set_pilot == 0 && (millis() - base_time) < 50){
    if (interval_time != 0){ //if false, then no interrupt happened so PE is not transitioning ie PWM is off or PE not connected to EVSE
      p_interval_time = interval_time;
      if (abs(p_interval_time - interval_time) < 10 && interval_time < 1005 && interval_time > 5)  // checks okay
        set_pilot = calculateEVSEPilot(interval_time);
      else {
        set_pilot = 0;  // evse is not following J1772 standard
      }
    }
  }
  if (!plugged_in)
    digitalWrite(PLUG_IN, LOW);
  detachInterrupt(digitalPinToInterrupt(pilot_pin));
  if (solar_evse && (evse_type == JUICEBOX)){
    if ((evse_max > set_pilot) && (set_pilot >= MIN_CURRENT_CAPACITY_J1772)){
      set_pilot = evse_max;
      override_evse_set_pilot = true;
    }
    else
      override_evse_set_pilot = false;
  }
  if (alt_set_current >= MIN_CURRENT_CAPACITY_J1772)           //if not auto detect
    set_pilot = alt_set_current;
  return set_pilot;
}

String readEEPROM(uint16_t start_byte, uint16_t allocated_size) {
  String variable;
  for (uint16_t i = start_byte; i < (start_byte + allocated_size); ++i) {
    variable += char(EEPROM.read(i));
  }
  delay(10);
  return variable.c_str();  //needs to be constant char to filter out any control characters that was used for padding
}

uint8_t readEEPROMbyte(uint16_t start_byte) {
  return EEPROM.read(start_byte);
}

uint16_t readEEPROMword(uint16_t start_byte) {
  uint16_t tmp = readEEPROMbyte(start_byte) << 8;
  uint16_t value = tmp + readEEPROMbyte(start_byte + 1);
  return value;
}

void writeEEPROM(uint16_t start_byte, uint16_t allocated_size, String contents) {
  uint8_t length_of_contents = contents.length();
  if (length_of_contents > allocated_size)
    length_of_contents = allocated_size;
  for (uint8_t i = 0; i < length_of_contents; ++i) {
    EEPROM.write(start_byte + i, contents[i]);
  }
  for (uint16_t i = (start_byte + length_of_contents); i < (start_byte + allocated_size); ++i)
    EEPROM.write(i,0);
  EEPROM.commit();
  //Serial.println("inside writeEEPROM function");
  delay(10);
}

void writeEEPROMbyte(uint16_t start_byte, uint8_t contents) {
  EEPROM.write(start_byte, contents);
  EEPROM.commit();
  //Serial.println("inside writeEEPROMbyte function");
  delay(10);
}

void writeEEPROMword(uint16_t location, uint16_t value) {
  uint8_t byte = value >> 8;
  writeEEPROMbyte(location, byte);
  byte = value & 0x00ff;
  writeEEPROMbyte(location + 1, byte);
}

void resetEEPROM(uint16_t start_byte, uint16_t end_byte) {
  //Serial.println("Erasing EEPROM");
  for (uint16_t i = start_byte; i < end_byte; ++i) {
   EEPROM.write(i, 0);
    //Serial.print("#"); 
  }
  EEPROM.commit();
  writeEEPROM(MEMORY_CHECK_START, MEMORY_CHECK_MAX_LENGTH, "B");  //use to indicate if memory is ready  
}

void handleWPS() {
  String s, hStr;
  String mDNS;
   if (solar_evse)
    mDNS = APSSID_E;
   else
    mDNS = APSSID;
  mDNS += device;
  digitalWrite(RED_LED_0,LOW);
  header1(s);
  s += "<META HTTP-EQUIV='refresh' CONTENT='3; URL=/wpsr'>"; 
  header2("WPS", hStr);
  s += hStr;
  s += "<P>Getting network info and connecting...</P>";
  wps_check = 0;
  s += "<P>Please wait about 20 secs and then check to see if the ";
  if (solar_evse)
    s += APSSID_E;
  else
    s += APSSID;
  for (uint16_t k = 0; k < strlen_P(m_wps); k++) {
    s += char(pgm_read_byte_near(m_wps + k));
  }
  s +=  mDNS + ".local ";
  s += "or the IP address assigned by your network to the ";
  s += deviceName();
  s += " in your browser.</P>";
  s += "</BODY></HTML>";
  server.send(200, "text/html", s);
}

void rebootIntoAPMode() {
  //Serial.println("inside rebootIntoAPMode()");
  writeEEPROMbyte(START_AP_START, AP);
  reboot = 1;
}

void handleWPSR() {
  String s, hStr;
  header1(s);
  s += "<META HTTP-EQUIV='refresh' CONTENT='10; URL=/wpsr'>";
  header2("WPS Status", hStr);
  s += hStr;
  WiFi.mode(WIFI_STA);  //must be WIFI_STA for wps to work
  if (wps_check == 0){
    if (!startWPS())
      wps_check = 1;  //failed
    else{
      wps_check = 2;  //passed
    }
  }
  //WiFi.softAPdisconnect(true);
  if (wps_check == 1) {
    //Serial.println("WPS Configuration failed");
    for (uint16_t k = 0; k < strlen_P(m_wpsr); k++) {
      s += char(pgm_read_byte_near(m_wpsr + k));
    }
    server.send(200, "text/html", s);
    rebootIntoAPMode();
  } 
  else if (wps_check == 2) {
    //Serial.println("WPS Configuration passed, connected to " + WiFi.SSID());
    reboot = 1;
  }
}

void handleCfm() {
  String s, hStr;
  header1(s);
  header2("Confirmation", hStr);
  s += hStr;
  for (uint16_t k = 0; k < strlen_P(m_confirm); k++) {
        s += char(pgm_read_byte_near(m_confirm + k));
      }
  server.send(200, "text/html", s);
}

void handleCfm2() {
  String s, hStr;
  header1(s);
  header2("Confirmation", hStr);
  s += hStr;
  s += "<P>You are about to reboot the ";
  s += deviceName();
  for (uint16_t k = 0; k < strlen_P(m_confirm2); k++) {
    s += char(pgm_read_byte_near(m_confirm2 + k));
  }
  server.send(200, "text/html", s);
}

void handleSettings() {
  String s, hStr, MQTTBroker,uName,pWord,port,tmpStr;
  port = readEEPROM(MQTT_PORT_START, MQTT_PORT_MAX_LENGTH);
  if (port == "")  
    port = DEFAULTMPORT;
  uName = readEEPROM(UNAME_START, UNAME_MAX_LENGTH);
  pWord = readEEPROM(PWORD_START, PWORD_MAX_LENGTH);
 //MQTT Broker
  MQTTBroker = readEEPROM(MQTTBROKER_START, MQTTBROKER_MAX_LENGTH);
  if (MQTTBroker == "")
    MQTTBroker = DEFAULTMSERVER;
  if (WiFi.SSID() == DEFAULTSSID){
    MQTTBroker = POWERMC_MSERVER;
    port = DEFAULTMPORT;
  }
  //Serial.println("inside handle settings");    
  if ((server.arg("change") == "Change WiFi Network") || (server.arg("rescan") == "Rescan")){
    change_network = true;
  }
  if (server.arg("cancel") == "   Cancel   "){
    change_network = false;
  }
  if (server.arg("submit") == "Disable MQTT") {          // 1 - no MQTT
    no_mqtt = 1;
    writeEEPROMbyte(NO_MQTT_START,no_mqtt);
    client.disconnect();
    server2_down = true;
    if (g_EvseController.IsAutomaticMode())
      g_EvseController.Sleep();
  }
  if (server.arg("submit") == " Enable MQTT ") {         // 0 - use MQTT
    no_mqtt = 0;
    writeEEPROMbyte(NO_MQTT_START,no_mqtt);
  }
  Timer12 = millis();   //reset LED timeout
  header1(s);
  header2("Network Configuration", hStr);
  s += hStr;
  s += "<P CLASS='P3'>Firmware version ";
  s += VERSION;
  s += "</P>";
  if (!change_network){
    displayConnectionStatus(tmpStr);
    s += tmpStr;
  }
  if (change_network || !WiFi.isConnected()){
    s += "<P>________________________________________</P>";
    s += "<P><B>WiFi Setup</B></P>";
    if (initialConnectionResults != "")
      s += "<P>Result from previous Auto Connect attempt: " + initialConnectionResults + "</P>";
    if (lastConnectionResults != "")
      s += "<P>Result from last connection attempt: " + lastConnectionResults + "</P>";
    if (!WiFi.isConnected()){
      for (uint16_t k = 0; k < strlen_P(m_settings1); k++) {
        s += char(pgm_read_byte_near(m_settings1 + k));
      }
    }
    s += "<FORM ACTION='/set'>";  
    s += "<INPUT TYPE=submit NAME = 'rescan' VALUE='Rescan'>";
    s += "</FORM>";
    s += "<FORM METHOD='post' ACTION='/cfg'>";
    if (WiFi.isConnected())
      s += "<P><I>Currently connected to </I>";
    else
      s += "<P><I>Choose a network </I>";
    networkScan(tmpStr);
    s += tmpStr;
    for (uint16_t k = 0; k < strlen_P(m_settings2); k++) {
        s += char(pgm_read_byte_near(m_settings2 + k));
      }
    tmpStr = WiFi.psk();
    processReservedCharacters(tmpStr);
    s += tmpStr + "'></P>";
    s += "<P>________________________________________</P>";
  }
  else {
    s += "<FORM ACTION='/set'>";  
    s += "<INPUT TYPE=submit NAME='change' VALUE='Change WiFi Network'>";
    s += "</FORM>";
  }
  if (!WiFi.isConnected()){
    s += "<P>Note. You are not connected to any network so no data will be sent ";
    s += "out. However, you can still control ";
    s += deviceName();
    for (uint16_t k = 0; k < strlen_P(m_settings3); k++) {
        s += char(pgm_read_byte_near(m_settings3 + k));
      }
  }
  if (WiFi.isConnected() && !change_network){
    s += "<P>________________________________________</P>";
    s += "<P><B>MQTT Server:</B> to publish the ";
    s += deviceName();
    s += " status & subscribe to power and control data</P>";
    s += "<FORM ACTION='/set'>";
    if (no_mqtt == 0) {
      s += "<P>MQTT enabled";
      s += "&nbsp;&nbsp;<INPUT TYPE=submit NAME='submit' VALUE='Disable MQTT'></P>";
    }
    else {
      s += "<P>MQTT disabled";
      s += "&nbsp;&nbsp;<INPUT TYPE=submit NAME='submit' VALUE=' Enable MQTT '></P>";
    }
    s += "</FORM>";
    s += "<FORM METHOD='post' ACTION='/cfg'>";
    if (!no_mqtt){
      s += "<P>Please fill in the information below on how to get or post data:</P>"; 
      s += "<P><LABEL><I> MQTT Broker Address (default is ";
      s += DEFAULTMSERVER;
      s += "):</I></LABEL><INPUT SIZE='50' NAME='MQTTBroker' MAXLENGTH='50' VALUE='";
      tmpStr = MQTTBroker;
      processReservedCharacters(tmpStr);
      s += tmpStr + "'></P>";
      
      s += "<P><LABEL><I>Port number (0-65535) (default is ";
      s += DEFAULTMPORT;
      s += "): </I></LABEL><INPUT TYPE='number' MIN='0' MAX='65535' NAME='port' VALUE='";
      s += port;
      s += "'></P>";
      
      s += "<P><LABEL><I>If required, Username: </I></LABEL><INPUT SIZE='32' NAME='uname' MAXLENGTH='32' VALUE='";
      tmpStr = uName;
      processReservedCharacters(tmpStr);
      s += tmpStr + "'></P>";  
      
      s +=  "<P><LABEL><I>If required, Password: </I></LABEL><INPUT TYPE='password' SIZE='32' NAME='pword' MAXLENGTH='32' VALUE='";
      tmpStr = pWord;
      processReservedCharacters(tmpStr);
      s += tmpStr + "'></P>";
      
      s += "<P><B>PowerMC:</B> to provide power data and coordinate each device on when to use the extra power (please wait at least ";
      s += String(PM_CYCLE_TIME/1000) + " secs before refreshing this page to detect all available)</P>";
      if (number_of_pms_found == 1){
        pub_pm_mac[0] = processed_pm_mac[0];
        pub_pm_dnum[0] = processed_pm_dnum[0];
        pub_pm_ip[0] = processed_pm_ip[0];
        if (selected_pm_mac_address != "0")
          s += "<P><I>Using a PowerMC found on the network at " + pub_pm_ip[0];
        else {
          for (uint16_t k = 0; k < strlen_P(m_settings4); k++) {
            s += char(pgm_read_byte_near(m_settings4 + k));
          }
          s += String(pub_pm_dnum[0]) + ") at " + pub_pm_ip[0] + "</OPTION></SELECT>";
        }
        s += "</I></P>";
      }
      if (number_of_pms_found == 0){
        s += "<P><I>Could not find a PowerMC. Check to make sure the PowerMC is using the same broker as this device. </I></P>";
      }
      else if (number_of_pms_found > 1){
        s += "<P><I>Detected multiple PowerMCs. Currently using ";
        s += "<SELECT NAME='pmbase'>";
        for (int index = 0; index < number_of_pms_found; index++) {
           pub_pm_mac[index] = processed_pm_mac[index];
           pub_pm_dnum[index] = processed_pm_dnum[index];
           pub_pm_ip[index] = processed_pm_ip[index];
           if (selected_pm_mac_address == pub_pm_mac[index]) {
             s += "<OPTION VALUE='" + String(index) + "'SELECTED>" + "PowerMC(" + String(pub_pm_dnum[index]) + ") at " + pub_pm_ip[index] + "</OPTION>";
           }
           else
             s += "<OPTION VALUE='" + String(index) + "'>" + "PowerMC(" + String(pub_pm_dnum[index]) + ") at " + pub_pm_ip[index] + "</OPTION>";
        }
        s += "</SELECT></I></P>";
      }
      
      s += "<P>________________________________________</P>";
      s += "<P><LABEL><I>Power consumed/extra sensor topic or JSON key (default is main): </I></LABEL><INPUT SIZE='20' NAME='consumed' MAXLENGTH='20' VALUE='";
      tmpStr = consumed;
      processReservedCharacters(tmpStr); 
      s += tmpStr + "'></P>";
      
      s +=  "<P><LABEL><I>Power generated sensor topic or JSON key (default is solar): </I></LABEL><INPUT SIZE='20' NAME='generated' MAXLENGTH='20' VALUE='";
      tmpStr = generated;
      processReservedCharacters(tmpStr);
      s += tmpStr + "'></P>";
      
      s += "<P><I>Data format for power data is <INPUT TYPE='radio' NAME='swconfig' VALUE='a'";
      if (sw_config == "a") //JSON
        s += " CHECKED";
      s += ">JSON (default)   <INPUT TYPE='radio' NAME='swconfig' VALUE='b'";
      if (sw_config == "b") //none
        s += " CHECKED";
      s += ">traditional</I></P>";
      s += "<P>________________________________________</P>";
    }
  }
  s += "<P><LABEL><I>Device # (default is 0): </I></LABEL><SELECT NAME='device'>"; 
  for (uint8_t i = 0; i < MAX_NUM_DEVICES; ++i) {
    s += "<OPTION VALUE='" + String(i) + "'";
    if (device == i)
      s += "selected='SELECTED'";
    s += ">" + String(i) + "</OPTION>";
  }
  s += "</SELECT></P>";
  s += "<INPUT TYPE=submit VALUE='    Save    '></FORM>";
  s += "<BR><TABLE><TR>";
  if (!change_network) {
    s += "<TD><FORM ACTION='/'><INPUT TYPE=submit VALUE='    Home   '></FORM></TD>";
  }
  else {
    s += "<TD><FORM ACTION='/set'><INPUT TYPE=submit NAME='cancel' VALUE='   Cancel   '></FORM></TD>";
  } 
  if (!change_network) {    
    s += "<TD><FORM ACTION='/confirm'>";  
    s += "<INPUT TYPE=submit VALUE='Erase Settings'>";
    s += "</FORM></TD>";
  }
  s += "</TR></TABLE><BR>";
  s += "</BODY>";
  s += "</HTML>";
	server.send(200, "text/html", s);
}

void getStateText (uint8_t state, String& state_text) {
  state_text = "";
  switch (state){
    case ADAPTER_STATE_A:
      state_text = "ready";
      break;
    case ADAPTER_STATE_B:
      state_text = "ready, waiting for car";
      break;
    case ADAPTER_STATE_C:
      state_text = "charging for ";
      state_text += minutes_charging;
      if (minutes_charging == 1)
        state_text += " minute";
      else
        state_text += " minutes";
      break;
    case 4:
      state_text = "requiring venting";
      break;
    case 5:
      state_text = "failing diode check";
      break;
    case ADAPTER_STATE_BYPASS:
      state_text = "in bypass mode";
      break;
    case 254:     
      state_text = "sleeping";
      break;
    default:
      state_text = "processing...";
  }
}

void handleShowCommands() {
  String s, hStr, MQTTBroker;
  String tmp,tmp2;
  uint8_t n;
  header1(s);
  header2("Show Commands", hStr);
  s += hStr;
  //MQTT Broker
  MQTTBroker = readEEPROM(MQTTBROKER_START, MQTTBROKER_MAX_LENGTH);
  if (MQTTBroker == "")
    MQTTBroker = DEFAULTMSERVER;
  if (WiFi.SSID() == DEFAULTSSID){
    MQTTBroker = POWERMC_MSERVER;
  }
  for (uint16_t k = 0; k < strlen_P(m_show); k++) {
     s += char(pgm_read_byte_near(m_show + k));
  }
  s += "<P><I>The following are the commands the " + location_name + " ";
  s += deviceName();
  s += " will respond to if published to the MQTT Broker at " + MQTTBroker;
  s += "</I></P>";

  if (sw_config == "a"){
    s += "<P><B>Topic: </B>" + base + "st<B> Payload: </B>JSON DICT " + consumed + ":[float W], " + generated + ":[float W], extra:[int W], uprate:[int secs]</P>";
  }
  else if (sw_config == "b"){
   s += "<P><B>Topic: </B>" + consumedTopic + "<B> Payload: </B> (string W)</P>";
   s += "<P><B>Topic: </B>" + generatedTopic + "<B> Payload: </B> (string W)</P>";
   tmp2 = base + "uprate";
   s += "<P><B>Topic: </B>" + tmp2 + "<B> Payload: </B> (string secs)</P>";      
  }

  for (uint16_t k = 0; k < strlen_P(m_show2); k++) {
     s += char(pgm_read_byte_near(m_show2 + k));
  }
  s.replace("_BASE_",base);
  s.replace("_MAX_",String(MAX_NUM_DEVICES));
  s.replace("_BASE2_",base2);
  s.replace("_MAX1_",String(MAX_NUM_DEVICES-1));
  
  if (EVSE_READY) {
    tmp2 = base2 + "c/pilot";
    s += "<P><B>Topic: </B>" + tmp2 + "<B> Payload: </B> (int " + String(MIN_CURRENT_CAPACITY_J1772) + "-up to " + String(set_current) + " depending on EVSE's Max)</P>";
    tmp2 = base2 + "c/hiamp";
    s += "<P><B>Topic: </B>" + tmp2 + "<B> Payload: </B> (int " + String(MIN_CURRENT_CAPACITY_J1772) + "-up to " + String(set_current) + " depending on EVSE's Max)</P>";
  }
  if (evse_type != JUICEBOX) {
    tmp2 = base2 + "c/level";
    s += "<P><B>Topic: </B>" + tmp2 + "<B> Payload: </B> (int 1-EVSE set to Level 1, 2-EVSE set to Level 2)</P>";
  }
  s += "<FORM ACTION='/'><P><INPUT TYPE=submit VALUE='  Home  '></P></FORM>";
  s += "</BODY></HTML>";
  server.send(200, "text/html", s);
}

void handleShowData() {
  String s, hStr, MQTTBroker;
  String tmp1, tmp2;
  header1(s);
  s += "<META HTTP-EQUIV='refresh' CONTENT='10; URL=/data'>";
  header2("Show Data", hStr);
  s += hStr;
  //MQTT Broker
  MQTTBroker = readEEPROM(MQTTBROKER_START, MQTTBROKER_MAX_LENGTH);
  if (MQTTBroker == "")
    MQTTBroker = DEFAULTMSERVER;
  if (WiFi.SSID() == DEFAULTSSID){
    MQTTBroker = POWERMC_MSERVER;
  }
  for (uint16_t k = 0; k < strlen_P(m_show); k++) {
    s += char(pgm_read_byte_near(m_show + k));
  }
  s += "<P><I>The following actual data is published to the MQTT Broker at " + MQTTBroker + " every " + String(SERVER_UPDATE_RATE/1000);
  if (SERVER_UPDATE_RATE == 1000)
    s += " sec";
  else
    s += " secs";
  for (uint16_t k = 0; k < strlen_P(m_show_data); k++) {
    s += char(pgm_read_byte_near(m_show_data + k));
  }
  s.replace("_BASE2_",base2);
  s.replace("_OUT1_",out1);
  s.replace("_MBROKER_",MQTTBroker);
  s.replace("_RATE_",String(REPORTING_IN_RATE/1000));
  s.replace("_BASE_",base);
  s.replace("_OUT2_",out2);
  s.replace("_SLOWLOAD_",slow_load);
  s.replace("_AINDEX_",String(assigned_index));
  server.send(200, "text/html", s);
}

void handleRapi() {
  String s, hStr;
  header1(s);
  header2("Send Rapi Command", hStr);
  s += hStr;
  for (uint16_t k = 0; k < strlen_P(m_rapi); k++) {
        s += char(pgm_read_byte_near(m_rapi + k));
      }
  s += "</BODY></HTML>";
  server.send(200, "text/html", s);
}

void handleRapiR() {
  String s, hStr;
  String rapiString;
  String rapi = server.arg("rapi");
  //Serial.println(rapi);
  rapi.replace("%24", "$");
  rapi.replace("+", " "); 
  //Serial.flush();
  //Serial.println(rapi);
  if (rapi[0] == '$'){
    switch(rapi[1]) { 
      case 'F': // function
        switch(rapi[2]) {
          case 'E': // enable adapter
            g_EvseController.Enable();
            user_set_adapter_mode = ADAPTER_MODE_ENABLED;
            writeEEPROMbyte(MODE_START,user_set_adapter_mode);
            g_EvseController.SetAutomaticMode(0);
            break;     
          case 'R': // reset adapter
            reboot = 1;
            break;
          case 'S': // sleep
            g_EvseController.Sleep();
            user_set_adapter_mode = ADAPTER_MODE_SLEEP;
            writeEEPROMbyte(MODE_START,user_set_adapter_mode);
            g_EvseController.SetAutomaticMode(0);
            break;
          default:
            break; // do nothing
        }
        break;
      case 'S': // set parameter
        uint8_t pilot_amp;
        uint8_t nosave;
        switch(rapi[2]) {
          case 'C': // current capacity
            if ((uint8_t)rapi[4] >= 48 && (uint8_t) rapi[4] < 58){
              if ((rapi[5] != ' ') && (rapi.length() > 5)){
                if ((uint8_t)rapi[5] >= 48 && (uint8_t) rapi[5] < 58){
                  pilot_amp = ((uint8_t)rapi[4] - 48)*10 + ((uint8_t) rapi[5]- 48);
                  if (rapi[7] == 'N')
                    nosave = 1;
                  else
                    nosave = 0;
                  g_EvseController.SetCurrentCapacity(pilot_amp,nosave);
                  }
              }
              else{
                pilot_amp = (uint8_t)rapi[4] - 48;
                if (rapi[6] == 'N')
                  nosave = 1;
                else
                  nosave = 0;
                g_EvseController.SetCurrentCapacity(pilot_amp,nosave);
              }
            } 
            break;
          case 'D': // diode check
            g_EvseController.EnableDiodeCheck((rapi[4] == '0') ? 0 : 1);
            break;
          case 'L': // service level
            uint8_t level;
            if ((evse_type != JUICEBOX) || !solar_evse)
              if ((uint8_t)rapi[4] > 48 && (uint8_t) rapi[4] < 51){
                  level = (uint8_t)rapi[4] - 48;    
                  g_EvseController.SetSvcLevel(level);
              }
            break;        
          case 'V': // vent required
            g_EvseController.EnableVentReq(rapi[4] == '0' ? 0 : 1);
            break;
          default:
            break; // do nothing
        }
        break;
      default:
        break; // do nothing 
    }
  }
  while (Serial.available()) {
     rapiString = Serial.readStringUntil('\r');
   } 
   header1(s);
   header2("RAPI Command Sent", hStr);
   s += hStr;
   for (uint16_t k = 0; k < strlen_P(m_rapi); k++) {
     s += char(pgm_read_byte_near(m_rapi + k));
   }
   s += "<P>";
   s += rapi;
   s += "</P><P>";
   s += rapiString;
   s += "</P></BODY></HTML>";
   server.send(200, "text/html", s);
}

void handleCfg() {
  String s, hStr,MQTTBroker,uName,pWord,port,saved_pm_mac_address;
  String qsid = server.arg("ssid");
  String qhsid = server.arg("hssid");
  if (qhsid != "empty")
    qsid = qhsid;
  String qpass = server.arg("pass");
  String qswconfig = server.arg("swconfig");
   
  //MQTT Broker
  String qMQTTBroker = server.arg("MQTTBroker");  
  String qport = server.arg("port");      
  String quName = server.arg("uname");
  String qpWord = server.arg("pword");
  String qpmbase = server.arg("pmbase");
  String qconsumed = server.arg("consumed");
  String qgenerated = server.arg("generated");
  String qdevice = server.arg("device");
  String mem_check,tmp;

  saved_pm_mac_address = readEEPROM(PM_MAC_START, PM_MAC_MAX_LENGTH);
  port = readEEPROM(MQTT_PORT_START, MQTT_PORT_MAX_LENGTH);
  if (port == "")  
    port = DEFAULTMPORT;
  uName = readEEPROM(UNAME_START, UNAME_MAX_LENGTH);
  pWord = readEEPROM(PWORD_START, PWORD_MAX_LENGTH);
  
  //MQTT Broker
  MQTTBroker = readEEPROM(MQTTBROKER_START, MQTTBROKER_MAX_LENGTH);
  if (MQTTBroker == "")
    MQTTBroker = DEFAULTMSERVER;
  if (WiFi.SSID() == DEFAULTSSID){
    MQTTBroker = POWERMC_MSERVER;
    port = DEFAULTMPORT;
  }
  if (WiFi.isConnected() && !change_network) {
    if (!no_mqtt){
      if (MQTTBroker != qMQTTBroker) {
        writeEEPROM(MQTTBROKER_START, MQTTBROKER_MAX_LENGTH, qMQTTBroker);
        MQTTBroker = qMQTTBroker;
        if (MQTTBroker == "")
          MQTTBroker = DEFAULTMSERVER;
        client.disconnect();
        connection_mode = WIFI;
      }
      if (port != qport) {
        writeEEPROM(MQTT_PORT_START, MQTT_PORT_MAX_LENGTH, qport);
        port = qport;
        if (port == "")
          port = DEFAULTMPORT;
        client.disconnect();
        connection_mode = WIFI;
      }
      if (uName != quName) {
        //Serial.println("different username detected");
        writeEEPROM(UNAME_START, UNAME_MAX_LENGTH, quName);
        client.disconnect();
        connection_mode = WIFI;
      }
      if (pWord != qpWord) {
        //Serial.println("different password detected");
        writeEEPROM(PWORD_START, PWORD_MAX_LENGTH, qpWord);
        client.disconnect();
        connection_mode = WIFI;
      }
      if (number_of_pms_found > 1 || (number_of_pms_found == 1 && qpmbase.toInt() == 0)){
        if (saved_pm_mac_address != pub_pm_mac[qpmbase.toInt()]) {
          //Serial.println("different PM detected");
          saved_pm_mac_address = pub_pm_mac[qpmbase.toInt()];
          selected_pm_mac_address = saved_pm_mac_address;
          writeEEPROM(PM_MAC_START, PM_MAC_MAX_LENGTH, saved_pm_mac_address);
          base = "2ms/PM/" + (String)pub_pm_dnum[qpmbase.toInt()] + "/";
          consumedTopic = base + consumed;
          generatedTopic = base + generated;
        } 
      }
      if (consumed != qconsumed) {
        //Serial.println("different consumed topic detected");
        writeEEPROM(CONSUMED_START, CONSUMED_MAX_LENGTH, qconsumed);
        consumed = qconsumed;
        consumedTopic = base + consumed;
      }
      if (generated != qgenerated) {
        //Serial.println("different generated topic detected");
        writeEEPROM(GENERATED_START, GENERATED_MAX_LENGTH, qgenerated);
        generated = qgenerated;
        generatedTopic = base +  generated;
      }   
      if (sw_config != qswconfig) {   
        writeEEPROM(SW_CONFIG_START, SW_CONFIG_MAX_LENGTH, qswconfig);
        sw_config = qswconfig;
      }
    }
  }
  if (device != qdevice.toInt()) {    
    writeEEPROMbyte(DEVICE_START, qdevice.toInt());
  }
  
  header1(s);
  s += "<META HTTP-EQUIV='refresh' CONTENT='3; URL=/'>";
  header2("Confirmation", hStr);
  s += hStr;
  if (qsid != "not chosen" && (change_network == true || !WiFi.isConnected())) {
    if (qsid != WiFi.SSID() || qpass != WiFi.psk()) {      
      s += "<P>Trying to connect to chosen network</P>";    
      s += "<P>The ";
      s += deviceName();
      s += " will try to join " + qsid + "</P>";
      s += "<P>After about 10 seconds, if successful, please use the IP address ";
      s += "assigned by your network to the ";
      s += deviceName();
      s += " in your Browser ";
      s += "in order to re-access the Configuration page.</P>";
      s += "<P>---------------------</P>";
      s += "<P>If unsuccessful after about 10 seconds, the ";
      s += deviceName();
      s += " will go back to the ";
      s += "default access point at SSID:";
      if (solar_evse)
        s += APSSID_E;
      else
        s += APSSID;
      s += "</P>";
      s += "</BODY></HTML>";
      server.send(200, "text/html", s);
      delay(500);
      client.disconnect();
      WiFi.persistent(true); // will save changes to internal flash for auto connect
      WiFi.disconnect();
      WiFi.begin(qsid,qpass);
      processWiFiConnectionAttempt();
      //If not successful, then restart in AP mode
      if (!WiFi.isConnected()){
        rebootIntoAPMode();
      }
      else
        reboot = 1;
      //Serial.println(WiFi.getMode());
    }
  }
  if (device != qdevice.toInt()){
    s += "<P>Saved to memory</P>";
    s += "<P>Because of the changes, the ";
    s += deviceName();
    s +=" will now need to reboot</P>";
    s += "<P>The new name is now ";
    if (solar_evse)
      s += APSSID_E;
    else
      s += APSSID;
    s += qdevice + ".local</P>";
    s += "<P>Please wait about 20 secs before retrying.</P>";
    s += "<P>Rebooting now...please wait</P>";
    s += "</FORM></BODY></HTML>";
    server.send(200, "text/html", s);
    reboot = 1; 
  }
  if (qsid == "not chosen"){
     s += "<P><B>Warning.</B> No network selected</P>";
     s += "<P>All functions except data logging will continue to work</P>";
  }
  else {
      generated_power = 0;  //reset any persistant data from MQTT when anything changes
      consumed_power = 0;   //reset any persistant data from MQTT when anything changes
      extra_power = 0;
      s += "<P>Saving to memory...please wait</P>";
   }
  s += "</FORM></BODY></HTML>";
  change_network = false;
  server.send(200, "text/html", s);
}

void handleRst() {
  String s, hStr;
  header1(s);
  header2("Reset", hStr);
  s += hStr;
  s += "<P>Reset to Defaults:</P>";
  s += "<P>Clearing the EEPROM...</P>";
  s += "<P>The ";
  s += deviceName();
  for (uint16_t k = 0; k < strlen_P(m_rst); k++) {
    s += char(pgm_read_byte_near(m_rst + k));
  }
  if (solar_evse)
    s += APSSID_E;
  else
    s += APSSID;
  s += " and password:";
  s += APPASS;
  s += "</P>";
  s += "</BODY></HTML>";
  resetEEPROM(0, EEPROM_SIZE);
  server.send(200, "text/html", s);
  reboot = 1;
}

void handleRst2() {
  String s, hStr;
  header1(s);
  s += "<META HTTP-EQUIV='refresh' CONTENT='1; URL=/'>";
  header2("Reset", hStr);
  s += hStr;
  s += "<P>Rebooting now...</P>";
  s += "<P>Please wait about 15 seconds</P>";
  s += "</BODY></HTML>";
  server.send(200, "text/html", s);
  reboot = 1;
}

void handleDbg() {
  String s, hStr;
  header1(s);
  if (server.arg("submit") == "Clear Reboot Counter")
    s += "<META HTTP-EQUIV='refresh' CONTENT='1; URL=/dbg'>";
  else if (server.arg("submit") == "Check for Updates")
    s += "<META HTTP-EQUIV='refresh' CONTENT='2; URL=/dbg'>";
  header2("Debug for " + String(WiFi.macAddress()),hStr);
  s += hStr;

  if (server.arg("submit") == "Check for Updates"){
    firmwareUpdateCheck(1);
    s += "<P STYLE='COLOR:#00A000'>No Updates Available! Please wait...</P>"; 
  }
  if (server.arg("submit") != "Check for Updates"){
    if (server.arg("submit") == "Clear Reboot Counter"){
      boot_count = 0;
      writeEEPROMbyte(BOOT_START,boot_count);
      s += "<P STYLE='COLOR:#00A000'>Clearing...Please wait!</P>";
    }
    s += "<P>Reboot count is ";
    s += boot_count;
    s += " time(s)</P>";
    s += "<FORM ACTION='/dbg'>";
    s += "<P>&nbsp;&nbsp;<INPUT TYPE=submit NAME='submit' VALUE='Clear Reboot Counter'></P>";
    s += "</FORM>";
    s += "<P>Unplugged count when awake is ";
    s += unplugged_count;
    s += "</P>";
    s += "<P>Unplugged high value is ";
    s += unplugged_hi;
    s += "</P>";
    s += "<P>Unplugged low value is ";
    s += unplugged_lo;
    s += "</P>";
    s += "<P>Plugged in and awaken count is ";
    s += plugged_in_count;
    s += "</P>";
    s += "<P>Plugged in high value is ";
    s += plugged_in_hi;
    s += "</P>";
    s += "<P>Plugged in low value is ";
    s += plugged_in_lo;
    s += "</P>";
    s += "<P>Charging count is ";
    s += charge_count;
    s += "</P>";
    s += "<P>Charging high value is ";
    s += charge_hi;
    s += "</P>";
    s += "<P>Charging low value is ";
    s += charge_lo;
    s += "</P>";
    s += "<P>Proximity pilot in EVSE handle value is ";
    s += sample_pp;
    s += "</P>";
    s += "<FORM ACTION='/dbgR'>";
    s += "<P>Configuration ";
    s += "<INPUT TYPE='radio' NAME='solar_evse' VALUE='1'";
    if (solar_evse)
      s += " CHECKED";
    s += ">SolarEVSE <INPUT TYPE='radio' NAME='solar_evse' VALUE='0'";
    if (!solar_evse)
      s += " CHECKED";
    s += ">SolarAdapter</P>";
    s += "<P>EVSE type ";
    s += "<INPUT TYPE='radio' NAME='type' VALUE='1'";
    if (evse_type == JUICEBOX)
      s += " CHECKED";
    s += ">JuiceBox <INPUT TYPE='radio' NAME='type' VALUE='0'";
    if (evse_type == 0)
      s += " CHECKED";
    s += ">Generic</P>";
    s += "<P>Proximity signaling ";
    s += "<INPUT TYPE='radio' NAME='skip_pp' VALUE='1'";
    if (skip_pp)
      s += " CHECKED";
    s += ">Skip PP <INPUT TYPE='radio' NAME='skip_pp' VALUE='0'";
    if (!skip_pp)
      s += " CHECKED";
    s += ">Check PP</P>";
    s += "<P>Pilot current is set to ";
    s += pilot;
    s += " A</P>";
    if (alt_set_current < MIN_CURRENT_CAPACITY_J1772) {
      s += "<P>Detected EVSE max current is ";
      s += set_current;   
      s += " A</P>";
    }
    if (evse_type == JUICEBOX)
      s += "<P>EVSE status string is " + received_from_evse + "</P>";
    s += "<P><LABEL>Enter Max Current for EVSE (";
    s += String(MIN_CURRENT_CAPACITY_J1772 - 1) + " or less for autodetect) </LABEL><INPUT TYPE='number' ID='altc' NAME='altc' MIN='0' MAX='";
    if (g_EvseController.GetCurSvcLevel() == 1){
      s += String(MAX_CURRENT_CAPACITY_L1) + "' VALUE='";
    }
    else {
      s += String(MAX_CURRENT_CAPACITY_L2) + "' VALUE='";
    }
    s += alt_set_current;
    s += "'>&nbsp;A</P>";
    for (uint16_t k = 0; k < strlen_P(m_updates); k++) {
      s += char(pgm_read_byte_near(m_updates + k));
    }
  }
  s += "</BODY>";
  s += "</HTML>";
  server.send(200, "text/html", s);
}

void handleDbgR() {
  uint8_t tmp;
  String s, hStr;
  String sSolar_evse = server.arg("solar_evse");
  String sType = server.arg("type");
  String sSkip_pp = server.arg("skip_pp");
  String sAltc = server.arg("altc");

  tmp = sAltc.toInt();
  if (alt_set_current != tmp){
    alt_set_current = tmp;
    unplug_time = 6;              //needed to take into effect immediately
    immediate_status_update = true; //needed take into effect immediately
    writeEEPROMbyte(ALT_SET_CURRENT_START, tmp);
  } 
  
  tmp = sSolar_evse.toInt();
  if (tmp != solar_evse){
    reboot = 1;
    writeEEPROMbyte(SOLAR_EVSE_START, tmp);
    if (tmp == 0) {     //solar adapter then automatically set type to generic
      writeEEPROMbyte(EVSE_TYPE_START, 0);  
    }
  }

  tmp = sType.toInt();
  if (tmp != evse_type){
    writeEEPROMbyte(EVSE_TYPE_START, tmp);
    reboot = 1;
  }

  tmp = sSkip_pp.toInt();
  if (tmp != skip_pp){
    skip_pp = tmp;
    writeEEPROMbyte(SKIP_PP_START,tmp);
  }
   
  header1(s);
  s += "<META HTTP-EQUIV='refresh' CONTENT='1; URL=/'>";
  header2("Reset", hStr);
  s += hStr;
  if (reboot){
    s += "<P>Rebooting now...</P>";
    s += "<P>Please wait about 15 seconds</P>";
  }
  else {
   s += "<P>Saving data now...</P>";
   s += "<P>Please wait...</P>";
  }
  s += "</BODY></HTML>";
  server.send(200, "text/html", s);
}

void encodeString(String& toEncode) {
  toEncode.replace("<", "&lt;");
  toEncode.replace(">", "&gt;");
  toEncode.replace("|", "&vert;");
}

void createOptionSelector(const String& name, const String values[], const String options[], int len, const String& value, String& str) {
  String tmpStr;
  str = "<select name='" + name + "'>\n";
  for (int i = 0; i < len; i++) {
    tmpStr = options[i];
    encodeString(tmpStr);
    str += "<option value='";
    str += values[i];
    str += "'";
    str += values[i] == value ? " selected" : "";
    str += ">";
    str += tmpStr;
    str += "</option>\n";
  }
  str += "</select>\n";
}

void handleAdvanced() {
  String s, hStr, tmpStr;
  header1(s);
  header2("Advanced", hStr);
  s += hStr;
  s += "<P CLASS='P3'>Firmware version ";
  s += VERSION;
  s += "</P>";  // controller firmware version
  //get adapter flag and pilot
  updateFlag();
  s += "<FORM ACTION='/advR'>"; 
  s +=  "<P><LABEL>Name or location of device:</LABEL><INPUT NAME='loc' MAXLENGTH='32' VALUE='";
  tmpStr = location_name;
  processReservedCharacters(tmpStr);
  s += tmpStr + "'></P>";
  if (!g_EvseController.IsBypassMode()){   
    s += "<P>Diode check is set to <INPUT TYPE='radio' NAME='diode_check' VALUE='1'";
    if (!(adapter_flag & ECF_DIODE_CHK_DISABLED)) // yes
      s += " CHECKED";
    s += ">YES* <INPUT TYPE='radio' NAME='diode_check' VALUE='0'";
    if ((adapter_flag & ECF_DIODE_CHK_DISABLED) == ECF_DIODE_CHK_DISABLED) //no
      s += " CHECKED";
    s += ">NO</P>";
    
    s += "<P>Vent required state is set to <INPUT TYPE='radio' NAME='vent_check' VALUE='1'";
    if (!(adapter_flag & ECF_VENT_REQ_DISABLED)) // yes
      s += " CHECKED";
    s += ">YES* <INPUT TYPE='radio' NAME='vent_check' VALUE='0'";
    if ((adapter_flag & ECF_VENT_REQ_DISABLED) == ECF_VENT_REQ_DISABLED) //no
      s += " CHECKED";
    s += ">NO</P>";
  
    s += "<P>Always start up in sleep mode after reboot or power loss <INPUT TYPE='radio' NAME='start_sleep' VALUE='1'";
    if (g_EvseController.IsStartSleep())
      s += " CHECKED";
    s += ">YES  <INPUT TYPE='radio' NAME='start_sleep' VALUE='0'";
    if (!g_EvseController.IsStartSleep()) //no
      s += " CHECKED";
    s += ">NO* </P>";
    s += "<P>Turn ON temporarily during auto tracking to align priority";
    s += "<INPUT TYPE='radio' NAME='allowon' VALUE='1'";
    if (allow_on_temporarily)
      s += " CHECKED";
    s += ">YES <INPUT TYPE='radio' NAME='allowon' VALUE='0'";
    if (!allow_on_temporarily)
      s += " CHECKED";
    s += ">NO* </P>";
    s +=  "<P><LABEL>Minimum Turn On Power for ";
    s += deviceName();
    String str = String (g_EvseController.GetCurSvcLevel()*SPLIT_PHASE_VOLTAGE*MIN_CURRENT_CAPACITY_J1772);
    String str1 = String (g_EvseController.GetCurSvcLevel()*SPLIT_PHASE_VOLTAGE*set_current);
    if (set_current < MIN_CURRENT_CAPACITY_J1772)
      str1 = String (g_EvseController.GetCurSvcLevel()*SPLIT_PHASE_VOLTAGE*MIN_CURRENT_CAPACITY_J1772 );
    s += " is (" + str + " - " + str1 + ") </LABEL><INPUT TYPE='number' ID='ipower' NAME='ipower' MIN='" + str + "' MAX='" + str1 + "' VALUE='";
    s += ipower;
    s += "'>&nbsp;W</P>";
  }
  if (adc_address != 0){
    s += "<P>Bypass ";
    s += deviceName();
    s += " <INPUT TYPE='radio' NAME='adapter_bypass' VALUE='1'";
    if (g_EvseController.IsBypassMode())
      s += " CHECKED";
    s += ">YES  <INPUT TYPE='radio' NAME='adapter_bypass' VALUE='0'";
    if (!g_EvseController.IsBypassMode())
      s += " CHECKED";
    s += ">NO*</P>";
  }

  s += "<P>Your time zone: ";

  String TIME_ZONE_CMD[10] = {"0","1","2","3","4","5","6","7","8","9"};
  String TIME_ZONE_TEXT[10] = {"AST","AST with DST", "EST with DST","CST with DST","MST","MST with DST","PST with DST","AKST with DST","HST","ChST"};
  createOptionSelector("tzone", TIME_ZONE_CMD, TIME_ZONE_TEXT, 10, String(time_zone), tmpStr);
  s += tmpStr + "</P>";
  s += "<P CLASS='P3'>* indicates default value</P>";
  
  s += "<INPUT TYPE=submit VALUE='    Save    '>";
  s += "</FORM>";
  s += "<BR><TABLE>";
  if (!g_EvseController.IsBypassMode() && !no_mqtt && !server2_down){
    s += "<TR><TD><FORM ACTION='/data'><INPUT TYPE='submit' VALUE=' Show Data '></FORM></TD>";
    s += "<TD><FORM ACTION='/commands'><INPUT TYPE='submit' VALUE='Show Commands'></FORM></TD></TR>";
  }
  s += "<TR><TD><FORM ACTION='/'><INPUT TYPE=submit VALUE='    Home   '></FORM></TD><TD></TD></TR>";
  s += "</TABLE>";
  s += "</BODY></HTML>";
  server.send(200, "text/html", s);
}

void handleAdvancedR() {
  String s, hStr;
  String sDiode = server.arg("diode_check");
  String sVent = server.arg("vent_check");
  String sStart_sleep = server.arg("start_sleep");
  String sAdapter_bypass = server.arg("adapter_bypass");
  String sLocation = server.arg("loc");
  String sIpower = server.arg("ipower");
  String sTzone = server.arg("tzone");
  String sAllowon = server.arg("allowon");
  uint8_t tmp;

  if (location_name != sLocation) {
    writeEEPROM(LOCATION_START, LOCATION_MAX_LENGTH, sLocation);
    location_name = sLocation;
  }
  tmp = sTzone.toInt();
  if (time_zone != tmp) {
    String TIME_ZONE[10] = {"AST4","AST4ADT,M3.2.0,M11.1.0","EST5EDT,M3.2.0,M11.1.0","CST6CDT,M3.2.0,M11.1.0","MST7","MST7MDT,M3.2.0,M11.1.0","PST8PDT,M3.2.0,M11.1.0","AKST9AKDT,M3.2.0,M11.1.0","HST10","ChST-10"}; // https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
    writeEEPROMbyte(TIME_ZONE_START, tmp);
    time_zone = tmp;
    configTime(TIME_ZONE[time_zone].c_str(), MY_NTP_SERVER);  //for NTP
  }
  if (!g_EvseController.IsBypassMode()){
    if (ipower != sIpower.toInt()){
      ipower = sIpower.toInt();
      writeEEPROMword(IPOWER_START,ipower);
    }   
    if (g_EvseController.IsStartSleep() != sStart_sleep.toInt()){
      if (sStart_sleep == "1")
        g_EvseController.SetStartSleep(1);
      else
        g_EvseController.SetStartSleep(0);
    }
    if (g_EvseController.DiodeCheckEnabled() != sDiode.toInt()){
      if (sDiode == "1")
        g_EvseController.EnableDiodeCheck(1);
      else
        g_EvseController.EnableDiodeCheck(0);
    } 
    if (g_EvseController.VentReqEnabled() != sVent.toInt()){
      if (sVent == "1")
        g_EvseController.EnableVentReq(1);    // vent check
      else
         g_EvseController.EnableVentReq(0);
    }    
    tmp = sAllowon.toInt();
    if (tmp != allow_on_temporarily){
      allow_on_temporarily = tmp;
      writeEEPROMbyte(ALLOW_ON_START,tmp);
    }
  }
  if (g_EvseController.IsBypassMode() != sAdapter_bypass.toInt()){
    if (sAdapter_bypass == "1")
      g_EvseController.SetAdapterBypass(1);
    else
      g_EvseController.SetAdapterBypass(0);
  }
  header1(s);
  s += "<META HTTP-EQUIV='refresh' CONTENT='1; url=/'>";
  header2("Confirmation", hStr);
  s += hStr;
  s += "<FORM ACTION='/'>";  
  s += "<P>Success!</P>";
  s += "<P><INPUT TYPE=submit VALUE='     OK     '></P>";
  s += "</FORM>";
  s += "</BODY></HTML>";
  server.send(200, "text/html", s);
}

void handleOnR() {
  String s, hStr;
  header1(s);
  s += "<META HTTP-EQUIV='refresh' CONTENT='0; url=/'>";
  header2("Confirmation", hStr);
  s += hStr;
  if (!adapter_enabled) {
    g_EvseController.Enable();
    setDelayTimer(0);
    user_set_adapter_mode = ADAPTER_MODE_ENABLED;
    writeEEPROMbyte(MODE_START,user_set_adapter_mode);
    g_EvseController.SetAutomaticMode(0);
  }
  else {
    g_EvseController.Sleep();
    user_set_adapter_mode = ADAPTER_MODE_SLEEP;
    writeEEPROMbyte(MODE_START,user_set_adapter_mode);
    g_EvseController.SetAutomaticMode(0);
  }
  for (uint16_t k = 0; k < strlen_P(m_success); k++) {
    s += char(pgm_read_byte_near(m_success + k));
  }
  server.send(200, "text/html", s);
}

uint8_t getAdapterState() {
  uint8_t state = 0;
      state = g_EvseController.GetState();
      if (state == ADAPTER_STATE_C) {
        minutes_charging = g_EvseController.GetElapsedChargeTime()/60; //convert to minutes
      }
  return state;
}

void updateFlag() {
   pilot = g_EvseController.GetCurrentCapacity();
   adapter_flag = g_EvseController.GetFlags();   
}
  
void updatePluggedInStatus() {
    vflag = g_EvseController.GetVFlags();
    if ((vflag & PLUG_IN_MASK) > 0){
      digitalWrite(PLUG_IN, HIGH);
      plugged_in = 1;
      Timer10 = millis();
      }
    else
      if ((millis() - Timer10) > PLUG_OUT_TIMEOUT){   // delaying plug out in case EV glitches plug in resistor as seen in Teslas.
        plugged_in = 0;
        digitalWrite(PLUG_IN, LOW);
      } 
    if (pre_plugged_in != plugged_in) {           //plug in or out transition occurred
      Timer12 = millis();   //reset LED timeout 
      if (allow_on_temporarily) {
        if (plugged_in && g_EvseController.IsAutomaticMode() && EVSE_READY){ //turn on power when plugged in so that auto tracking will adjust to proper priority
          forced_reset_priority = true;
          g_EvseController.Enable();
          g_EvseController.SetCurrentCapacity(MIN_CURRENT_CAPACITY_J1772,1);
        }
      }
      pre_plugged_in = plugged_in;
    }
 }

void setHiAmpSetpoint(uint8_t amps) {
  if (EVSE_READY && (amps >= MIN_CURRENT_CAPACITY_J1772) && (amps <= set_current)) {
    hi_amp_setpoint = amps;
    //Serial.println(sHi_amp_setpoint);
    writeEEPROMbyte(HI_AMP_SETPOINT_START, amps);
    saved_hi_amp_setpoint = amps;
    immediate_status_update = true;
    if (g_EvseController.IsAutomaticMode())
      forced_reset_priority = true;
  }
}

void handleLimitsR() {
  uint8_t tmp;
  String s, hStr;
  String sTime_limit = server.arg("tlim");
  String sCharge_limit = server.arg("clim");
  header1(s);
  s += "<META HTTP-EQUIV='refresh' CONTENT='0; url=/'>";
  header2("Confirmation", hStr);
  s += hStr;
  s += "<FORM ACTION='/'>";
  tmp = sTime_limit.toInt();
  if (tmp != double_time_limit){
    double_time_limit = tmp;
    writeEEPROMbyte(DOUBLE_TIME_LIMIT_START,tmp);
  }
  tmp = sCharge_limit.toInt();
  if (tmp != half_charge_limit){
    half_charge_limit = tmp;
    writeEEPROMbyte(HALF_CHARGE_LIMIT_START,tmp);
  }
  s += "<P><INPUT TYPE=submit VALUE='     OK     '></P>";
  s += "</FORM>";
  s += "</BODY></HTML>";
  server.send(200, "text/html", s);
}

void handleSetPilotR() {
  String s, hStr;
  uint8_t tmp;
  String sCurrent = server.arg("maxi");
  header1(s);
  s += "<META HTTP-EQUIV='refresh' CONTENT='0; url=/'>";
  header2("Confirmation", hStr);
  s += hStr;
  tmp = sCurrent.toInt();
  if (pilot != tmp){
     g_EvseController.SetCurrentCapacity(tmp);
     g_EvseController.SetAutomaticMode(0);
  }
  for (uint16_t k = 0; k < strlen_P(m_success); k++) {
    s += char(pgm_read_byte_near(m_success + k));
  }
  server.send(200, "text/html", s);
}

void handleAutoTrackingSettingsR() {
  String s, hStr;
  String sHi_amp_setpoint = server.arg("hiset");
  String qwait = server.arg("wait_time");
  uint8_t tmp;
  
  header1(s);
  s += "<META HTTP-EQUIV='refresh' CONTENT='0; url=/'>";
  header2("Confirmation", hStr);
  s += hStr;
  tmp = server.arg("PRIORITY").toInt();
  if (tmp != device_priority){
    device_priority = tmp;
    writeEEPROMbyte(DEVICE_PRIORITY_START, tmp);
    immediate_status_update = true;
    forced_reset_priority = true;
  }
  tmp = sHi_amp_setpoint.toInt();
  if (hi_amp_setpoint != tmp){
    setHiAmpSetpoint(tmp);
  }
  if (on_off_wait_time != qwait.toInt()) {
    on_off_wait_time = qwait.toInt();   
    writeEEPROMword(TRANSITION_WAIT_START,on_off_wait_time);
  }
  if (!g_EvseController.IsAutomaticMode())
    g_EvseController.SetAutomaticMode(1);
  for (uint16_t k = 0; k < strlen_P(m_success); k++) {
    s += char(pgm_read_byte_near(m_success + k));
  }
  server.send(200, "text/html", s);
}

void updateSAStatus() {
  unsigned long elapsed_time_since_recorded;
  if (reboot == 1){
    boot_count--;
    writeEEPROMbyte(BOOT_START,boot_count);
    client.disconnect();
    delay(1000);
    ESP.restart();
  }
  if ((millis() - Timer3) >= SA_STATUS_UPDATE_RATE || immediate_status_update) {
    if (immediate_status_update){
      immediate_status_update = false;
      publish_immediately = true;
    }
    Timer3 = millis();
    if (ipower < g_EvseController.GetCurSvcLevel()*SPLIT_PHASE_VOLTAGE*MIN_CURRENT_CAPACITY_J1772)
      ipower =  g_EvseController.GetCurSvcLevel()*SPLIT_PHASE_VOLTAGE*MIN_CURRENT_CAPACITY_J1772;
    adapter_state = getAdapterState();
    if (adapter_state == ADAPTER_STATE_C){
      elapsed_time_since_recorded = g_EvseController.GetElapsedChargeTime();
      if (header_received == "$TM")   //for version 1
        kilowatt_seconds_accum = kilowatt_seconds_accum + (elapsed_time_since_recorded-recorded_elapsed_time)*evse_voltageX10*evse_currentX10*evse_pfX1000/2500000.0;
      else
        kilowatt_seconds_accum = kilowatt_seconds_accum + (elapsed_time_since_recorded-recorded_elapsed_time)*g_EvseController.GetCurSvcLevel()*3*pilot/25;
      recorded_elapsed_time = elapsed_time_since_recorded;
    }
    else {
      recorded_elapsed_time = 0;
      kilowatt_seconds_accum = 0;
    }
    if (g_EvseController.GetCurSvcLevel() == 1){
      if (alt_set_current > MAX_CURRENT_CAPACITY_L1){
        alt_set_current = MAX_CURRENT_CAPACITY_L1;
        unplug_time = 6;              //needed to take into effect immediately
        writeEEPROMbyte(ALT_SET_CURRENT_START, alt_set_current);
      }
    }
    else {
      if (alt_set_current > MAX_CURRENT_CAPACITY_L2){
        alt_set_current = MAX_CURRENT_CAPACITY_L2;
        unplug_time = 6;              //needed to take into effect immediately
        writeEEPROMbyte(ALT_SET_CURRENT_START, alt_set_current);
      }
    }
    if (plugged_in){
      set_current = checkEVSEStatus();
      unplug_time = 1;
    }
    else if (unplug_time >= 6){
      unplug_time = 1;
      set_current = checkEVSEStatus();
    }
    else 
      unplug_time++;
    if (set_current != pre_set_current){    //set current changed, need make sure confirm others are within limits
      Timer12 = millis();   //reset LED timeout
      if (set_current > MAX_CURRENT_CAPACITY_L2)
        set_current = MAX_CURRENT_CAPACITY_L2;
      if (EVSE_READY && !g_EvseController.IsAutomaticMode()){
        if (g_EvseController.GetMaxCurrentCapacity() <= set_current)
            g_EvseController.SetCurrentCapacity(g_EvseController.GetMaxCurrentCapacity(),1);  //don't save and let function adjust to set_current
        else
          g_EvseController.SetCurrentCapacity(set_current,1);  //don't save and let function adjust to set_current
        if (saved_hi_amp_setpoint <= set_current)
          hi_amp_setpoint = saved_hi_amp_setpoint; 
        else
          hi_amp_setpoint = set_current;  //don't save
      }
      pre_set_current = set_current;
    }
  }
}

void nameOfDay (int day, String& string_name) {
  string_name;
  switch (day) {
    case 0:
      string_name = "Sunday";
      break;
    case 1:
      string_name = "Monday";
      break;
    case 2:
      string_name = "Tuesday";
      break;
    case 3:
      string_name = "Wednesday";
      break;
    case 4:
      string_name = "Thursday";
      break;
    case 5:
      string_name = "Friday";
      break;
    case 6:
      string_name = "Saturday";
      break;
    default:
      string_name = "not set";
  }
}

void nameOfMonth (int month, String& string_name) {
  switch (month) {
    case 0:
      string_name = "January";
      break;
    case 1:
      string_name = "February";
      break;
    case 2:
      string_name = "March";
      break;
    case 3:
      string_name = "April";
      break;
    case 4:
      string_name = "May";
      break;
    case 5:
      string_name = "June";
      break;
    case 6:
      string_name = "July";
      break;
    case 7:
      string_name = "August";
      break;
    case 8:
      string_name = "September";
      break;
    case 9:
      string_name = "October";
      break;
    case 10:
      string_name = "November";
      break;
    case 11:
      string_name = "December";
      break;
    default:
      string_name = "not set";
  }
}

void handleOffTimerR() {
  String s, hStr;
  header1(s);
  s += "<META HTTP-EQUIV='refresh' CONTENT='0; url=/'>";
  header2("Confirmation", hStr);
  s += hStr;
  setDelayTimer(0);
  for (uint16_t k = 0; k < strlen_P(m_success); k++) {
    s += char(pgm_read_byte_near(m_success + k));
  }
  server.send(200, "text/html", s);
}

void handleOffTrackingR() {
  String s, hStr;
  header1(s);
  s += "<META HTTP-EQUIV='refresh' CONTENT='0; url=/'>";
  header2("Confirmation", hStr);
  s += hStr;
  if (g_EvseController.IsAutomaticMode())
     g_EvseController.SetAutomaticMode(0);  
  for (uint16_t k = 0; k < strlen_P(m_success); k++) {
    s += char(pgm_read_byte_near(m_success + k));
  }
  server.send(200, "text/html", s);
}

void handleOffLimitsR() {
  String s, hStr;
  header1(s);
  s += "<META HTTP-EQUIV='refresh' CONTENT='0; url=/'>";
  header2("Turn Off Limits", hStr);
  s += hStr;
  double_time_limit = 0;
  writeEEPROMbyte(DOUBLE_TIME_LIMIT_START,double_time_limit);
  half_charge_limit = 0;
  writeEEPROMbyte(HALF_CHARGE_LIMIT_START,half_charge_limit);
  for (uint16_t k = 0; k < strlen_P(m_success); k++) {
    s += char(pgm_read_byte_near(m_success + k));
  }
  server.send(200, "text/html", s);
}

void handleLimits() {
  String s, hStr;
  uint8_t index;
  float tmp;
  header1(s);
  header2("Charging Limits", hStr);
  s += hStr;
  s += "<FORM ACTION='/limitsR'>";
  s += "<P>Select limits per session:</P>";
  s += "<P>&nbsp;&nbsp;&nbsp;Stop after ";
  s += "<SELECT NAME='tlim'>";
  for (int index = 0; index <= DOUBLE_TIME_LIMIT_MAX; ++index) {   // drop down at 30 min increments
     tmp = index * 0.5;
     yield();
     if (index == 0) {
       if (double_time_limit == 0) {
         s += "<OPTION VALUE='" + String(index) + "'SELECTED>" + "no limit</OPTION>";
       }
       else
         s += "<OPTION VALUE='" + String(index) + "'>" +  "no limit</OPTION>";
     }
     else {
       if (double_time_limit == index) {
         s += "<OPTION VALUE='" + String(index) + "'SELECTED>";
       }
       else
         s += "<OPTION VALUE='" + String(index) + "'>";
       s += String(tmp,1)  + "</OPTION>";
     }
  }
  if (double_time_limit == 2.0)
    s += "</SELECT> hour</P>";
  else
    s += "</SELECT> hours</P>";
  s += "<P>&nbsp;&nbsp;&nbsp;Stop after adding ";
  s += "<SELECT NAME='clim'>";
  for (int index = 0; index <= HALF_CHARGE_LIMIT_MAX; ++index) {   // drop down at 2 kWh increments
     tmp = index * 2;
     yield();
     if (index == 0 ) {
       if (half_charge_limit == 0) {
         s += "<OPTION VALUE='" + String(index) + "'SELECTED>" + "no limit</OPTION>";
       }
       else
         s += "<OPTION VALUE='" + String(index) + "'>" +  "no limit</OPTION>";
     }
     else {
       if (half_charge_limit == index) {
         s += "<OPTION VALUE='" + String(index) + "'SELECTED>";
       }
       else
         s += "<OPTION VALUE='" + String(index) + "'>";
       s += String(tmp,0)  + "</OPTION>";
     }
  }
  s += "</SELECT> kWh</P>";
  for (uint16_t k = 0; k < strlen_P(m_submit); k++) {
    s += char(pgm_read_byte_near(m_submit + k));
  }
  server.send(200, "text/html", s);
}

void handleDelayTimer() {
  String s, hStr, tmpStr, tmpStr2;
  uint8_t index;
  header1(s);
  header2("Scheduled Charging in 24-hour Time Format", hStr);
  s += hStr;
  s += "<FORM ACTION='/dtimerR'>";
  getDateTimeString(2, tmpStr);
  getDateTimeString(3, tmpStr2);
  s += "<P CLASS='P3'>Current time in 24-hour format is " + tmpStr + " or " + tmpStr2 + "</P>";
  s += "<P>Start Time (hh:mm) - ";
  s += " <SELECT NAME='starthour'>";
  for (index = 0; index <= 9; ++index) {
     if (index == start_hour)
       s += "<OPTION VALUE='" + String(index) + "'SELECTED>" + "0" + String(index) + "</OPTION>";
     else
       s += "<OPTION VALUE='" + String(index) + "'>" + "0" + String(index) + "</OPTION>";
  }
    for (index = 10; index <= 23; ++index) {
     if (index == start_hour)
       s += "<OPTION VALUE='" + String(index) + "'SELECTED>" + String(index) + "</OPTION>";
     else
       s += "<OPTION VALUE='" + String(index) + "'>" + String(index) + "</OPTION>";
  }
  s += "</SELECT>:";
  s += "<SELECT NAME='startmin'>";
  for (index = 0; index <= 9; ++index) {
     if (index == start_min)
       s += "<OPTION VALUE='" + String(index) + "'SELECTED>" + "0" + String(index) + "</OPTION>";
     else
       s += "<OPTION VALUE='" + String(index) + "'>" + "0" + String(index) + "</OPTION>";
  }
  for (index = 10; index <= 59; ++index) {
     if (index == start_min)
       s += "<OPTION VALUE='" + String(index) + "'SELECTED>" + String(index) + "</OPTION>";
     else
       s += "<OPTION VALUE='" + String(index) + "'>" + String(index) + "</OPTION>";
  }
  s += "</SELECT></P>";
  s += "<P>Stop Time (hh:mm) - ";
  s += "<SELECT NAME='stophour'>";
  for (index = 0; index <= 9; ++index) {
     if (index == stop_hour)
       s += "<OPTION VALUE='" + String(index) + "'SELECTED>" + "0" + String(index) + "</OPTION>";
     else
       s += "<OPTION VALUE='" + String(index) + "'>" + "0" + String(index) + "</OPTION>";
  }
  for (index = 10; index <= 23; ++index) {
     if (index == stop_hour)
       s += "<OPTION VALUE='" + String(index) + "'SELECTED>" + String(index) + "</OPTION>";
     else
       s += "<OPTION VALUE='" + String(index) + "'>" + String(index) + "</OPTION>";
  }
  s += "</SELECT>:";
  s += "<SELECT NAME='stopmin'>";
  for (index = 0; index <= 9; ++index) {
     if (index == stop_min)
       s += "<OPTION VALUE='" + String(index) + "'SELECTED>" + "0" + String(index) + "</OPTION>";
     else
       s += "<OPTION VALUE='" + String(index) + "'>" + "0" + String(index) + "</OPTION>";
  }
  for (index = 10; index <= 59; ++index)  {
     if (index == stop_min)
       s += "<OPTION VALUE='" + String(index) + "'SELECTED>" + String(index) + "</OPTION>";
     else
       s += "<OPTION VALUE='" + String(index) + "'>" + String(index) + "</OPTION>";
  }
  s += "</SELECT></P>";
  for (uint16_t k = 0; k < strlen_P(m_submit); k++) {
    s += char(pgm_read_byte_near(m_submit + k));
  }
  server.send(200, "text/html", s);
}

void handleDelayTimerR() {
  uint8_t tmp;
  String s, hStr;
  String sStart_hour = server.arg("starthour");   
  String sStart_min = server.arg("startmin");
  String sStop_hour = server.arg("stophour");
  String sStop_min = server.arg("stopmin");
  tmp = sStart_hour.toInt();
  if (start_hour != tmp){
    writeEEPROMbyte(START_HOUR_START,tmp);
    start_hour = tmp;
  }
  tmp = sStart_min.toInt();
  if (start_min != tmp){
    writeEEPROMbyte(START_MIN_START,tmp);
    start_min = tmp;
  }
  tmp = sStop_hour.toInt();
  if (stop_hour != tmp){
    writeEEPROMbyte(STOP_HOUR_START,tmp);
    stop_hour = tmp;
  }
  tmp = sStop_min.toInt();
  if (stop_min != tmp){
    writeEEPROMbyte(STOP_MIN_START,tmp);
    stop_min = tmp;
  }
  if (IsTimerValid()){
    setDelayTimer(1);
    g_EvseController.SetLimitSleep(0);
    g_EvseController.SetAutomaticMode(0,0);
    header1(s);
    s += "<META HTTP-EQUIV='refresh' CONTENT='0; url=/'>";
    header2("Confirmation", hStr);
    s += hStr;
    s += "<FORM ACTION='/'>";
    s += "<P>Success!</P>";
  }
  else {
    setDelayTimer(0);
    header1(s);
    s += "<META HTTP-EQUIV='refresh' CONTENT='0; url=/'>";
    header2("Confirmation", hStr);
    s += hStr;
    s += "<P>ERROR! Start time is equal to stop time. Please try again. </P>";
    s += "<FORM ACTION='/dtimer'>";
  }
  s += "<P><INPUT TYPE=submit VALUE='     OK     '></P>";
  s += "</FORM>";
  s += "</BODY></HTML>";
  server.send(200, "text/html", s);
}

void handleSetPilot() {
  String s, hStr;
  header1(s);
  header2("Charging Current", hStr);
  s += hStr;
  s += "<FORM ACTION='/setpilotR'>";
  s += "<P>max charge current at ";
  s += "<SELECT NAME='maxi'>";
  //get min max current allowable
  uint8_t minamp;
  uint8_t maxamp;
  if (EVSE_READY){
     minamp = MIN_CURRENT_CAPACITY_J1772;
     maxamp = set_current;
     if (g_EvseController.GetCurSvcLevel() == 2){
       if (set_current > MAX_CURRENT_CAPACITY_L2)
          maxamp = MAX_CURRENT_CAPACITY_L2;
     }
     if (g_EvseController.GetCurSvcLevel() == 1){
       if (set_current > MAX_CURRENT_CAPACITY_L1)
          maxamp = MAX_CURRENT_CAPACITY_L1;
     }             
  }
  for (uint8_t index = minamp; index <= maxamp; ++index)  {
    yield();
    if (index == pilot) {
      s += "<OPTION VALUE='" + String(index) + "'selected='SELECTED'>" + String(index) + "</OPTION>";
    }
    else
      s += "<OPTION VALUE='" + String(index) + "'>" + String(index) + "</OPTION>";
  }
  s += "</SELECT> A</P>";
  for (uint16_t k = 0; k < strlen_P(m_submit); k++) {
    s += char(pgm_read_byte_near(m_submit + k));
  }
  server.send(200, "text/html", s);
}

void handleAutoTrackingSchedule() {
  String s, hStr;
  header1(s);
  if (server.arg("save") == "    Save    ")
    s += "<META HTTP-EQUIV='refresh' CONTENT='2; URL=/atsch'>";
  header2("Auto Tracking Schedule", hStr);
  s += hStr;
  s += "<FORM ACTION='/atsch'>";
  
  s += "<P>Please choose the start hour for auto tracking ";
  s += "<SELECT NAME='startAT'>";  
  for (int16_t index = 0; index <= 23; index++) {
    if (index == start_AT) {
      if (index == 0)
        s += "<OPTION VALUE='" + String(index) + "'selected='SELECTED'>" + String(12) + ":00 AM</OPTION>";
      if (index >= 1 && index < 13)
        s += "<OPTION VALUE='" + String(index) + "'selected='SELECTED'>" + String(index) + ":00 AM</OPTION>";
      if (index == 12)
        s += "<OPTION VALUE='" + String(index) + "'selected='SELECTED'>" + String(12) + ":00 PM</OPTION>";
      if (index > 12 && index <= 23)
        s += "<OPTION VALUE='" + String(index) + "'selected='SELECTED'>" + String(index-12) + ":00 PM</OPTION>";
    }
    else{
      if (index == 0)
        s += "<OPTION VALUE='" + String(index) + "'>" + String(12) + ":00 AM</OPTION>";
      if (index >= 1 && index < 13)
        s += "<OPTION VALUE='" + String(index) + "'>" + String(index) + ":00 AM</OPTION>";
      if (index == 12)
        s += "<OPTION VALUE='" + String(index) + "'>" + String(12) + ":00 PM</OPTION>";
      if (index > 12 && index <= 23)
        s += "<OPTION VALUE='" + String(index) + "'>" + String(index-12) + ":00 PM</OPTION>";
    }
  }
  s += "</SELECT></P>";
  s += "<P>Please choose the stop hour for auto tracking ";
  s += "<SELECT NAME='stopAT'>";  
  for (int16_t index = 0; index <= 23; index++) {
    if (index == stop_AT) {
      if (index == 0)
        s += "<OPTION VALUE='" + String(index) + "'selected='SELECTED'>" + String(12) + ":00 AM</OPTION>";
      if (index >= 1 && index < 13)
        s += "<OPTION VALUE='" + String(index) + "'selected='SELECTED'>" + String(index) + ":00 AM</OPTION>";
      if (index == 12)
        s += "<OPTION VALUE='" + String(index) + "'selected='SELECTED'>" + String(12) + ":00 PM</OPTION>";
      if (index > 12 && index <= 23)
        s += "<OPTION VALUE='" + String(index) + "'selected='SELECTED'>" + String(index-12) + ":00 PM</OPTION>";
    }
    else{
      if (index == 0)
        s += "<OPTION VALUE='" + String(index) + "'>" + String(12) + ":00 AM</OPTION>";
      if (index >= 1 && index < 13)
        s += "<OPTION VALUE='" + String(index) + "'>" + String(index) + ":00 AM</OPTION>";
      if (index == 12)
        s += "<OPTION VALUE='" + String(index) + "'>" + String(12) + ":00 PM</OPTION>";
      if (index > 12 && index <= 23)
        s += "<OPTION VALUE='" + String(index) + "'>" + String(index-12) + ":00 PM</OPTION>";
    }
  }
  s += "</SELECT></P>";
  if (server.arg("save") == "    Save    "){
    String startAT = server.arg("startAT");
    String stopAT = server.arg("stopAT");  
    if (startAT.toInt() != start_AT){
      start_AT = startAT.toInt();
      writeEEPROMbyte(START_AT_START, start_AT);
    }
    if (stopAT.toInt() != stop_AT){
      stop_AT = stopAT.toInt();
      writeEEPROMbyte(STOP_AT_START, stop_AT);
    }
    if (start_AT != stop_AT)
      s += "<P><SPAN STYLE='COLOR:#00A000'>Saving...Please wait!</SPAN></P>";
    else{
      setAutoSchedule(0);
      s += "<P><SPAN STYLE='COLOR:#FF0000'>Error! Start and Stop times are the same</SPAN></P>";
    }
  }
  setAutoSchedule(1);
  s += "<P><INPUT TYPE=submit NAME='save' VALUE='    Save    '></P>";
  s += "</FORM><FORM ACTION='/'>";
  s += "<P><INPUT TYPE=submit VALUE='    Done    '></P>";
  s += "</FORM>";
  s += "</BODY></HTML>";
  server.send(200, "text/html", s);
}

void handleAutoTrackingSettings() {
  String s, hStr;

  header1(s);
  header2("Auto Tracking Settings", hStr);
  s += hStr;
  s += "<FORM ACTION='/atsetR'>";
  uint8_t found_default = 0;  //used if current default setting doesn't match drop down for cases if changed via RAPI command                
  
  s += "<P><LABEL>Priority is (1-254) </LABEL><INPUT TYPE='number' ID='priority' NAME='PRIORITY' MIN='1' MAX='254' VALUE='";
  s += device_priority;
  s += "'></P>";
  s += "<P>Cycle completes when reaching ";
  s += "<SELECT NAME='hiset'>";
  for (int index = MIN_CURRENT_CAPACITY_J1772; index <= set_current; index++) {
     if (index == hi_amp_setpoint) {
       s += "<OPTION VALUE='" + String(index) + "'SELECTED>" + String(index) + "</OPTION>";
     }
     else
       s += "<OPTION VALUE='" + String(index) + "'>" + String(index) + "</OPTION>";
  }
  s += "</SELECT>&nbsp;A</P>";
  s += "<P><LABEL>Wait time to adjust charging after turning on - EV dependant reaction time (15-240 seconds) </LABEL><INPUT TYPE='number' ID='wait_time' NAME='wait_time' MIN='15' MAX='240' VALUE='";
  s += on_off_wait_time;  
  s += "'></P>";
  for (uint16_t k = 0; k < strlen_P(m_submit); k++) {
    s += char(pgm_read_byte_near(m_submit + k));
  }
  server.send(200, "text/html", s);
}

void handleHome(){
  String s, hStr, tmpS;
  String sStart_hour;   
  String sStart_min;
  String sStop_hour;
  String sStop_min;
  float tmp;
  char tmpStr[40];
  String sFirst = "0";
  bool refresh = false;
  Timer12 = millis();   //reset LED timeout
  if (server.arg("level") == "Change to Level 1") {
    g_EvseController.SetSvcLevel(1);
    ipower = readEEPROMword(IPOWER_START);          //reload ipower
    refresh = true;
  }
  else if (server.arg("level") == "Change to Level 2") {
    g_EvseController.SetSvcLevel(2);                              
    ipower = readEEPROMbyte(IPOWER_START);        //reload ipower
    refresh = true;
  }
  //get pilot setting and EVSE flag
  updateFlag();
  if (server.arg("schedule_AT") == "Turn OFF Schedule"){
    setAutoSchedule(0);
    refresh = true;
  }
  header1(s);
  if (refresh)
    s += "<META HTTP-EQUIV='refresh' CONTENT='1; URL=/'>"; 
  else
    s += "<META HTTP-EQUIV='refresh' CONTENT='30; url=/'>"; 
  header2("Home", hStr);
  s += hStr;
  if (refresh)
    s += "<P><SPAN STYLE='COLOR:#00A000'>Saving...Please wait!</SPAN></P>";
  // for testing set_current = 48;
  s += "<P><B>EV Charger (EVSE) Status</B></P>";
  if (EVSE_READY && (evse_state != 5)){
    if (override_evse_set_pilot && !g_EvseController.IsBypassMode())
      s += "<P><SPAN STYLE='COLOR:#00A000'>EVSE is ready and the max current has been changed to ";
    else
      s += "<P><SPAN STYLE='COLOR:#00A000'>EVSE is ready and the max current is ";
    s += set_current;
    s += " A</SPAN></P>";
  }
  else { 
    if ((evse_type == JUICEBOX) && received_error != "")
      s += "<P><SPAN STYLE='COLOR:#FF0000'>EVSE is not ready, the error is " + received_error;
    else if (evse_state == 5)
      s += "<P><SPAN STYLE='COLOR:#FF0000'>EVSE is reporting an error";
    else {
      s += "<P><SPAN STYLE='COLOR:#FFB000'>EVSE is not ready or not connected to the ";
      s += deviceName();
      s += ", please investigate or check the connection";
    }
    s += "</SPAN></P>";
  }
  if (evse_tempS != "")
    s += evse_tempS;
  if (evse_state == 2)
    s += "<P>EVSE is now charging</P>";
  if (!g_EvseController.IsBypassMode()){
    if ((evse_type != JUICEBOX) || !solar_evse)
      s += "<FORM ACTION='/'>";
    s += "<P>EVSE is a Level ";
     if (g_EvseController.GetCurSvcLevel() == 1){
       s += "1 (120 Vac)";
       if ((evse_type != JUICEBOX) || !solar_evse)
        s += "&nbsp;&nbsp;<INPUT TYPE=submit NAME='level' VALUE='Change to Level 2'></P>";
       else
        s+= "</P>";
     }
     else {
       s += "2 (240 Vac)";
       if ((evse_type != JUICEBOX) || !solar_evse)
        s += "&nbsp;&nbsp;<INPUT TYPE=submit NAME='level' VALUE='Change to Level 1'></P>";
       else
        s+= "</P>";
     }
    if ((evse_type != JUICEBOX) || !solar_evse)
      s += "</FORM>";
    if (lifetime_energy > 10)
      s += "<P>The lifetime energy is " + String(lifetime_energy/1000.0, 2) + " kWh </P>";
  }
  uint8_t display_connected_indicator = 0;
  s += "<P>-----------------------------------------------</P>";
  s += "<P><B>";
  s += deviceName();
  s += " Status</B></P>";
  adapter_state = getAdapterState();  
  switch (adapter_state) {
    case ADAPTER_STATE_A:
      s += "<P><SPAN STYLE='background-color: #FFFF80'> ";
      s += deviceName();
      s += " is NOT connected to EV</SPAN></P>";    // vehicle state A - yellow
      break;
    case ADAPTER_STATE_B:
      if (sample_pp < (PP_NOT_PRESS + 20)){
        s += "<P><SPAN STYLE='background-color: #00FF26'> ";
        s += deviceName();
        s += " is connected to EV&#44; ready</SPAN></P>";       //&#44; is a comma, vehicle state B - green
      }
      else
        s += "<P><SPAN STYLE='background-color: #FFFF80'> EVSE plug's latch release button is NOT engaged or PP line has a bad connection</SPAN></P>";    // latch switch failure - yellow
      break;
    case ADAPTER_STATE_C:
      sFirst = "<P><SPAN STYLE='background-color: #0032FF'><SPAN STYLE='COLOR:#FFFFFF'> EV is charging for ";    // vehicle state C - blue
      sFirst += minutes_charging;
      if (minutes_charging == 1)
        sFirst += " minute delivering about ";
      else
        sFirst += " minutes delivering about ";
      tmp = kilowatt_seconds_accum/3600.0;
      if (header_received == "$TM"){   //for version 1
        sFirst += String(tmp,1) + " kWh with a charge current of  ";
        sFirst += String(evse_currentX10/10.0,1) + " A</SPAN></SPAN></P>";        
      }
      else {
        sFirst += String(tmp,1) + " kWh at max current setting of ";
        sFirst += String(pilot) + " A</SPAN></SPAN></P>";
      }
      s += sFirst;
      break;
    case ADAPTER_STATE_D:
      sFirst = "<P><SPAN STYLE='background-color: #FFB900'> EV is requesting venting</SPAN></P>";          // vehicle state D - amber
     // display_connected_indicator = 1;
      s += sFirst;
      break;
    case ADAPTER_STATE_DIODE_CHK_FAILED:
      sFirst = "<P><SPAN STYLE='background-color: #FF0000'><SPAN STYLE='COLOR:#FFFFFF'> ERROR, diode check failed</SPAN></SPAN></P>";  // red for error
      //display_connected_indicator = 1;
      s += sFirst;
      break;
    case ADAPTER_STATE_BYPASS:
      //Serial.println("bypass mode");
      if (adc_address == 0){
        sFirst = "<P><SPAN STYLE='background-color: #FFFF80'> ";
        sFirst += deviceName();
        sFirst += " is in Bypass Mode due to ADC failure, please contact support</SPAN></P>";          //forced bypass mode
      }
      else
      {
        sFirst = "<P><SPAN STYLE='background-color: #FFB900'> ";
        sFirst += deviceName();
        sFirst += " is in Bypass Mode</SPAN></P>";          //bypass mode
      }
      //display_connected_indicator = 1;
      s += sFirst;
      break;
    case ADAPTER_STATE_SLEEPING: 
      if (!g_EvseController.IsAutomaticMode()){
        if (g_EvseController.LimitSleepIsSet()){ //SetLimitSleep state
           sFirst = "<P><SPAN STYLE='background-color: #00A0FF'> ";
           sFirst += deviceName();
           sFirst += " is sleeping due to time/charge limit reached</SPAN></P>";  // bluegreen
           s += sFirst;
        }
        else if (timer_enabled){
           sFirst = "<P><SPAN STYLE='background-color: #00FFFF'> ";
           sFirst += deviceName();
           sFirst += " is sleeping and waiting for start time</SPAN></P>";  //cyan
           s += sFirst;
        }
        else {
           sFirst = "<P><SPAN STYLE='background-color: #FFFF80'> ";
           sFirst += deviceName();
           sFirst += " is sleeping</SPAN></P>";  //yellow
           s += sFirst;
        }
      }
      else {
         sFirst = "<P><SPAN STYLE='background-color: #FFFF80'> ";
         sFirst += deviceName();
         sFirst += " is sleeping</SPAN></P>";  //yellow
         s += sFirst;
      }
      display_connected_indicator = 1;
      break;
    default:
      sFirst = "<P><SPAN STYLE='background-color: #FFB900'> Unknown state</SPAN></P>";  //amber
      display_connected_indicator = 1;
      s += sFirst;
  }
  if (display_connected_indicator ==  1) {
    if (plugged_in)
      s += "<P><SPAN STYLE='background-color: #00FF26'> and is connected to EV</SPAN></P>";  // green plugged in 
    else
      s += "<P><SPAN STYLE='background-color: #FFFF80'> and is NOT connected to EV</SPAN></P>";  // yellow not plugged in
  }
  if (!g_EvseController.IsBypassMode()){
    if (EVSE_READY){      
      if (!g_EvseController.IsAutomaticMode()){
        s += "<FORM ACTION='/onR'>";
        if (adapter_enabled){
           s += "<P><INPUT TYPE=submit VALUE='Sleep/Stop Charging'></P>";
         }
         else {
           s += "<P><INPUT TYPE=submit VALUE='Start Charging Now'></P>";
         }
        s += "</FORM>"; 
        //sets charging schedule
        s += "<FORM ACTION='/dtimer'>";
        if (timer_enabled) {
          s += "<P>Scheduled Charging:</P>";
          if (start_hour < 10)
            sStart_hour = "0" + String(start_hour);
          else
            sStart_hour = String(start_hour);
          if (start_min < 10)
            sStart_min = "0" + String(start_min);
          else
            sStart_min = String(start_min);
          if (stop_hour < 10)
            sStop_hour = "0" + String(stop_hour);
          else
            sStop_hour = String(stop_hour);
          if (stop_min < 10)
            sStop_min = "0" + String(stop_min);
          else
            sStop_min = String(stop_min);         
          s += "<P>&nbsp;&nbsp;&nbsp;start at " + sStart_hour + ":" + sStart_min + "</P>";
          s += "<P>&nbsp;&nbsp;&nbsp;stop at " + sStop_hour + ":" + sStop_min + "</P>";
          s += "<INPUT TYPE=submit VALUE='   Change  '></FORM>";
          s += "<TABLE><TR>";
          s += "<TD><FORM ACTION='/offtimerR'><INPUT TYPE=submit VALUE='Turn OFF Scheduled Charging'></FORM></TD>";
          s += "</TR></TABLE>";
          s += "<BR>";
        }
        else {
          s += "<INPUT TYPE=submit VALUE='Turn ON Scheduled Charging'></P>";
          s += "</FORM>";
        }
        //sets limits
        s += "<FORM ACTION='/limits'>";
        if (double_time_limit > 0 || half_charge_limit > 0) {
          if (double_time_limit > 0 && half_charge_limit > 0)
            s += "<P>Limits per session (first to reach):</P>";
          else
            s += "<P>Limit per session:</P>";
          if (double_time_limit > 0) {
            s += "<P>&nbsp;&nbsp;&nbsp;Stop charging after ";
            tmp = double_time_limit * 0.5;
            if (tmp == 1.0)
              s += String(tmp,1) + " hour</P>";
            else
              s += String(tmp,1) + " hours</P>";
          }
          if (half_charge_limit > 0) {
            s += "<P>&nbsp;&nbsp;&nbsp;Stop charging after adding ";
            tmp = half_charge_limit * 2;
            s += String(tmp,0) + " kWh</P>";
          }          
          s += "<INPUT TYPE=submit VALUE='   Change  '></FORM>";
          s += "<TABLE><TR>";          
          s += "<TD><FORM ACTION='/offlimitsR'><INPUT TYPE=submit VALUE='Turn OFF Charging Limits'></FORM></TD>";
          s += "</TR></TABLE>";
        }
        else {
          s += "<INPUT TYPE=submit VALUE='Turn ON Charging Limits'></P>";
          s += "</FORM>";
        }
        s += "<FORM ACTION='/setpilot'>";
        s += "<P>Max charging current is ";
        s += pilot;
        s += " A";
        s += "&nbsp;&nbsp;<INPUT TYPE=submit VALUE='  Change  '></P>";
        s += "</FORM>";
      }  
      s += "<P>-----------------------------------------------</P>";
      s += "<P><B>Auto Tracking Settings</B></P>";     
      if (!no_consumed_data_received && !no_mqtt){
        //auto tracking       
        if (g_EvseController.IsAutomaticMode()) {
          s += "<P>Auto tracking is enabled</P>";
          s += "<P>&nbsp;&nbsp;&nbsp;Allow up to ";
          s += offset;
          s += " W (+ is import from the Grid, - is export to the Grid. Set at PowerMC)</P>";
          s += "<P>&nbsp;&nbsp;&nbsp;Priority is ";
          s += device_priority;
          s += "</P>";
          s += "<P>&nbsp;&nbsp;&nbsp;Cycle completes when reaching ";
          s += hi_amp_setpoint;
          s += "&nbsp;A</P>";
          s += "<P>&nbsp;&nbsp;&nbsp;Wait time for EV to start charging after turn on is ";
          s += on_off_wait_time;
          s += "&nbsp;secs</P>";
          s += "<TABLE><TR>";
          s += "<TD><FORM ACTION='/atset'><INPUT TYPE=submit VALUE=' Change '></FORM></TD>";
          if (!schedule_AT){
            s += "<TD><FORM ACTION='/offtrackingR'><INPUT TYPE=submit VALUE='Turn OFF Auto Tracking'></FORM></TD>";
          }
          s += "</TR></TABLE>";
        }
        else {
          if (!schedule_AT){
            s += "<FORM ACTION='/atset'>";
            s += "<P>Auto tracking is OFF&nbsp;&nbsp;<INPUT TYPE=submit VALUE='  Turn ON  '></P>";
            s += "</FORM>";
          }
          else
            s += "<P>Auto tracking is OFF</P>";
        }
        //auto tracking schedule
        if (schedule_AT) {
          s += "<P>Scheduled Auto Tracking </P>";
          s += "<P>&nbsp;&nbsp;&nbsp;Start at ";
          if (start_AT == 0)
            s += "12:00 AM";
          if (start_AT > 0 && start_AT < 12){
            s += start_AT;
            s +=":00 AM";
          }
          if (start_AT == 12)
            s += "12:00 PM";
          if (start_AT > 12 && start_AT <= 23) {
            s += start_AT - 12;
            s += ":00 PM";
          }
          s += "</P>";
          s += "<P>&nbsp;&nbsp;&nbsp;Stop at ";
          if (stop_AT == 0)
            s += "12:00 AM";
          if (stop_AT > 0 && stop_AT < 12){
            s += stop_AT;
            s +=":00 AM";
          }
          if (stop_AT == 12)
            s += "12:00 PM";
          if (stop_AT > 12 && stop_AT <= 23) {
            s += stop_AT - 12;
            s += ":00 PM";
          }
          s += "</P>";
          s += "<TABLE><TR>";
          s += "<TD><FORM ACTION='/atsch'><INPUT TYPE=submit VALUE=' Change '></FORM></TD>";
          s += "<TD><FORM ACTION='/'><INPUT TYPE=submit NAME='schedule_AT' VALUE='Turn OFF Schedule'></FORM></TD>";
          s += "</TR></TABLE>";
        }
        else {
          s += "<FORM ACTION='/atsch'><P>Auto tracking is not scheduled &nbsp;&nbsp;<INPUT TYPE=submit VALUE='  Schedule '></P>";
          s += "</FORM>";
        }
      }
      else
        s += "<P>Auto Tracking is not available because power data is not being received from the MQTT Broker</P>";     
    }
  }
  else
    if (adc_address != 0)
      s += "<P>You cannot control your EVSE with " + deviceName() + ". To change, see Advanced</P>";

  s += "<P>-----------------------------------------------</P>";
  s += "<P><B>Network Status</B></P>";
  displayConnectionStatus(tmpS);
  s += tmpS;
  s += "<P>-----------------------------------------------</P>";
  if (!no_consumed_data_received && !no_mqtt){
    s += "<P><B>Power Status</B></P>";
    s += "<P>Extra power detected is ";
    if (g_EvseController.IsAutomaticMode() && (selected_device_mac_address == WiFi.macAddress()) && (((extra_power < turn_off_threshold) && (adapter_state == ADAPTER_STATE_C)) || (extra_power > turn_on_threshold)))
      s += "<SPAN STYLE='COLOR:#FF0000'>"; //red
    else if (g_EvseController.IsAutomaticMode() && (selected_device_mac_address == WiFi.macAddress()) && (adapter_state == ADAPTER_STATE_C) && ((extra_power > increase_threshold) || (extra_power < decrease_threshold)))
      s += "<SPAN STYLE='COLOR:#FFB000'>"; //yellow
    else
      s += "<SPAN STYLE='COLOR:#00A000'>"; //green
       
    if (extra_power == 1 || extra_power == -1)
      sprintf(tmpStr,"%d watt</SPAN></P>",extra_power);
    else
      sprintf(tmpStr,"%d watts</SPAN></P>",extra_power);
    s += tmpStr;
    if (g_EvseController.IsAutomaticMode()){
      if (selected_device_mac_address == WiFi.macAddress()){
        s += "<P>Now adjusting this device</P>";
      }
    }
    if (number_of_devices > 0){
      s += "<P>Found ";
      s += number_of_devices;
      if (number_of_devices == 1)
        s += " device connected to the PowerMC</P>";
      else
        s += " devices connected to the PowerMC</P>";
      s += "<P>Now waiting on device with MAC address ";
      s += received_device_mac_address;
      s += " at ";
      s += received_device_ip_address;
      s += "</P>";
    }
    if (publish_done)
      s += "<P>Now waiting to send done response...</P>";
  }
  for (uint16_t k = 0; k < strlen_P(m_home); k++) {
    s += char(pgm_read_byte_near(m_home + k));
  }
  server.send(200, "text/html", s);
}


void handleAbout()
{
  String s, hStr;
  header1(s);
  header2("About",hStr);
  s += hStr;
  for (uint16_t k = 0; k < strlen_P(m_about); k++) {
    s += char(pgm_read_byte_near(m_about + k));
  }
  server.send(200, "text/html", s);
}

void downloadEEPROM() {
  String mem_check,tmp;
  mem_check = readEEPROM(MEMORY_CHECK_START, MEMORY_CHECK_MAX_LENGTH);
  if (mem_check != "B"){    //if memory hasn't been set up or corrupt
    resetEEPROM(0, EEPROM_SIZE);
  }
  else {
    //MQTT Broker
    boot_count = readEEPROMbyte(BOOT_START);
    boot_count++;
    writeEEPROMbyte(BOOT_START,boot_count);
    consumed = readEEPROM(CONSUMED_START, CONSUMED_MAX_LENGTH);
    if (consumed == "")
      consumed = "main";   //default is main
    consumedTopic = base + consumed;
    generated = readEEPROM(GENERATED_START, GENERATED_MAX_LENGTH);
    if (generated == "")
      generated = "solar";  //default is solar
    generatedTopic = base + generated;
    alt_set_current = readEEPROMbyte(ALT_SET_CURRENT_START);
    if (alt_set_current > MAX_CURRENT_CAPACITY_L2)
      alt_set_current = 0;
    device = readEEPROMbyte(DEVICE_START);
    if (device > MAX_NUM_DEVICES)
      device = 0;
    base2 = "2ms/";
    base2 += THIS_DEVICE;
    base2 += "/";
    base2 += String(device);
    base2 += "/";
    location_name = readEEPROM(LOCATION_START, LOCATION_MAX_LENGTH);
    if (location_name == "")
      location_name = "My Device";
    device_priority = readEEPROMbyte(DEVICE_PRIORITY_START);
    if (device_priority == 0)
      device_priority = 1;
    connection_mode = readEEPROMbyte(START_AP_START);
    if (connection_mode > AP)
      connection_mode = AP;
    user_set_adapter_mode = readEEPROMbyte(MODE_START);
    if (user_set_adapter_mode > ADAPTER_MODE_SLEEP)
      user_set_adapter_mode = ADAPTER_MODE_SLEEP;
    sw_config = readEEPROM(SW_CONFIG_START, SW_CONFIG_MAX_LENGTH);
    if (sw_config  != "a" && sw_config != "b")
      sw_config = "a";     //default to JSON
    ipower = readEEPROMword(IPOWER_START);
    hi_amp_setpoint = readEEPROMbyte(HI_AMP_SETPOINT_START);
    if (hi_amp_setpoint < MIN_CURRENT_CAPACITY_J1772)
      hi_amp_setpoint = MAX_CURRENT_CAPACITY_L1;
    saved_hi_amp_setpoint = hi_amp_setpoint;

    start_AT = readEEPROMbyte(START_AT_START);
    if (start_AT > 24)
      start_AT = 0;
    stop_AT = readEEPROMbyte(STOP_AT_START);
    if (stop_AT > 24)
      stop_AT = 0;
    schedule_AT = readEEPROMbyte(SCHEDULE_AT_START);
    if (schedule_AT > 1)
      schedule_AT = 1;
    solar_evse = readEEPROMbyte(SOLAR_EVSE_START);
    if (solar_evse > 1)
      solar_evse = 1;
    start_hour = readEEPROMbyte(START_HOUR_START);
    if (start_hour > 24)
      start_hour = 0;
    start_min = readEEPROMbyte(START_MIN_START);
    if (start_min > 60)
      start_min = 0;
    stop_hour = readEEPROMbyte(STOP_HOUR_START);
    if (stop_hour > 24)
      stop_hour = 0;
    stop_min = readEEPROMbyte(STOP_MIN_START);
    if (stop_min > 60)
      stop_min = 0;
    timer_enabled = readEEPROMbyte(TIMER_ENABLED_START);
    if (timer_enabled > 1)
      timer_enabled = 1;
    double_time_limit = readEEPROMbyte(DOUBLE_TIME_LIMIT_START);
    if (double_time_limit > DOUBLE_TIME_LIMIT_MAX)
      double_time_limit = 0;
    half_charge_limit = readEEPROMbyte(HALF_CHARGE_LIMIT_START);
    if (half_charge_limit > HALF_CHARGE_LIMIT_MAX)
      half_charge_limit = 0;
    no_mqtt = readEEPROMbyte(NO_MQTT_START);
    if (no_mqtt > 1)
      no_mqtt = 1;
    time_zone = readEEPROMbyte(TIME_ZONE_START);
    if (time_zone > 9)
      time_zone = 0;
    skip_pp = readEEPROMbyte(SKIP_PP_START);
    if (skip_pp > 1)
      skip_pp = 1;
    allow_on_temporarily = readEEPROMbyte(ALLOW_ON_START);
    if (allow_on_temporarily > 1)
      allow_on_temporarily = 0;
    evse_type = readEEPROMbyte(EVSE_TYPE_START);
    if (evse_type > 1)
      evse_type = 0;
    if (evse_type == JUICEBOX)
      g_EvseController.SetSvcLevel(2);
    on_off_wait_time = readEEPROMword(TRANSITION_WAIT_START);
    if (on_off_wait_time < 15 || on_off_wait_time > 240)  //in seconds
      on_off_wait_time = 15;
  }
}

void setup() {
  pinMode(RED_LED_0, OUTPUT);
  digitalWrite(RED_LED_0,LOW);      //indicates setting up
  //pinMode(PILOT, OUTPUT);  //since GPIO2 is used for PILOT and GPIO2 is also used for boot up as an input, changing it to an output here will turn on the blue LED on ESP12s, relocated during J1772 init()
  pinMode(PLUG_IN, OUTPUT);
  digitalWrite(PLUG_IN, LOW);
  pinMode(CHARGING, OUTPUT);
  digitalWrite(CHARGING,LOW);
  pinMode(USE_SA_PILOT, OUTPUT);
  digitalWrite(USE_SA_PILOT,HIGH);   //default should be using SA's pilot when powered
  pinMode(YELLOW_LED, OUTPUT);
  digitalWrite(YELLOW_LED,HIGH);
#ifdef PROTOTYPE
  Wire.begin();  //use default
#else
  Wire.begin(5,4);                 // Initialize adc, used to sample the pilot voltage
#endif
  Wire.setClock(400000);        // default is 100kHz but need faster  
  EEPROM.begin(600);
  // start the adapter related functions first before any network stuff in case there are delays
  g_EvseController.Init();  //sets up all the adapters initial parameters including start in sleep mode and bypass mode check
  delay(250);
  downloadEEPROM(); 
  if (evse_type == JUICEBOX) {
    Serial.begin(115200,SERIAL_8N1,SERIAL_RX_ONLY);  //GPIO1 or TX Pin is used to decode PWM of the pilot for the EVSE
    pilot_pin = EVSE_PILOT1; 
  }
  else {
    Serial.begin(115200,SERIAL_8N1,SERIAL_TX_ONLY);  //GPIO3 or RX Pin is used to decode PWM of the pilot for the EVSE
    pilot_pin = EVSE_PILOT3;
  }
  pinMode(pilot_pin,INPUT_PULLUP);
  g_EvseController.Update();
  delay(500);
  if (!g_EvseController.IsStartSleep() && !g_EvseController.IsBypassMode() && !g_EvseController.IsAutomaticMode()){
    if (user_set_adapter_mode == ADAPTER_MODE_ENABLED)
       g_EvseController.Enable();
  }
  //scan for ADC to make sure it's ready and determine which address is the ADC as part on PCB assy may be different (8 possible addresses)
  for (uint8_t i = 0 + 72; i < 8 + 72; i++)
  {
    Wire.beginTransmission (i);
    if (Wire.endTransmission () == 0)
      {
      //Serial.print ("Found ADC at address: ");
      //Serial.println (i, DEC);
      adc_address = i;
      } 
     delay(50);  // give devices time to recover
  } // end of for loop
  if (adc_address == 0){
    //Serial.println ("ERROR, ADC is not responding.");
    g_EvseController.SetAdapterBypass(1);  //set in bypass mode
  }

  String TIME_ZONE[10] = {"AST4","AST4ADT,M3.2.0,M11.1.0","EST5EDT,M3.2.0,M11.1.0","CST6CDT,M3.2.0,M11.1.0","MST7","MST7MDT,M3.2.0,M11.1.0","PST8PDT,M3.2.0,M11.1.0","AKST9AKDT,M3.2.0,M11.1.0","HST10","ChST-10"}; // https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
  configTime(TIME_ZONE[time_zone].c_str(), MY_NTP_SERVER);  //for NTP

  // now we can start all the network stuff amd connections
  // Tried to make network connections as easy as possible for those who are not technical
  // Sequence at boot up:  
  //  1. Automatic reconnect is enabled to connect to last connected network
  //  2. Waits about 20 secs to see if there is any connection to the last connect network (indicated by slow blinking Yellow LED at 1 time per sec). If connected, then done
  //  3. If no connection, try connection to the default network of PowerMC for 20 secs (indicated by slow blinking Yellow LED at 1 time per sec).  If connected, then done
  //  4. If no connection, go into WPS mode.  This will last about 150 seconds with the first 2 min is waiting for user to press the WPS button on their router/access point (indicated by solid red LED).  If connected, then will reboot and sequence will start over.
  //  5. If no connection, set flag to start in AP mode after reboot and reboot.
  //  6. Read flag and start in both AP and STA Mode (indicated by very fast blinking Red LED at 4 times per sec).  Waiting for user to manual configure or previous connected network to come online
  //  7. If successfully connected, AP Mode will be disconnected and LEDs will blink according to last 8-bit field of the assigned IP address
  //  8. If at any time the network becomes temporarily unavailable, the red LED will blink fast at 2 times per sec waiting for the network to be available so it can automatically reconnect
  
  enableWiFiAtBootTime();
  // in station mode
  WiFi.mode(WIFI_STA);
  if (connection_mode == AP)
    startAPMode();
  else
    connectWiFi();
  client.setCallback(callback);
  //client.setBufferSize(512);  //set MQTT max packet size to 512 bytes default is 256 if needed

  server.on("/confirm", handleCfm);
  server.on("/confirm2", handleCfm2);
  server.on("/wps", handleWPS);
  server.on("/wpsr", handleWPSR);
	server.on("/set", handleSettings);
  server.on("/cfg", handleCfg);
  server.on("/dbg", handleDbg);
  server.on("/dbgR", handleDbgR);
  server.on("/r", handleRapiR);
  server.on("/reset", handleRst);
  server.on("/reset2", handleRst2);
  server.on("/rapi", handleRapi);
  server.on("/", handleHome);
  server.on("/atsetR", handleAutoTrackingSettingsR);
  server.on("/atset", handleAutoTrackingSettings);
  server.on("/atsch", handleAutoTrackingSchedule);
  server.on("/dtimerR", handleDelayTimerR);
  server.on("/dtimer", handleDelayTimer);
  server.on("/adv", handleAdvanced);
  server.on("/onR", handleOnR);
  server.on("/advR", handleAdvancedR);
  server.on("/data", handleShowData);
  server.on("/commands", handleShowCommands);
  server.on("/offtimerR", handleOffTimerR);
  server.on("/offtrackingR", handleOffTrackingR);
  server.on("/limits", handleLimits);
  server.on("/limitsR", handleLimitsR);
  server.on("/offlimitsR", handleOffLimitsR);
  server.on("/setpilot", handleSetPilot);
  server.on("/setpilotR", handleSetPilotR);
  server.on("/ab", handleAbout);
	server.begin();
	//Serial.println("HTTP server started");
  delay(100);
  bootOTA();
  Timer = millis();
  Timer2 = millis();
  Timer3 = millis() - SA_STATUS_UPDATE_RATE;  //update data before first send
  Timer4 = millis();
  Timer6 = millis();
  Timer8 = millis();
  Timer_zero = millis();
  Timer10 = millis();
  Timer11 = millis();
  Timer12 = millis();
  Timer20 = millis();
  m_LastCheck = millis();
  start_time = micros();
  digitalWrite(RED_LED_0,HIGH);    //indicates done setting up
  immediate_status_update = false;  //need to clear this to allow assigned index to be updated first so that external UI is less confusing
  //Serial.println("Setup done");
  
}

ICACHE_RAM_ATTR void EvsePilotISR() {
  if (digitalRead(pilot_pin) == HIGH) {
    unsigned long tmp;
    tmp = micros();
    if (tmp > start_time)
      interval_time = uint16_t(tmp-start_time);
    else
      interval_time = uint16_t(4294967295 - tmp + start_time);
  }
  else
    start_time = micros();
}

void get_ip_address_for_LED() {
  //Serial.println(" ");
  //Serial.println("Connected as a Client");
  IPAddress myAddress = WiFi.localIP();
  //Serial.println(myAddress);
  //Serial.println("$FP 0 0 Client-IP.......");
  delay(100);
  if (myAddress[3] <= 9){
    first_ip_digit = 0;
    second_ip_digit = 0;
    third_ip_digit = myAddress[3];
    digits = 2;
  }
  else if (myAddress[3] <= 99){
    first_ip_digit = 0;
    second_ip_digit = myAddress[3]/10;
    third_ip_digit = myAddress[3] - second_ip_digit*10;  
    digits = 3;    
  }
  else {
    first_ip_digit = myAddress[3]/100;
    second_ip_digit = (myAddress[3] - first_ip_digit*100)/10;
    third_ip_digit = myAddress[3] - first_ip_digit*100 - second_ip_digit*10; 
    digits= 4;    
  }
  total_ip = first_ip_digit + second_ip_digit + third_ip_digit + digits;
  ip_position = 1;
  //sprintf(tmpStr,"$FP 0 1 %d.%d.%d.%d",myAddress[0],myAddress[1],myAddress[2],myAddress[3]);
  //Serial.println(tmpStr);
}

void updateRedLED() {
  uint16_t blink_rate;
  if (!WiFi.isConnected()){
    if (connection_mode == STARTED_IN_AP)
      blink_rate = FAST_BLINK_RATE/2; // Red LED blinks really fast if it is in ap mode
    else
      blink_rate = FAST_BLINK_RATE;  // Red LED blinks fast if not connected to saved network
  }
  else
    return;
  if (((millis() - Timer6) >= blink_rate)) {
    if (blink_LED1){
      blink_LED1 = false;
      digitalWrite(RED_LED_0,HIGH);
    }
    else{
      blink_LED1 = true;
      digitalWrite(RED_LED_0,LOW);
    }
    Timer6 = millis();
  }
}

void blinkYellowLED() {
  if (blink_LED2){
    blink_LED2 = false;
    digitalWrite(YELLOW_LED,HIGH);
    ip_position++;
   }
   else{
     blink_LED2 = true;
     digitalWrite(YELLOW_LED,LOW);
   }
}

void blinkRedLED() {
  if (blink_LED2){
    blink_LED2 = false;
    digitalWrite(RED_LED_0,HIGH);
    ip_position++;
   }
   else{
     blink_LED2 = true;
     digitalWrite(RED_LED_0,LOW);
   }
}

void updateYellowLED() {
  if (((millis() - Timer12) > LED_TIMEOUT) && WiFi.isConnected()) {
    digitalWrite(RED_LED_0,HIGH);
    digitalWrite(YELLOW_LED,HIGH);
    return;
  }
  else if (((millis() - Timer6) >= FAST_BLINK_RATE) && WiFi.isConnected()) {
    if (connection_mode == STARTED_IN_AP){
      WiFi.softAPdisconnect(true);
    }
    connection_mode = WIFI;   //currently connected to wifi
    
    IPAddress tmp = WiFi.localIP();
    if (currentAddress != tmp){
      get_ip_address_for_LED();
      currentAddress = tmp;
    }
    if (ip_position <= 2){   
      blinkRedLED();
      digitalWrite(YELLOW_LED,HIGH);
    }
    else {
      if (digits == 2){
        if (ip_position <= total_ip)
           blinkYellowLED();
        else
           ip_position = 1;
      }
      if (digits == 3){
        if (ip_position <= (2 + second_ip_digit))
           blinkYellowLED();
        else if (ip_position == (3 + second_ip_digit))  
           blinkRedLED();
        else if (ip_position <= total_ip)
           blinkYellowLED();
        else
           ip_position = 1;
      }
      if (digits == 4){
        if (ip_position <= (2 + first_ip_digit))
           blinkYellowLED();
        else if (ip_position == (3 + first_ip_digit))  
           blinkRedLED();
        else if (ip_position <= (3 + first_ip_digit + second_ip_digit))
           blinkYellowLED();
        else if (ip_position == (4 + first_ip_digit + second_ip_digit))  
           blinkRedLED();
        else if (ip_position <= total_ip)
           blinkYellowLED();
        else
           ip_position = 1;
       }
    }
    Timer6 = millis();   
  }
  else if (!WiFi.isConnected()){
    digitalWrite(YELLOW_LED,LOW);
    ip_position = 1;
    //Serial.println("wl not connected");
  }
}

void update_started() {
  //Serial.println("CALLBACK:  HTTP update process started");
}
 
void update_finished() {
  //Serial.println("CALLBACK:  HTTP update process finished");
}
 
void update_progress(int cur, int total) {
  //Serial.printf("CALLBACK:  HTTP update process at %d of %d bytes...\n", cur, total);
}
 
void update_error(int err) {
  //Serial.printf("CALLBACK:  HTTP update fatal error code %d\n", err);
}

bool actualUpdate() {
    char updateUrl[45];
    strcpy(updateUrl,UPDATE_URL);
    strcat(updateUrl,THIS_DEVICE_UPDATE);
    t_httpUpdate_return ret;
    // Create a WiFiClientSecure object.
    BearSSL::WiFiClientSecure updateClient;
    bool mfln = updateClient.probeMaxFragmentLength(UPDATE_HOST, UPDATE_PORT, 1024);  // server must be the same as in ESPhttpUpdate.update() and CPU Frequency needs to be 160 MHz, won't work with 80 MHz
    //Serial.printf("MFLN supported: %s\n", mfln ? "yes" : "no");
    if (mfln) {
      updateClient.setBufferSizes(1024, 1024);
    }
    updateClient.setInsecure();
    ESPhttpUpdate.setAuthorization("<username>","<password>");
    ESPhttpUpdate.rebootOnUpdate(false);
    //Serial.println("Step 1");
    ret=ESPhttpUpdate.update(updateClient,updateUrl,VERSION);     // **************** This is the line that "does the business"   
    //Serial.print("Step 2");
    if(ret!=HTTP_UPDATE_NO_UPDATES){
      if(ret==HTTP_UPDATE_OK){
        //Serial.println("UPDATE SUCCEEDED");
        return true;
        }
      else {
        if(ret==HTTP_UPDATE_FAILED){
          //Serial.println("Upgrade Failed");
          }
        }
      }
  return false;
}

void firmwareUpdateCheck(uint8_t now) {
  if (((millis() - Timer2) >= (FIRMWARE_UPDATE_CHECK + factor*100)) || now){
    factor = random(200); 
    Timer2 = millis();  
    if (WiFi.isConnected()){   //check for fw updates
      //Serial.println("inside fw check");
      if (actualUpdate()){
          reboot = 1;
      }
    }
  }
}

void processMQTTData() {
  if (received_pm_st){
    JsonDocument doc1;
    deserializeJson(doc1,pm_st_payload);
    consumed_power = doc1[consumed];
    consumed_data_status = 1;
    generated_power = doc1[generated];
    wait_for_power_change = float(doc1["uprate"])*2000;
    extra_power = doc1["extra"];
    received_pm_st = false;
  }
  if (received_pm_next){
    processReceivedNextData((char*)pm_next_payload);
    received_pm_next = false;
  }
  if (received_auto){
    if (auto_message == "true" || auto_message == "1" || auto_message == "1.0")
      g_EvseController.SetAutomaticMode(1); 
    else
      g_EvseController.SetAutomaticMode(0);
    received_auto = false;
  }
  if (received_priority){
    int value = priority_message.toInt();
    if (value >= 1 && value <= 254){ 
      if (value != device_priority){
        device_priority = value;   
        writeEEPROMbyte(DEVICE_PRIORITY_START, device_priority);
        immediate_status_update = true;
        forced_reset_priority = true;
      }
    }
    received_priority = false;
  }
  if (!g_EvseController.IsBypassMode()){
    if (received_enabled){
      if (enabled_message != String(adapter_enabled)){
        if (enabled_message == "1"){
          g_EvseController.Enable();
          if (user_set_adapter_mode != ADAPTER_MODE_ENABLED){
            user_set_adapter_mode = ADAPTER_MODE_ENABLED;
            writeEEPROMbyte(MODE_START,user_set_adapter_mode);
          }
        }  
        else if (enabled_message == "0"){
           g_EvseController.Sleep();
           if (user_set_adapter_mode != ADAPTER_MODE_SLEEP) {
            user_set_adapter_mode = ADAPTER_MODE_SLEEP;
            writeEEPROMbyte(MODE_START,user_set_adapter_mode);
           }
        }
        g_EvseController.SetAutomaticMode(0);
      }
      received_enabled = false;
    }
    if (received_pilot){
      if (pilot_message != String(pilot)){
          g_EvseController.SetCurrentCapacity(pilot_message.toInt());
          g_EvseController.SetAutomaticMode(0);
      }
      received_pilot = false; 
    }
    if (received_schedule_AT){
      if (schedule_AT_message != String(schedule_AT)){
        setAutoSchedule(schedule_AT_message.toInt());
      }
      received_schedule_AT = false;
    }
    if (received_timer){
      if (timer_message != String(timer_enabled)){
        setDelayTimer(timer_message.toInt());
      }
      received_timer = false;
    }
  }
  else {
    received_enabled = false;
    received_pilot = false;
    received_schedule_AT = false;
    received_timer = false;
  }
  if (received_bypass){
    if (bypass_message != String(g_EvseController.IsBypassMode())){
      g_EvseController.SetAdapterBypass(bypass_message.toInt());
    }
    received_bypass = false; 
  }
  if (received_level){
    if (level_message != String(g_EvseController.GetCurSvcLevel())){
      g_EvseController.SetSvcLevel(level_message.toInt()); 
    }
    received_level = false;
  }
  if (received_hiamp){
    if (hiamp_message != String(hi_amp_setpoint)){
      setHiAmpSetpoint(hiamp_message.toInt());
    }
    received_hiamp = false;
  }
}

void callback(char* topic, byte* payload, uint16_t length) {
 //Serial.print("Message arrived [");
 //Serial.print(topic);
 //Serial.print("] ");
 char tmpStr[20];
 String tmp2;
 const char* PM_devs_topic = "2ms/PM/devs";
 /*for (uint16_t i=0;i<length;i++) {
  char receivedChar = (char)payload[i];
  Serial.print(receivedChar);
  }*/
  //Serial.println();
  payload[length] = '\0';
  String message = (char*)payload;
  
  if (String(topic) == String(PM_devs_topic)){       //need to processs as soon as possible as pm devs are asynchronous
      processReceivedPMData((char*)payload);
  }
  if (String(topic) == base +"s"){  //need to processs as soon as possible as pm devs are asynchronous
    if (message == "1")  // every XX secs this device will sync to the PowerMC plus an offset based on the number of devices
      Timer_zero = millis();
  }
  if (sw_config == "a"){
    tmp2 = base + "st";
    if (String(topic) == tmp2 ){
      pm_st_payload = payload;
      received_pm_st = true;
    }  
  }
  else if (sw_config == "b"){
    if ((String(topic) == consumedTopic) || (String(topic) == generatedTopic)) {
      if (String(topic) == consumedTopic){
          consumed_power = message.toFloat();  //assuming worse case number type
          consumed_data_status = 1;
      /*    Serial.print("Consumed power = ");
          sprintf(tmpStr,"%.2f",consumed_power);
          Serial.print(tmpStr);*/
      }
      if (String(topic) == generatedTopic){
          generated_power = message.toFloat();  //assuming worse case number type
    /*      Serial.print("Generated power = ");
          sprintf(tmpStr,"%.2f",generated_power);
          Serial.print(tmpStr);*/
      }
      extra_power = (int32_t)(generated_power - consumed_power);
    }
    tmp2 = base + "uprate";
    if (String(topic) == tmp2){ 
        wait_for_power_change = message.toFloat()*2000; //assuming worse case number type
    }
  }
  
  tmp2 = base + "next";
  if (String(topic) == tmp2){
    pm_next_payload = payload;
    received_pm_next = true;
    if (one_second == 0)
      m_LastCheck = millis();   //sync the checkTimer function with receiving the next command which is used to determine when to do light sleep.  
  }
  tmp2 = base + "devs";               //need to processs as soon as possible as dev topics are asynchronous
  if (String(topic) == tmp2){
    processReceivedDeviceData((char*)payload);
  }
  if ((millis() - Timer8) >= DELAY_START) {
    ready_to_start = true;
    tmp2 = base2 + "c/auto";
    if (String(topic) == tmp2){
      auto_message = message;
      received_auto = true;          
    }      
    tmp2 = base2 + "c/priority";
    if (String(topic) == tmp2){
      priority_message = message;
      received_priority = true;
    }
    tmp2 = base2 + "c/enabled";
    if (String(topic) == tmp2){
      enabled_message = message;
      received_enabled = true;
    }
    tmp2 = base2 + "c/bypass";
    if (String(topic) == tmp2){
      bypass_message = message;
      received_bypass = true;
    }
    if (evse_type != JUICEBOX) {
      tmp2 = base2 + "c/level";
      if (String(topic) == tmp2){
        level_message = message;
        received_level = true;
      }
    }
    tmp2 = base2 + "c/hiamp";
    if (String(topic) == tmp2){
      hiamp_message = message;
      received_hiamp = true;
    }
    
    tmp2 = base2 + "c/pilot";
    if (String(topic) == tmp2 && !g_EvseController.IsBypassMode()){
      pilot_message = message;
      received_pilot = true;
    }

    tmp2 = base2 + "c/schedat";
    if (String(topic) == tmp2){
      schedule_AT_message = message;
      received_schedule_AT = true;
    }
    
    tmp2 = base2 + "c/timer";
    if (String(topic) == tmp2){
      timer_message = message;
      received_timer = true;
    }
  }
 //Serial.println();
}

boolean reconnect() {
  String uName;
  String pWord;
  String MQTTBroker;
  String port;
  String tmp,tmp2;
  String PM_subscription = "2ms/PM/#";
  tmp = THIS_DEVICE + WiFi.macAddress();  //clientID
  uName = readEEPROM(UNAME_START, UNAME_MAX_LENGTH);
  pWord = readEEPROM(PWORD_START, PWORD_MAX_LENGTH);
  port = readEEPROM(MQTT_PORT_START, MQTT_PORT_MAX_LENGTH);
  if (port == "")  
    port = DEFAULTMPORT;
 //MQTT Broker
  MQTTBroker = readEEPROM(MQTTBROKER_START, MQTTBROKER_MAX_LENGTH);
  if (MQTTBroker == "")
    MQTTBroker = DEFAULTMSERVER;
  if (WiFi.SSID() == DEFAULTSSID){
    MQTTBroker = POWERMC_MSERVER;
    port = DEFAULTMPORT;
  }
  client.setServer(MQTTBroker.c_str(),port.toInt());
  //mqtt_client_id, NULL, NULL, NULL, 0, false, NULL, true
  if (client.connect(tmp.c_str(),uName.c_str(),pWord.c_str(), NULL, 0, false, NULL, true)) {  
    tmp2 = PM_subscription;
    client.subscribe(tmp2.c_str());  
    tmp2 = base2 + "c/#";
    client.subscribe(tmp2.c_str());
    server2_down = false;
  }
  return client.connected();
}

void automaticTrackingUpdate() {
  //get max current allowable
  uint8_t maxamp;
  uint16_t increment;
  char tmpStr[20];
  uint16_t power_step = g_EvseController.GetCurSvcLevel()*SPLIT_PHASE_VOLTAGE;
  uint16_t min_power_on = ipower;
  
  if (hi_amp_setpoint > set_current)
    maxamp = set_current;
  else
    maxamp = hi_amp_setpoint;
  
  if (offset >= 0){   // limit offset amount pulled from the grid
    turn_on_threshold = min_power_on - offset + 100;
    increase_threshold = offset < power_step?(power_step - offset):0;  //limit to -offset or -powerstep depending on which is larger
    decrease_threshold = -offset;   
    turn_off_threshold = -offset;
  }
  else{   //limit offset amount pushed to the grid
    turn_on_threshold = -offset;    
    increase_threshold = -offset;
    decrease_threshold = -power_step - offset; 
    turn_off_threshold = -min_power_on - offset; 
  }

  if (((millis() - Timer4) > (transition*1000 + wait_for_power_change/2 + 1500)) && temporarily_done == 1){
    publish_done = true;
    temporarily_done = 0;
    transition = 0;
    Timer4 = millis();
  }
  // next 3 ifs are for the start timeout feature
  if ((selected_device_mac_address == WiFi.macAddress()) && (received_time_on == 255) && (temporarily_done == 1 || start_timeout)){  //abort if returned eariler
    start_timeout = false;
    temporarily_done = 0;
    publish_done = true;
    transition = 0;
  }
  if ((selected_device_mac_address == WiFi.macAddress()) && (received_time_on != 255) && !start_timeout){
    start_timeout = true;
    Timeout_start_time = millis();
  }
  if (((millis() - Timeout_start_time) > (received_time_on * 1000)) && start_timeout){
    g_EvseController.Sleep();
    transition = 2;
    Timer4 = millis();
    start_timeout = false;
    forced_reset_priority = true;
  }
  if ((selected_device_mac_address == WiFi.macAddress()) && ready_to_start && (millis() - Timer4) > (transition*1000 + wait_for_power_change/2 + 1500)){
    Timer4 = millis();
    assigned_index = current_done_index;
    if ((!down_scan && HIGHER_PRIORITY_CONDITION) || (down_scan && LOWER_PRIORITY_CONDITION) || !EVSE_READY){
      publish_done = true;
      transition = 0;    
    } 
    else if (plugged_in && (g_EvseController.IsAutomaticMode() == 1)){     
      //Serial.print("Extra power = ");
      //sprintf(tmpStr,"%d",extra_power);
      //Serial.print(tmpStr);
      //Serial.println();
      if ((adapter_enabled == 0) && (extra_power > turn_on_threshold) && !down_scan){  // turn on
        increment = (extra_power - turn_on_threshold)/(1.2*power_step);
        g_EvseController.SetCurrentCapacity(MIN_CURRENT_CAPACITY_J1772 + increment,1);  //no save to eeprom
        g_EvseController.Enable();
        publish_done = true;    //temporarily indicate done so others can use up the power while waiting for car to start
        temporarily_done = 1;
        transition = on_off_wait_time; //was 18
        }
      else if (adapter_enabled && (extra_power > increase_threshold) && (pilot < maxamp)){  // increase
        increment = 1 + (extra_power - increase_threshold)/(1.2*power_step);
        pilot += increment;
        if (pilot > maxamp)
          pilot = maxamp;
        g_EvseController.SetCurrentCapacity(pilot,1);  //no save to eeprom
        transition = 3; //was 4
      }
      else if (adapter_enabled && (extra_power < decrease_threshold) && (pilot > MIN_CURRENT_CAPACITY_J1772)){  // decrease
        increment = 1 + (decrease_threshold - extra_power)/(1.2*power_step);
        if (increment > pilot) //pilot can't be negative due to variable definition
          increment = pilot;
        pilot -= increment;
        if (pilot < MIN_CURRENT_CAPACITY_J1772)
          pilot = MIN_CURRENT_CAPACITY_J1772;
        g_EvseController.SetCurrentCapacity(pilot,1);  //no save to eeprom
        transition = 3;  //was 4
      }
      else if (adapter_enabled && (extra_power < turn_off_threshold) && (pilot == MIN_CURRENT_CAPACITY_J1772) && down_scan){  //turn off
         g_EvseController.Sleep();
         transition = 2;  
      }
    }
  }
}

void publishData() {
   String tmp, tmp1, tmp2;
   if ((millis() - Timer) >= SERVER_UPDATE_RATE || publish_immediately) {
    Timer = millis();
    publish_immediately = false;
      
    // We now publish the data
    //Serial.println("Begin publishing data....");
    String adapter_state_text;
    char tmpStr[40];
    IPAddress myAddress = WiFi.localIP();
    long rssi = WiFi.RSSI();
    tmp1 = WiFi.SSID();
    tmp1 += " at ";
    sprintf(tmpStr,"%d.%d.%d.%d with strength %ld dBm",myAddress[0],myAddress[1],myAddress[2],myAddress[3],rssi);
    tmp1 += tmpStr;
    JsonDocument doc;
    doc["connect"] = tmp1;
    doc["auto"] = g_EvseController.IsAutomaticMode();
    doc["pilot"] = pilot; 
    doc["plugin"] = plugged_in;
    doc["dprior"] = device_priority;
    doc["enabled"] = adapter_enabled;
    doc["level"] = g_EvseController.GetCurSvcLevel();
    getStateText(adapter_state, adapter_state_text); 
    doc["sttxt"] = adapter_state_text;
    uint8_t evse_ready;
    if (EVSE_READY)
      evse_ready = set_current;
    else
      evse_ready = 0;
    doc["ready"] = evse_ready;
    doc["hiamp"] =  hi_amp_setpoint;
    doc["schedat"] = schedule_AT;
    doc["timer"] = timer_enabled;
    if (evse_tempS != "")
      doc["tempc"] = evse_tempC;
    if (header_received == "$TM"){   //for version 1
      doc["e_cur"] = evse_currentX10;
      doc["e"] = lifetime_energy;
    }
    doc.shrinkToFit();
    serializeJson (doc,out1);
    tmp2 = base2 + "st";
    if (!server2_down){
      if (!client.publish(tmp2.c_str(),out1.c_str()))
        server2_down = true;
    }
   }
   if ((assigned_index + 1) > number_of_devices)
      assigned_index = 0;
   uint8_t denom;
   if (number_of_devices == 0)
      denom = 1;
   else
      denom = number_of_devices;
   int mod = (millis() - Timer_zero)%REPORTING_IN_RATE;
   int time_slot = 1000*(assigned_index + 1)*MAX_NUM_DEVICES/denom;
   bool low;
   if (mod <= time_slot)
      low = true;
   else 
      low = false;
   if (low == false && pre_low == true){  //look for transition
      JsonDocument doc1;
      IPAddress myAddress = WiFi.localIP();
      char tmpStr[20];
      sprintf(tmpStr,"%d.%d.%d.%d",myAddress[0],myAddress[1],myAddress[2],myAddress[3]);
      tmp1 = tmpStr;      
      doc1["ip"] = tmp1;
      doc1["dpriority"] = device_priority;
      doc1["dname"] = location_name;
      doc1["dnumber"] = device;
      doc1["dtype"] = THIS_DEVICE;
      String mDNS;
      if (solar_evse)
        mDNS = APSSID_E;
      else
        mDNS = APSSID;
      mDNS += device;
      doc1["mdns"] = mDNS;
      doc1["mac"] = WiFi.macAddress();
      doc1["ipower"] = ipower;
      doc1["tempon"] = on_off_wait_time;
      doc1.shrinkToFit();
      serializeJson (doc1,out2);
      tmp2 = base + "devs";  
      if (!server2_down){
        if (!client.publish(tmp2.c_str(),out2.c_str()))
          server2_down = true;
      }
      
      uint8_t maxamp;
      if (hi_amp_setpoint > set_current)
        maxamp = set_current;
      else
        maxamp = hi_amp_setpoint;
      tmp2 = base + "slow_load";      
      if (plugged_in && (g_EvseController.IsAutomaticMode() == 1) && !(pilot >= maxamp))
        slow_load = "0";        
      else 
        slow_load = "1";
      if (!server2_down)
         if (!client.publish(tmp2.c_str(),slow_load.c_str()))
            server2_down = true;
   }
   pre_low = low;
}

void checkBrokerConnection() {
  if (!client.connected()) {
    if (connection_mode == WIFI_MQTT)
      server2_down = true;  // to do:  need to get the error message out to help troubleshoot
    Timer20 = millis(); //reset timer for when to check PMs
    if (millis() - lastReconnectAttempt > 5000) {
      //Serial.println("Broker not connected");
      extra_power = 0; //reset extra power in case current sensor stops sending data 
      generated_power = 0;
      consumed_power = 0;
      lastReconnectAttempt = millis();
      // Attempt to reconnect
      if (reconnect()) {
        lastReconnectAttempt = 0;
        server2_down_reconnect_attempt = 0;
      }
      else {
        if ((server2_down_reconnect_attempt > 24) && (connection_mode == WIFI_MQTT))
           reboot = 1;  //needed because once down for whatever reason (bug?) sometimes will not recover unless reset
        server2_down_reconnect_attempt++;
      }
    }
  } 
  else {
    // Client connected
    client.loop();
    server2_down = false;
    connection_mode = WIFI_MQTT;
  }
}

void checkDuplicateDeviceNumbers(String& r_mdns, String& r_mac) {
  if (r_mac != WiFi.macAddress()){
    String mDNS;
    if (solar_evse)
      mDNS = APSSID_E;
    else
      mDNS = APSSID;
    mDNS += device;
    if (r_mdns == mDNS){
      device++;
      if (device >= MAX_NUM_DEVICES)
        device = 0;
      writeEEPROMbyte(DEVICE_START, device);
      reboot = 1;
    }
  }
}

uint8_t IsTimerValid() {
     if (start_hour || start_min || stop_hour || stop_min){ // Check not all equal 0
       if ((start_hour == stop_hour) && (start_min == stop_min)){ // Check start time not equal to stop time
         return 0;
       } else {
         return 1;
       }
     } else {
       return 0; 
     }
  }
  
void checkTimer() {
  float tmp;
  tmp = kilowatt_seconds_accum/7200.0;
  if ((millis() - m_LastCheck) >= 500){
    m_LastCheck = millis();
    one_second++;
    reset_sleep = true;
    if (one_second%2 == 0) {
      one_second = 0;
      updatePluggedInStatus();
      if (solar_evse && (evse_type == JUICEBOX)) {   //checking serial port for messages from evse microcontroller vs via checkevsestatus because needs to be faster to prevent buffer overrun
        uint8_t start_substring, end_substring;
        String s, l, h, a, f, t, tf, v, i, p, e;
        String received, sub;
        if (Serial.available() > 0){
         received = Serial.readStringUntil('\n');
         if (received.substring(0,3) == "$TM"){   //for version 1
          header_received = received.substring(0,3);
          start_substring = received.indexOf(':') + 1;
          end_substring = received.indexOf(':', start_substring);
          received_from_evse = received.substring(start_substring, end_substring);
          sub = received_from_evse.substring(received_from_evse.indexOf('s') + 1);
          s = sub.substring(0,sub.indexOf(','));
          evse_state = s.toInt();
          sub = received_from_evse.substring(received_from_evse.indexOf('t') + 1);
          t = sub.substring(0,sub.indexOf(','));
          sub = received_from_evse.substring(received_from_evse.indexOf('e') + 1);
          e = sub.substring(0,sub.indexOf(','));
          lifetime_energy = e.toInt();
          evse_tempC = t.toInt();
          tf = String((9.0*evse_tempC/5.0 + 32.0),1);
          evse_tempS = "<P>EVSE temp is " + String(evse_tempC) + " deg C or " + tf + " deg F</P>";
          sub = received_from_evse.substring(received_from_evse.indexOf('v') + 1);
          v = sub.substring(0,sub.indexOf(','));
          evse_voltageX10 = v.toInt();
          sub = received_from_evse.substring(received_from_evse.indexOf('i') + 1);
          i = sub.substring(0,sub.indexOf(','));
          evse_currentX10 = i.toInt();
          sub = received_from_evse.substring(received_from_evse.indexOf('p') + 1);
          if (sub.substring(0,1) == "-") { 
            p = sub.substring(1,sub.indexOf(','));
            evse_pfX1000 = p.toInt();
            if (evse_pfX1000 < 300)         // in case readings are incorrect intermittently during charging as it has been reported on certain versions
              evse_pfX1000 = 990;           // even though pf is used in the calculation for power, it's not that important for ev charging so assuming near 1 pf is good
          }
          sub = received_from_evse.substring(received_from_evse.indexOf('R') + 1);
          h = sub.substring(0,sub.indexOf(','));
          sub = received_from_evse.substring(received_from_evse.indexOf('A') + 1);
          a = sub.substring(0,sub.indexOf(','));
          if (h.toInt() >= a.toInt())
            evse_max = h.toInt();
          else
            evse_max = a.toInt();
         }
         if (received.substring(0,3) == "$ES"){   //for version 2
          start_substring = received.indexOf(':') + 1;
          end_substring = received.indexOf(':', start_substring);
          received_from_evse = received.substring(start_substring, end_substring);
          sub = received_from_evse.substring(received_from_evse.indexOf('S') + 1);
          s = sub.substring(0,sub.indexOf(','));
          evse_state = s.toInt();
          sub = received_from_evse.substring(received_from_evse.indexOf('T') + 1);
          t = sub.substring(0,sub.indexOf(','));
          evse_tempC = t.toInt();
          tf = String((9.0*evse_tempC/5.0 + 32.0),1);
          evse_tempS = "<P>EVSE temp is " + String(evse_tempC) + " deg C or " + tf + " deg F</P>";
          sub = received_from_evse.substring(received_from_evse.indexOf('H') + 1);
          h = sub.substring(0,sub.indexOf(','));
          sub = received_from_evse.substring(received_from_evse.indexOf('A') + 1);
          a = sub.substring(0,sub.indexOf(','));
          sub = received_from_evse.substring(received_from_evse.indexOf('F') + 1);
          f = sub.substring(0,sub.indexOf(','));
          if (h.toInt() >= a.toInt())
            evse_max = h.toInt();
          else
            evse_max = a.toInt();
          if (evse_max < f.toInt())
            evse_max = f.toInt();
         }
         if (received.substring(0,3) == "$WR"){
          received_error = received.substring(received.indexOf(':') + 1, received.length() - 5);
         }
        }
      }
      time(&now);                       // read the current time
      localtime_r(&now, &tm);
      if (schedule_AT && !no_consumed_data_received && !no_mqtt){
        if (stop_AT > start_AT){
          if (tm.tm_hour >= start_AT && tm.tm_hour < stop_AT){
            if (!g_EvseController.IsAutomaticMode())
              g_EvseController.SetAutomaticMode(1,1);
          }
          else {
            if (g_EvseController.IsAutomaticMode())
              g_EvseController.SetAutomaticMode(0,1);
          }
        }
        if (stop_AT < start_AT){
          if (tm.tm_hour >= start_AT || tm.tm_hour < stop_AT){
            if (!g_EvseController.IsAutomaticMode())
              g_EvseController.SetAutomaticMode(1,1);
          }
          else{
            if (g_EvseController.IsAutomaticMode())
              g_EvseController.SetAutomaticMode(0,1);
          }
        }
      } 
      if (!g_EvseController.IsAutomaticMode() && EVSE_READY) {
        if ((double_time_limit > 0) && (minutes_charging >= double_time_limit*30) && !g_EvseController.LimitSleepIsSet() && (adapter_state == ADAPTER_STATE_C)){  //time limit reached
          g_EvseController.SetLimitSleep(1);     
          g_EvseController.Sleep();
          if (timer_started){
            limits_reached = true;
          }
        }
        else if ((half_charge_limit > 0) && (tmp >= half_charge_limit) && !g_EvseController.LimitSleepIsSet() && (adapter_state == ADAPTER_STATE_C)){  //charge limit reached
          g_EvseController.SetLimitSleep(1);     
          g_EvseController.Sleep();
          if (timer_started){
            limits_reached = true;
          }
        }
        if (timer_enabled && IsTimerValid()) {
          //Serial.println("inside time delay"); 
          uint16_t startTimerMinutes = start_hour * 60 + start_min; 
          uint16_t stopTimerMinutes = stop_hour * 60 + stop_min;
          uint16_t currTimeMinutes = tm.tm_hour * 60 + tm.tm_min;
          
          if (stopTimerMinutes < startTimerMinutes) {
            //End time is for next day 
            if ((currTimeMinutes >= startTimerMinutes) || (currTimeMinutes < stopTimerMinutes)){
              // Within time interval
              timer_started = true;
              if (!adapter_enabled && !g_EvseController.LimitSleepIsSet() && !limits_reached) {
                g_EvseController.Enable();
              }           
            }
            else if ((currTimeMinutes >= stopTimerMinutes) && (currTimeMinutes < startTimerMinutes)) { 
              // Not in time interval
              timer_started = false;
              limits_reached = false;
              g_EvseController.SetLimitSleep(0);
              if (adapter_enabled){     
                g_EvseController.Sleep();
              }
            }
          }
          else { // not crossing midnite
            if ((currTimeMinutes >= startTimerMinutes) && (currTimeMinutes < stopTimerMinutes)){
              // Within time interval
              timer_started = true;
              if (!adapter_enabled && !g_EvseController.LimitSleepIsSet() && !limits_reached)
                g_EvseController.Enable();           
            }
            else if ((currTimeMinutes >= stopTimerMinutes) || (currTimeMinutes < startTimerMinutes)){ 
              // Not in time interval
              timer_started = false;
              limits_reached = false;
              g_EvseController.SetLimitSleep(0);
              if (adapter_enabled)
                g_EvseController.Sleep();
            }
          }
        }
      }
    }
  }
}

void processReceivedPMData(char* payload) {
  char* tmpj;
  JsonDocument doc;
  tmpj = payload;
  deserializeJson(doc,tmpj);
  for (uint8_t i = 0; i < current_num_pm; i++){      //same device reporting again within the timeout
    if (String(doc["mac"]) == rec_pm_mac[i]){
      return;
    }
  }
  rec_pm_mac[current_num_pm] = String(doc["mac"]);
  rec_pm_dnum[current_num_pm] = doc["dnum"];
  rec_pm_ip[current_num_pm] = String(doc["ip"]);
  current_num_pm++;
}

void checkPM() {
  String saved_pm_mac_address;
  bool selected_pm_still_alive = false;
  uint8_t lowest = 0;
  if ((millis() - Timer20) > PM_CYCLE_TIME){
    saved_pm_mac_address = readEEPROM(PM_MAC_START, PM_MAC_MAX_LENGTH);
    Timer20 = millis();
    if (current_num_pm == 1){
      processed_pm_mac[0] = rec_pm_mac[0];
      processed_pm_dnum[0] = rec_pm_dnum[0];
      processed_pm_ip[0] = rec_pm_ip[0];
    }
    if (current_num_pm == 1 && ((saved_pm_mac_address == "") || (saved_pm_mac_address == rec_pm_mac[0]))){
      base = "2ms/PM/" + String(rec_pm_dnum[0]) + "/";
      selected_pm_mac_address = rec_pm_mac[0];
    }
    else
      selected_pm_mac_address = "0";
    if (current_num_pm > 1){
      for (uint8_t index = 0; index < current_num_pm; index++){
        processed_pm_mac[index] = rec_pm_mac[index];
        processed_pm_dnum[index] = rec_pm_dnum[index];
        processed_pm_ip[index] = rec_pm_ip[index];
        if (saved_pm_mac_address == rec_pm_mac[index]){
          selected_pm_still_alive = true;
          base = "2ms/PM/" + String(rec_pm_dnum[index]) + "/";
          selected_pm_mac_address = rec_pm_mac[index];
        }
        if (rec_pm_dnum[lowest] > rec_pm_dnum[index])
          lowest = index;
      }
      if (!selected_pm_still_alive && saved_pm_mac_address == ""){
        selected_pm_mac_address = rec_pm_mac[lowest];  //select the lower numbered PM if none has been selected before
        base = "2ms/PM/" + String(rec_pm_dnum[lowest]) + "/";
      }
    }
    if (current_num_pm == 0){
      selected_pm_mac_address = "0";
      base = "2ms/PM/0/";
    }
    number_of_pms_found = current_num_pm;   
    current_num_pm = 0;
    consumedTopic = base + consumed;
    generatedTopic = base + generated;
  }
}

void loop() {
  server.handleClient(); 
  //if (solar_evse == 1)
    Timer12 = millis();   // reset LED timeout as it won't be powered by a battery so no need to turn off LEDs
  if ((millis() - m_LastCheck >= 125) && (solar_evse == 0) && reset_sleep && 0) {   // Go into light sleep for solaradapter since it could be battery powered
    WiFi.setSleepMode(WIFI_LIGHT_SLEEP,2);                     // Automatic Light Sleep mode, DTIM listen interval = 2, higher DTIM interval is not recommended as establishing and maintaining a connection is difficult
    delay(350);
    m_LastCheck -= 350;  // increment counters that were paused during light sleep
    Timer12 -= 350;
    Timer6 -= 350;
    Timer3 -= 350;
    Timer4 -= 350;
    Timer_zero -= 350;
    Timer -= 350;
    Timer20 -= 350;
    reset_sleep = false;
  }
  ArduinoOTA.handle();        // initiates OTA update capability
  updateRedLED();
  updateYellowLED();
  firmwareUpdateCheck(0);
  updateSAStatus();
  updateFlag();
  checkTimer();

  if (WiFi.isConnected()){
    if (!no_mqtt){
// Section A. The following section is for safety in case the energy data is no longer updated for 300 secs basically turn_off autotracking and autotracking scheduling and no longer using power
      if (consumed_data_status == 1){
         no_consumed_data_received = false;
         consumed_data_status = 0;
         if (previously_auto_mode & 0x01){        // turn auto tracking on  bit 0 is for autotracking flag, bit 1 is for schedule autotrakcing flag
            g_EvseController.SetAutomaticMode(1,1);
            previously_auto_mode = previously_auto_mode & 0x02;
         }
         if (previously_auto_mode & 0x02) {       //schedule auto tracking
            schedule_AT = 1;
            previously_auto_mode = previously_auto_mode & 0x01;
         }
         Timer11 = millis();
      }
      if (consumed_data_status == 0 && ((millis() - Timer11) >= 300000)){  // if topic for power goes away or never started
         Timer11 = millis();
         no_consumed_data_received = true;
         client.disconnect();
         if (g_EvseController.IsAutomaticMode()){   //turn off auto tracking and set flag
            previously_auto_mode = previously_auto_mode | 0x01;        
            g_EvseController.SetAutomaticMode(0,1);
            g_EvseController.Sleep();
         }
         if (schedule_AT == 1) {             // turn off scheduled auto tracking and set flag
            schedule_AT = 0;
            previously_auto_mode = previously_auto_mode | 0x02;
         }
      }
 // Section A ends
          
      if (!server2_down){
        checkPM();
        processMQTTData();
      }
      automaticTrackingUpdate();
      if (!server2_down){
        publishData();
        publishDone();
        publishResetPriority();
      }
    }
  }
  if (!no_mqtt && ((connection_mode == WIFI) || (connection_mode == WIFI_MQTT))) 
    checkBrokerConnection();
  g_EvseController.Update();
}

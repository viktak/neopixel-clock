#define __debugSettings
#include "includes.h"

//  Web server
ESP8266WebServer server(80);

//  Initialize Wifi
WiFiClient wclient;
PubSubClient PSclient(wclient);

//  Timers and their flags
os_timer_t heartbeatTimer;
os_timer_t accessPointTimer;

//  Flags
bool needsHeartbeat = false;

//  NeoPixel
NeoGamma<NeoGammaTableMethod> colorGamma;
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(NUMBER_OF_LEDS);
RGBPixelData buf[NUMBER_OF_LEDS];

//  Other global variables
config appConfig;
bool isAccessPoint = false;
bool isAccessPointCreated = false;
uint8_t oldSecs = -1;

TimeChangeRule *tcr;        // Pointer to the time change rule

char16_t buttonMillis;

bool ntpInitialized = false;
enum CONNECTION_STATE connectionState;

WiFiUDP Udp;

// Daylight savings time rules for Greece
TimeChangeRule myDST = {"MDT", Fourth, Sun, Mar, 2, DST_TIMEZONE_OFFSET * 60};
TimeChangeRule mySTD = {"MST", Fourth,  Sun, Oct, 2,  ST_TIMEZONE_OFFSET * 60};
Timezone myTZ(myDST, mySTD);

void LogEvent(int Category, int ID, String Title, String Data){
  if (PSclient.connected()){

    String msg = "{";

    msg += "\"Node\":" + (String)ESP.getChipId() + ",";
    msg += "\"Category\":" + (String)Category + ",";
    msg += "\"ID\":" + (String)ID + ",";
    msg += "\"Title\":\"" + Title + "\",";
    msg += "\"Data\":\"" + Data + "\"}";

    Serial.println(msg);

    PSclient.publish((MQTT_CUSTOMER + String("/") + MQTT_PROJECT + String("/") + appConfig.mqttTopic + "/log").c_str(), msg.c_str(), false);
  }
}

void SetRandomSeed(){
    uint32_t seed;

    // random works best with a seed that can use 31 bits
    // analogRead on a unconnected pin tends toward less than four bits
    seed = analogRead(0);
    delay(1);

    for (int shifts = 3; shifts < 31; shifts += 3)
    {
        seed ^= analogRead(0) << shifts;
        delay(1);
    }

    randomSeed(seed);
}

void accessPointTimerCallback(void *pArg) {
  ESP.reset();
}

void heartbeatTimerCallback(void *pArg) {
  needsHeartbeat = true;
}

bool loadSettings(config& data) {
  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
    LogEvent(EVENTCATEGORIES::System, 1, "FS failure", "Failed to open config file.");
    return false;
  }

  size_t size = configFile.size();
  if (size > 1024) {
    Serial.println("Config file size is too large");
    LogEvent(EVENTCATEGORIES::System, 2, "FS failure", "Config file size is too large.");
    return false;
  }

  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);

  // We don't use String here because ArduinoJson library requires the input
  // buffer to be mutable. If you don't use ArduinoJson, you may as well
  // use configFile.readString instead.
  configFile.readBytes(buf.get(), size);
  configFile.close();

  StaticJsonDocument<JSON_SETTINGS_SIZE> doc;
  DeserializationError error = deserializeJson(doc, buf.get());

  if (error) {
    Serial.println("Failed to parse config file");
    LogEvent(EVENTCATEGORIES::System, 3, "FS failure", "Failed to parse config file.");
    Serial.println(error.c_str());
    return false;
  }

  #ifdef __debugSettings
  serializeJsonPretty(doc,Serial);
  Serial.println();
  #endif

  if (doc["ssid"]){
    strcpy(appConfig.ssid, doc["ssid"]);
  }
  else
  {
    strcpy(appConfig.ssid, defaultSSID);
  }
  
  if (doc["password"]){
    strcpy(appConfig.password, doc["password"]);
  }
  else
  {
    strcpy(appConfig.password, DEFAULT_PASSWORD);
  }
  
  if (doc["mqttServer"]){
    strcpy(appConfig.mqttServer, doc["mqttServer"]);
  }
  else
  {
    strcpy(appConfig.mqttServer, DEFAULT_MQTT_SERVER);
  }
  
  if (doc["mqttPort"]){
    appConfig.mqttPort = doc["mqttPort"];
  }
  else
  {
    appConfig.mqttPort = DEFAULT_MQTT_PORT;
  }
  
  if (doc["mqttTopic"]){
    strcpy(appConfig.mqttTopic, doc["mqttTopic"]);
  }
  else
  {
    sprintf(defaultSSID, "%s-%u", DEFAULT_MQTT_TOPIC, ESP.getChipId());
    strcpy(appConfig.mqttTopic, defaultSSID);
  }
  
  if (doc["friendlyName"]){
    strcpy(appConfig.friendlyName, doc["friendlyName"]);
  }
  else
  {
    strcpy(appConfig.friendlyName, NODE_DEFAULT_FRIENDLY_NAME);
  }
  
  if (doc["timezone"]){
    appConfig.timeZone = doc["timezone"];
  }
  else
  {
    appConfig.timeZone = 0;
  }
  
  if (doc["heartbeatInterval"]){
    appConfig.heartbeatInterval = doc["heartbeatInterval"];
  }
  else
  {
    appConfig.heartbeatInterval = DEFAULT_HEARTBEAT_INTERVAL;
  }
  
  if (doc["trailLength"]){
    appConfig.trailLength = doc["trailLength"];
  }

  if ( doc.containsKey("reverseClockDirection"))
    appConfig.reverseClockDirection = doc["reverseClockDirection"];

  if (doc.containsKey("showFiveMinuteMarks"))
    appConfig.showFiveMinuteMarks = doc["showFiveMinuteMarks"];
    
  if (doc.containsKey("showSeconds"))
    appConfig.showSeconds = doc["showSeconds"];
    

  return true;
}

bool saveSettings() {
  StaticJsonDocument<1024> doc;

  doc["ssid"] = appConfig.ssid;
  doc["password"] = appConfig.password;

  doc["heartbeatInterval"] = appConfig.heartbeatInterval;

  doc["timezone"] = appConfig.timeZone;

  doc["mqttServer"] = appConfig.mqttServer;
  doc["mqttPort"] = appConfig.mqttPort;
  doc["mqttTopic"] = appConfig.mqttTopic;

  doc["friendlyName"] = appConfig.friendlyName;

  doc["trailLength"] = appConfig.trailLength;
  doc["reverseClockDirection"] = appConfig.reverseClockDirection;
  doc["showFiveMinuteMarks"] = appConfig.showFiveMinuteMarks;
  doc["showSeconds"] = appConfig.showSeconds;

  #ifdef __debugSettings
  serializeJsonPretty(doc,Serial);
  Serial.println();
  #endif

  File configFile = LittleFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    LogEvent(System, 4, "FS failure", "Failed to open config file for writing.");
    return false;
  }
  serializeJson(doc, configFile);
  configFile.close();

  return true;
}

void defaultSettings(){
  #ifdef __debugSettings
  strcpy(appConfig.ssid, DEBUG_WIFI_SSID);
  strcpy(appConfig.password, DEBUG_WIFI_PASSWORD);
  strcpy(appConfig.mqttServer, DEBUG_MQTT_SERVER);
  #else
  strcpy(appConfig.ssid, "ESP");
  strcpy(appConfig.password, "password");
  strcpy(appConfig.mqttServer, "test.mosquitto.org");
  #endif

  appConfig.mqttPort = DEFAULT_MQTT_PORT;

  sprintf(defaultSSID, "%s-%u", DEFAULT_MQTT_TOPIC, ESP.getChipId());
  strcpy(appConfig.mqttTopic, defaultSSID);

  appConfig.timeZone = 2;

  strcpy(appConfig.friendlyName, NODE_DEFAULT_FRIENDLY_NAME);
  appConfig.heartbeatInterval = DEFAULT_HEARTBEAT_INTERVAL;

  appConfig.trailLength = DEFAULT_TRAIL_LENGTH;
  appConfig.reverseClockDirection = DEFAULT_REVERSE_CLOCK_DIRECTION;
  appConfig.showFiveMinuteMarks = DEFAULT_SHOW_FIVE_MINUTE_MARKS;
  appConfig.showSeconds = DEFAULT_SHOW_SECONDS;

  if (!saveSettings()) {
    Serial.println("Failed to save config");
  } else {
    Serial.println("Config saved");
  }
}

String DateTimeToString(time_t time){

  String myTime = "";
  char s[2];

  //  years
  itoa(year(time), s, DEC);
  myTime+= s;
  myTime+="-";


  //  months
  itoa(month(time), s, DEC);
  myTime+= s;
  myTime+="-";

  //  days
  itoa(day(time), s, DEC);
  myTime+= s;

  myTime+=" ";

  //  hours
  itoa(hour(time), s, DEC);
  myTime+= s;
  myTime+=":";

  //  minutes
  if(minute(time) <10)
    myTime+="0";

  itoa(minute(time), s, DEC);
  myTime+= s;
  myTime+=":";

  //  seconds
  if(second(time) <10)
    myTime+="0";

  itoa(second(time), s, DEC);
  myTime+= s;

  return myTime;
}

String TimeIntervalToString(time_t time){

  String myTime = "";
  char s[2];

  //  hours
  itoa((time/3600), s, DEC);
  myTime+= s;
  myTime+=":";

  //  minutes
  if(minute(time) <10)
    myTime+="0";

  itoa(minute(time), s, DEC);
  myTime+= s;
  myTime+=":";

  //  seconds
  if(second(time) <10)
    myTime+="0";

  itoa(second(time), s, DEC);
  myTime+= s;
  return myTime;
}

bool is_authenticated(){
  #ifdef __debugSettings
  return true;
  #endif
  if (server.hasHeader("Cookie")){
    String cookie = server.header("Cookie");
    if (cookie.indexOf("EspAuth=1") != -1) {
      LogEvent(EVENTCATEGORIES::Authentication, 1, "Success", "");
      return true;
    }
  }
  LogEvent(EVENTCATEGORIES::Authentication, 2, "Failure", "");
  return false;
}

void handleLogin(){
  String msg = "";
  if (server.hasHeader("Cookie")){
    String cookie = server.header("Cookie");
  }
  if (server.hasArg("DISCONNECT")){
    String header = "HTTP/1.1 301 OK\r\nSet-Cookie: EspAuth=0\r\nLocation: /login.html\r\nCache-Control: no-cache\r\n\r\n";
    server.sendContent(header);
    LogEvent(EVENTCATEGORIES::Login, 1, "Logout", "");
    return;
  }
  if (server.hasArg("username") && server.hasArg("password")){
    if (server.arg("username") == ADMIN_USERNAME &&  server.arg("password") == ADMIN_PASSWORD ){
      String header = "HTTP/1.1 301 OK\r\nSet-Cookie: EspAuth=1\r\nLocation: /status.html\r\nCache-Control: no-cache\r\n\r\n";
      server.sendContent(header);
      LogEvent(EVENTCATEGORIES::Login, 2, "Success", "User name: " + server.arg("username"));
      return;
    }
    msg = "<div class=\"alert alert-danger\"><strong>Error!</strong> Wrong user name and/or password specified.<a href=\"#\" class=\"close\" data-dismiss=\"alert\" aria-label=\"close\">&times;</a></div>";
    LogEvent(EVENTCATEGORIES::Login, 2, "Failure", "User name: " + server.arg("username") + " - Password: " + server.arg("password"));
  }

  File f = LittleFS.open("/pageheader.html", "r");
  String headerString;
  if (f.available()) headerString = f.readString();
  f.close();

  time_t localTime = myTZ.toLocal(now(), &tcr);

  f = LittleFS.open("/login.html", "r");

  String s, htmlString;

  while (f.available()){
    s = f.readStringUntil('\n');

    if (s.indexOf("%pageheader%")>-1) s.replace("%pageheader%", headerString);
    if (s.indexOf("%year%")>-1) s.replace("%year%", (String)year(localTime));
    if (s.indexOf("%alert%")>-1) s.replace("%alert%", msg);

    htmlString+=s;
  }
  f.close();
  server.send(200, "text/html", htmlString);
  LogEvent(PageHandler, 2, "Page served", "/");
}

void handleRoot() {
  LogEvent(EVENTCATEGORIES::PageHandler, 1, "Page requested", "/");

  if (!is_authenticated()){
    String header = "HTTP/1.1 301 OK\r\nLocation: /login.html\r\nCache-Control: no-cache\r\n\r\n";
    server.sendContent(header);
    return;
  }

  File f = LittleFS.open("/pageheader.html", "r");
  String headerString;
  if (f.available()) headerString = f.readString();
  f.close();

  time_t localTime = myTZ.toLocal(now(), &tcr);

  f = LittleFS.open("/index.html", "r");

  String FirmwareVersionString = String(FIRMWARE_VERSION) + " @ " + String(__TIME__) + " - " + String(__DATE__);

  String s, htmlString;

  while (f.available()){
    s = f.readStringUntil('\n');

    if (s.indexOf("%pageheader%")>-1) s.replace("%pageheader%", headerString);
    if (s.indexOf("%year%")>-1) s.replace("%year%", (String)year(localTime));
    if (s.indexOf("%espid%")>-1) s.replace("%espid%", (String)ESP.getChipId());
    if (s.indexOf("%hardwareid%")>-1) s.replace("%hardwareid%", HARDWARE_ID);
    if (s.indexOf("%hardwareversion%")>-1) s.replace("%hardwareversion%", HARDWARE_VERSION);
    if (s.indexOf("%softwareid%")>-1) s.replace("%softwareid%", SOFTWARE_ID);
    if (s.indexOf("%firmwareversion%")>-1) s.replace("%firmwareversion%", FirmwareVersionString);

    htmlString+=s;
  }
  f.close();
  server.send(200, "text/html", htmlString);
  LogEvent(EVENTCATEGORIES::PageHandler, 2, "Page served", "/");
}

void handleStatus() {

  LogEvent(EVENTCATEGORIES::PageHandler, 1, "Page requested", "status.html");
  if (!is_authenticated()){
     String header = "HTTP/1.1 301 OK\r\nLocation: /login.html\r\nCache-Control: no-cache\r\n\r\n";
     server.sendContent(header);
     return;
  }

  File f = LittleFS.open("/pageheader.html", "r");
  String headerString;
  if (f.available()) headerString = f.readString();
  f.close();

  time_t localTime = myTZ.toLocal(now(), &tcr);
  
  String s;

  f = LittleFS.open("/status.html", "r");

  String htmlString, ds18b20list;

  while (f.available()){
    s = f.readStringUntil('\n');
 
    //  System information
    if (s.indexOf("%pageheader%")>-1) s.replace("%pageheader%", headerString);
    if (s.indexOf("%year%")>-1) s.replace("%year%", (String)year(localTime));
    if (s.indexOf("%chipid%")>-1) s.replace("%chipid%", (String)ESP.getChipId());
    if (s.indexOf("%uptime%")>-1) s.replace("%uptime%", TimeIntervalToString(millis()/1000));
    if (s.indexOf("%currenttime%")>-1) s.replace("%currenttime%", DateTimeToString(localTime));
    if (s.indexOf("%lastresetreason%")>-1) s.replace("%lastresetreason%", ESP.getResetReason());
    if (s.indexOf("%flashchipsize%")>-1) s.replace("%flashchipsize%",String(ESP.getFlashChipSize()));
    if (s.indexOf("%flashchipspeed%")>-1) s.replace("%flashchipspeed%",String(ESP.getFlashChipSpeed()));
    if (s.indexOf("%freeheapsize%")>-1) s.replace("%freeheapsize%",String(ESP.getFreeHeap()));
    if (s.indexOf("%freesketchspace%")>-1) s.replace("%freesketchspace%",String(ESP.getFreeSketchSpace()));
    if (s.indexOf("%friendlyname%")>-1) s.replace("%friendlyname%",appConfig.friendlyName);
    if (s.indexOf("%mqtt-topic%")>-1) s.replace("%mqtt-topic%",appConfig.mqttTopic);

    //  Network settings
    switch (WiFi.getMode()) {
      case WIFI_AP:
        if (s.indexOf("%wifimode%")>-1) s.replace("%wifimode%", "Access Point");
        if (s.indexOf("%macaddress%")>-1) s.replace("%macaddress%",String(WiFi.softAPmacAddress()));
        if (s.indexOf("%networkaddress%")>-1) s.replace("%networkaddress%",WiFi.softAPIP().toString());
        if (s.indexOf("%ssid%")>-1) s.replace("%ssid%",String(WiFi.SSID()));
        if (s.indexOf("%subnetmask%")>-1) s.replace("%subnetmask%","n/a");
        if (s.indexOf("%gateway%")>-1) s.replace("%gateway%","n/a");
        break;
      case WIFI_STA:
        if (s.indexOf("%wifimode%")>-1) s.replace("%wifimode%", "Station");
        if (s.indexOf("%macaddress%")>-1) s.replace("%macaddress%",String(WiFi.macAddress()));
        if (s.indexOf("%networkaddress%")>-1) s.replace("%networkaddress%",WiFi.localIP().toString());
        if (s.indexOf("%ssid%")>-1) s.replace("%ssid%",String(WiFi.SSID()));
        if (s.indexOf("%subnetmask%")>-1) s.replace("%subnetmask%",WiFi.subnetMask().toString());
        if (s.indexOf("%gateway%")>-1) s.replace("%gateway%",WiFi.gatewayIP().toString());
        break;
      default:
        //  This should not happen...
        break;
    }

      htmlString+=s;
    }
    f.close();
  server.send(200, "text/html", htmlString);
  LogEvent(EVENTCATEGORIES::PageHandler, 2, "Page served", "status.html");
}

void handleGeneralSettings() {
  LogEvent(EVENTCATEGORIES::PageHandler, 1, "Page requested", "generalsettings.html");

  if (!is_authenticated()){
     String header = "HTTP/1.1 301 OK\r\nLocation: /login.html\r\nCache-Control: no-cache\r\n\r\n";
     server.sendContent(header);
     return;
   }

  if (server.method() == HTTP_POST){  //  POST
    bool mqttDirty = false;

    if (server.hasArg("timezoneselector")){
      signed char oldTimeZone = appConfig.timeZone;
      appConfig.timeZone = atoi(server.arg("timezoneselector").c_str());

      adjustTime((appConfig.timeZone - oldTimeZone) * SECS_PER_HOUR);

      LogEvent(EVENTCATEGORIES::TimeZoneChange, 1, "New time zone", "UTC " + server.arg("timezoneselector"));
    }

    if (server.hasArg("friendlyname")){
      strcpy(appConfig.friendlyName, server.arg("friendlyname").c_str());
      LogEvent(EVENTCATEGORIES::FriendlyNameChange, 1, "New friendly name", appConfig.friendlyName);
    }

    if (server.hasArg("heartbeatinterval")){
      os_timer_disarm(&heartbeatTimer);
      appConfig.heartbeatInterval = server.arg("heartbeatinterval").toInt();
      LogEvent(EVENTCATEGORIES::HeartbeatIntervalChange, 1, "New Heartbeat interval", (String)appConfig.heartbeatInterval);
      os_timer_arm(&heartbeatTimer, appConfig.heartbeatInterval * 1000, true);
    }

    //  MQTT settings
    if (server.hasArg("mqttbroker")){
      if ((String)appConfig.mqttServer != server.arg("mqttbroker"))
        mqttDirty = true;
        sprintf(appConfig.mqttServer, "%s", server.arg("mqttbroker").c_str());
        LogEvent(EVENTCATEGORIES::MqttParamChange, 1, "New MQTT broker", appConfig.mqttServer);
    }

    if (server.hasArg("mqttport")){
      if (appConfig.mqttPort != atoi(server.arg("mqttport").c_str()))
        mqttDirty = true;
      appConfig.mqttPort = atoi(server.arg("mqttport").c_str());
      LogEvent(EVENTCATEGORIES::MqttParamChange, 2, "New MQTT port", server.arg("mqttport").c_str());
    }

    if (server.hasArg("mqtttopic")){
      if ((String)appConfig.mqttTopic != server.arg("mqtttopic"))
        mqttDirty = true;
        sprintf(appConfig.mqttTopic, "%s", server.arg("mqtttopic").c_str());
        LogEvent(EVENTCATEGORIES::MqttParamChange, 1, "New MQTT topic", appConfig.mqttTopic);
    }

    if (mqttDirty)
      PSclient.disconnect();

    saveSettings();
    ESP.reset();

  }

  File f = LittleFS.open("/pageheader.html", "r");
  String headerString;
  if (f.available()) headerString = f.readString();
  f.close();

  time_t localTime = myTZ.toLocal(now(), &tcr);

  f = LittleFS.open("/generalsettings.html", "r");

  String s, htmlString, timezoneslist, traillist, chkreverse;

  char ss[2];

  for (signed char i = -12; i < 15; i++) {
    itoa(i, ss, DEC);
    timezoneslist+="<option ";
    if (appConfig.timeZone == i){
      timezoneslist+= "selected ";
    }
    timezoneslist+= "value=\"";
    timezoneslist+=ss;
    timezoneslist+="\">UTC ";
    if (i>0){
      timezoneslist+="+";
    }
    if (i!=0){
      timezoneslist+=ss;
      timezoneslist+=":00";
    }
    timezoneslist+="</option>";
    timezoneslist+="\n";
  }

  while (f.available()){
    s = f.readStringUntil('\n');

    if (s.indexOf("%pageheader%")>-1) s.replace("%pageheader%", headerString);
    if (s.indexOf("%year%")>-1) s.replace("%year%", (String)year(localTime));
    if (s.indexOf("%mqtt-servername%")>-1) s.replace("%mqtt-servername%", appConfig.mqttServer);
    if (s.indexOf("%mqtt-port%")>-1) s.replace("%mqtt-port%", String(appConfig.mqttPort));
    if (s.indexOf("%mqtt-topic%")>-1) s.replace("%mqtt-topic%", appConfig.mqttTopic);
    if (s.indexOf("%timezoneslist%")>-1) s.replace("%timezoneslist%", timezoneslist);
    if (s.indexOf("%friendlyname%")>-1) s.replace("%friendlyname%", appConfig.friendlyName);
    if (s.indexOf("%heartbeatinterval%")>-1) s.replace("%heartbeatinterval%", (String)appConfig.heartbeatInterval);

    if (s.indexOf("%traillist%")>-1) s.replace("%traillist%", traillist);
    if (s.indexOf("%chkreverse%")>-1) s.replace("%chkreverse%", chkreverse);

    htmlString+=s;
  }
  f.close();
  server.send(200, "text/html", htmlString);

  LogEvent(EVENTCATEGORIES::PageHandler, 2, "Page served", "generalsettings.html");
}

void handleNetworkSettings() {
  LogEvent(EVENTCATEGORIES::PageHandler, 1, "Page requested", "networksettings.html");

  if (!is_authenticated()){
     String header = "HTTP/1.1 301 OK\r\nLocation: /login.html\r\nCache-Control: no-cache\r\n\r\n";
     server.sendContent(header);
     return;
   }

  if (server.method() == HTTP_POST){  //  POST
    if (server.hasArg("ssid")){
      strcpy(appConfig.ssid, server.arg("ssid").c_str());
      strcpy(appConfig.password, server.arg("password").c_str());
      saveSettings();

      isAccessPoint=false;
      connectionState = STATE_CHECK_WIFI_CONNECTION;
      WiFi.disconnect(false);

      ESP.reset();
    }
  }

  File f = LittleFS.open("/pageheader.html", "r");

  String headerString;

  if (f.available()) headerString = f.readString();
  f.close();

  time_t localTime = myTZ.toLocal(now(), &tcr);

  f = LittleFS.open("/networksettings.html", "r");
  String s, htmlString, wifiList;

  byte numberOfNetworks = WiFi.scanNetworks();
  for (size_t i = 0; i < numberOfNetworks; i++) {
    wifiList+="<div class=\"radio\"><label><input ";
    if (i==0) wifiList+="id=\"ssid\" ";

    wifiList+="type=\"radio\" name=\"ssid\" value=\"" + WiFi.SSID(i) + "\">" + WiFi.SSID(i) + "</label></div>";
  }

  while (f.available()){
    s = f.readStringUntil('\n');

    if (s.indexOf("%pageheader%")>-1) s.replace("%pageheader%", headerString);
    if (s.indexOf("%year%")>-1) s.replace("%year%", (String)year(localTime));
    if (s.indexOf("%wifilist%")>-1) s.replace("%wifilist%", wifiList);
      htmlString+=s;
    }
    f.close();
  server.send(200, "text/html", htmlString);

  LogEvent(EVENTCATEGORIES::PageHandler, 2, "Page served", "networksettings.html");
}

void handleClock() {
  LogEvent(EVENTCATEGORIES::PageHandler, 1, "Page requested", "clock.html");

  if (!is_authenticated()){
     String header = "HTTP/1.1 301 OK\r\nLocation: /login.html\r\nCache-Control: no-cache\r\n\r\n";
     server.sendContent(header);
     return;
   }

  if (server.method() == HTTP_POST){  //  POST

    if (server.hasArg("trailselector")){
      appConfig.trailLength = server.arg("trailselector").toInt();
      LogEvent(EVENTCATEGORIES::Clock, 1, "New trail length", (String)appConfig.trailLength);
    }

    if (server.hasArg("reverse")){
      appConfig.reverseClockDirection = true;
      LogEvent(EVENTCATEGORIES::Clock, 1, "Reverse direction", "true");
    }
    else{
      appConfig.reverseClockDirection = false;
      LogEvent(EVENTCATEGORIES::Clock, 2, "Reverse direction", "false");
    }

    if (server.hasArg("showfivemark")){
      appConfig.showFiveMinuteMarks = true;
      LogEvent(EVENTCATEGORIES::Clock, 3, "Show five minute marks", "true");
    }
    else{
      appConfig.showFiveMinuteMarks = false;
      LogEvent(EVENTCATEGORIES::Clock, 4, "Show five minute marks", "false");
    }

    if (server.hasArg("showseconds")){
      appConfig.showSeconds = true;
      LogEvent(EVENTCATEGORIES::Clock, 5, "Show seconds", "true");
    }
    else{
      appConfig.showSeconds = false;
      LogEvent(EVENTCATEGORIES::Clock, 6, "Show seconds", "false");
    }

    saveSettings();

  }

  File f = LittleFS.open("/pageheader.html", "r");
  String headerString;
  if (f.available()) headerString = f.readString();
  f.close();

  time_t localTime = myTZ.toLocal(now(), &tcr);

  f = LittleFS.open("/clock.html", "r");

  String s, htmlString, traillist, chkreverse, chkfiveminute, chkshowseconds;

  char sss[2];

  for (size_t i = 1; i < 10; i++){
    itoa(i, sss, DEC);
    traillist+="<option ";
    if (appConfig.trailLength == i){
      traillist+= "selected ";
    }
    traillist+= "value=\"";
    traillist+=sss;
    traillist+="\">";
    traillist+=sss;
    traillist+="</option>";
    traillist+="\n";
  }

  chkreverse = "<input type=""checkbox"" name=""reverse"" id=""reverse""  ";
  if (appConfig.reverseClockDirection) chkreverse+="checked=";
  chkreverse+= "/>";

  chkfiveminute = "<input type=""checkbox"" name=""showfivemark"" id=""showfivemark""  ";
  if (appConfig.showFiveMinuteMarks) chkfiveminute+="checked=";
  chkfiveminute+= "/>";

  chkshowseconds = "<input type=""checkbox"" name=""showseconds"" id=""showseconds""  ";
  if (appConfig.showSeconds) chkshowseconds+="checked=";
  chkshowseconds+= "/>";

  while (f.available()){
    s = f.readStringUntil('\n');


    if (s.indexOf("%year%")>-1) s.replace("%year%", (String)year(localTime));
    if (s.indexOf("%pageheader%")>-1) s.replace("%pageheader%", headerString);

    if (s.indexOf("%traillist%")>-1) s.replace("%traillist%", traillist);
    if (s.indexOf("%chkreverse%")>-1) s.replace("%chkreverse%", chkreverse);
    if (s.indexOf("%chkfiveminute%")>-1) s.replace("%chkfiveminute%", chkfiveminute);
    if (s.indexOf("%chkshowseconds%")>-1) s.replace("%chkshowseconds%", chkshowseconds);

    htmlString+=s;
  }
  f.close();
  server.send(200, "text/html", htmlString);

  LogEvent(EVENTCATEGORIES::PageHandler, 2, "Page served", "clock.html");
}

void handleTools() {
  LogEvent(EVENTCATEGORIES::PageHandler, 1, "Page requested", "tools.html");

  if (!is_authenticated()){
     String header = "HTTP/1.1 301 OK\r\nLocation: /login.html\r\nCache-Control: no-cache\r\n\r\n";
     server.sendContent(header);
     return;
   }

  if (server.method() == HTTP_POST){  //  POST

    if (server.hasArg("reset")){
      LogEvent(EVENTCATEGORIES::Reboot, 1, "Reset", "");
      defaultSettings();
      ESP.reset();
    }

    if (server.hasArg("restart")){
      LogEvent(EVENTCATEGORIES::Reboot, 2, "Restart", "");
      ESP.reset();
    }
  }

  File f = LittleFS.open("/pageheader.html", "r");
  String headerString;
  if (f.available()) headerString = f.readString();
  f.close();

  time_t localTime = myTZ.toLocal(now(), &tcr);

  f = LittleFS.open("/tools.html", "r");

  String s, htmlString;

  while (f.available()){
    s = f.readStringUntil('\n');

    if (s.indexOf("%pageheader%")>-1) s.replace("%pageheader%", headerString);
    if (s.indexOf("%year%")>-1) s.replace("%year%", (String)year(localTime));

      htmlString+=s;
    }
    f.close();
  server.send(200, "text/html", htmlString);

  LogEvent(EVENTCATEGORIES::PageHandler, 2, "Page served", "tools.html");
}

/*
    for (size_t i = 0; i < server.args(); i++) {
      Serial.print(server.argName(i));
      Serial.print(": ");
      Serial.println(server.arg(i));
    }
*/

void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void SendHeartbeat(){

  if (PSclient.connected()){

    time_t localTime = myTZ.toLocal(now(), &tcr);

    const size_t capacity = JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(6) + 180;
    StaticJsonDocument<capacity> doc;

    doc["Time"] = DateTimeToString(localTime);
    doc["Node"] = ESP.getChipId();
    doc["Freeheap"] = ESP.getFreeHeap();
    doc["FriendlyName"] = appConfig.friendlyName;
    doc["HeartbeatInterval"] = appConfig.heartbeatInterval;

    JsonObject wifiDetails = doc.createNestedObject("Wifi");
    wifiDetails["SSId"] = String(WiFi.SSID());
    wifiDetails["MACAddress"] = String(WiFi.macAddress());
    wifiDetails["IPAddress"] = WiFi.localIP().toString();

    #ifdef __debugSettings
    serializeJsonPretty(doc,Serial);
    Serial.println();
    #endif

    String myJsonString;

    serializeJson(doc, myJsonString);

    PSclient.publish((MQTT_CUSTOMER + String("/") + MQTT_PROJECT + "/" + appConfig.mqttTopic + "/HEARTBEAT").c_str(), myJsonString.c_str(), false);
  }

  needsHeartbeat = false;
}

void DisplayTime(){
  
  //  Clear pixels
  for (size_t i = 0; i < NUMBER_OF_LEDS; i++){
    buf[i].Red = 0;
    buf[i].Green = 0;
    buf[i].Blue = 0;
  }

  //  Calculate time
  time_t localTime = myTZ.toLocal(now(), &tcr);

 
   //  Display time markers
  int8_t myHours = hourFormat12(localTime);
  int8_t myMinutes = minute(localTime);
  int8_t mySeconds = second(localTime);

  // myHours = hh;
  // myMinutes = mm;
  // mySeconds = ss;

  myHours*=5;

  //  Display time and trail
  int8_t diff = (LED_MAX_BRIGHTNESS - LAST_OF_TRAIL_BRIGHTNESS) / ( appConfig.trailLength - 1);

  for (int8_t i = 0; i < appConfig.trailLength; i++){
    int8_t offset = 0;

    if ( myHours - i < 0 ) offset = NUMBER_OF_LEDS; else offset = 0;
    buf[(myHours   - i + offset>59?0:myHours   - i + offset)].Blue  = LED_MAX_BRIGHTNESS - diff * i;

    //Serial.printf("i: %i\thours: %i\tposition: %i\t\r\n", i, myHours, myHours - i + offset);

    if ( myMinutes - i < 0 ) offset = NUMBER_OF_LEDS; else offset = 0;
    buf[myMinutes - i + offset].Green = LED_MAX_BRIGHTNESS - diff * i;

    //Serial.printf("i: %i\tminutes: %i\tposition: %i\t\r\n", i, myMinutes, myMinutes - i + offset);

    if ( appConfig.showSeconds ){
      if ( mySeconds - i < 0 ) offset = NUMBER_OF_LEDS; else offset = 0;
      buf[mySeconds - i + offset].Red   = LED_MAX_BRIGHTNESS - diff * i;
    }
    //Serial.printf("i: %i\tseconds: %i\tposition: %i\t\r\n", i, mySeconds, mySeconds - i + offset);
    //Serial.println();
  }

  //Serial.println();

  //  Display 5 minute marks
  if ( appConfig.showFiveMinuteMarks ){
    for (size_t i = 0; i < 12; i++){

      bool isRedLit = buf[i*5].Red;
      bool isGreenLit = buf[i*5].Green;
      bool isBlueLit = buf[i*5].Blue;

      buf[i*5].Red = isRedLit?buf[i*5].Red:FIVE_MINUTE_MARK_BRIGHTNESS;
      buf[i*5].Green = isGreenLit?buf[i*5].Green:FIVE_MINUTE_MARK_BRIGHTNESS;
      buf[i*5].Blue = isBlueLit?buf[i*5].Blue:FIVE_MINUTE_MARK_BRIGHTNESS;
    }
  }

  //  If clock is reversed, it needs its pixels shifted by one,
  //  so that the 12 hour mark stays put.
  if (appConfig.reverseClockDirection){
    RGBPixelData pd;
    pd.Red = buf[0].Red;
    pd.Green = buf[0].Green;
    pd.Blue = buf[0].Blue;
    // //Serial.printf("%i\t%i\t%i\r\n", pd.Red, pd.Green, pd.Blue);
    for (size_t i = 1; i < NUMBER_OF_LEDS; i++)
    {
      buf[i-1].Red = buf[i].Red;
      buf[i-1].Green = buf[i].Green;
      buf[i-1].Blue = buf[i].Blue;
    }
    buf[NUMBER_OF_LEDS - 1].Red = pd.Red;
    buf[NUMBER_OF_LEDS - 1].Green = pd.Green;
    buf[NUMBER_OF_LEDS - 1].Blue = pd.Blue;
  }

  //  Move buffer to strip
  for (size_t i = 0; i < NUMBER_OF_LEDS; i++){
    if (appConfig.reverseClockDirection)
      strip.SetPixelColor(NUMBER_OF_LEDS - i - 1, colorGamma.Correct(RgbColor(buf[i].Red, buf[i].Green, buf[i].Blue)));
      else
      strip.SetPixelColor(i, colorGamma.Correct(RgbColor(buf[i].Red, buf[i].Green, buf[i].Blue)));
  }
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {

  Serial.print("Topic:\t\t");
  Serial.println(topic);

  Serial.print("Payload:\t");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  StaticJsonDocument<JSON_MQTT_COMMAND_SIZE> doc;
  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    Serial.println("Failed to parse incoming string.");
    Serial.println(error.c_str());
    for (size_t i = 0; i < 10; i++) {
      digitalWrite(CONNECTION_STATUS_LED_GPIO, !digitalRead(CONNECTION_STATUS_LED_GPIO));
      delay(50);
    }
  }
  else{
    //  It IS a JSON string

    #ifdef __debugSettings
    serializeJsonPretty(doc,Serial);
    Serial.println();
    #endif


    if (doc.containsKey("dotest")){
      Serial.println("Testing LEDs...");

      for (size_t i = 0; i < NUMBER_OF_LEDS; i++)
      {
        strip.SetPixelColor(i, colorGamma.Correct(RgbColor(255, 0, 0)));
        strip.Show();
      }
      delay(3000);
      
      for (size_t i = 0; i < NUMBER_OF_LEDS; i++)
      {
        strip.SetPixelColor(i, colorGamma.Correct(RgbColor(0, 255, 0)));
        strip.Show();
      }
      delay(3000);
      
      for (size_t i = 0; i < NUMBER_OF_LEDS; i++)
      {
        strip.SetPixelColor(i, colorGamma.Correct(RgbColor(0, 0, 255)));
        strip.Show();
      }
      delay(3000);
      
      Serial.println("End of testing, resuming normal operation...");
    }

    //  reset
    if (doc.containsKey("reset")){
      LogEvent(EVENTCATEGORIES::MqttMsg, 1, "Reset", "");
      defaultSettings();
      ESP.reset();
    }

    //  restart
    if (doc.containsKey("restart")){
      LogEvent(EVENTCATEGORIES::MqttMsg, 2, "Restart", "");
      ESP.reset();
    }
  }

}

void setup() {
  delay(1); //  Needed for PlatformIO serial monitor
  Serial.begin(115200);
  Serial.setDebugOutput(false);
  Serial.print("\n\n\n\rBooting node:     ");
  Serial.print(ESP.getChipId());
  Serial.println("...");

  String FirmwareVersionString = String(FIRMWARE_VERSION) + " @ " + String(__TIME__) + " - " + String(__DATE__);

  Serial.println("Hardware ID:      " + (String)HARDWARE_ID);
  Serial.println("Hardware version: " + (String)HARDWARE_VERSION);
  Serial.println("Software ID:      " + (String)SOFTWARE_ID);
  Serial.println("Software version: " + FirmwareVersionString);
  Serial.println();

  //  File system
  if (!LittleFS.begin()){
    Serial.println("Error: Failed to initialize the filesystem!");
  }

  if (!loadSettings(appConfig)) {
    Serial.println("Failed to load config, creating default settings...");
    defaultSettings();
  } else {
    Serial.println("Config loaded.");
  }

  WiFi.hostname(defaultSSID);

  //  GPIOs
  //  outputs
  pinMode(CONNECTION_STATUS_LED_GPIO, OUTPUT);
  digitalWrite(CONNECTION_STATUS_LED_GPIO, HIGH);

  //  OTA
  ArduinoOTA.onStart([]() {
    Serial.println("OTA started.");
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA finished.");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    if (progress % OTA_BLINKING_RATE == 0){
      if (digitalRead(CONNECTION_STATUS_LED_GPIO)==HIGH)
        digitalWrite(CONNECTION_STATUS_LED_GPIO, LOW);
        else
        digitalWrite(CONNECTION_STATUS_LED_GPIO, HIGH);
    }
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Authentication failed.");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin failed.");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect failed.");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive failed.");
    else if (error == OTA_END_ERROR) Serial.println("End failed.");
  });

  ArduinoOTA.begin();

  Serial.println();

  server.on("/", handleRoot);
  server.on("/status.html", handleStatus);
  server.on("/generalsettings.html", handleGeneralSettings);
  server.on("/clock.html", handleClock);
  server.on("/networksettings.html", handleNetworkSettings);
  server.on("/tools.html", handleTools);
  server.on("/login.html", handleLogin);

  server.onNotFound(handleNotFound);

  //  Web server
  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started.");
  }

  //  Start HTTP (web) server
  server.begin();
  Serial.println("HTTP server started.");

  //  Authenticate HTTP requests
  const char * headerkeys[] = {"User-Agent","Cookie"} ;
  size_t headerkeyssize = sizeof(headerkeys)/sizeof(char*);
  server.collectHeaders(headerkeys, headerkeyssize );

  //  Timers
  os_timer_setfn(&heartbeatTimer, heartbeatTimerCallback, NULL);
  os_timer_arm(&heartbeatTimer, appConfig.heartbeatInterval * 1000, true);

  strip.Begin();
  strip.Show();

  //  Randomizer
  SetRandomSeed();

  // Set the initial connection state
  connectionState = STATE_CHECK_WIFI_CONNECTION;

}

void loop(){

  if (isAccessPoint){
    if (!isAccessPointCreated){
      Serial.print("Could not connect to ");
      Serial.print(appConfig.ssid);
      Serial.println("\r\nReverting to Access Point mode.");

      delay(500);

      WiFi.mode(WiFiMode::WIFI_AP);
      WiFi.softAP(defaultSSID, DEFAULT_PASSWORD);

      IPAddress myIP;
      myIP = WiFi.softAPIP();
      isAccessPointCreated = true;

      Serial.println("Access point created. Use the following information to connect to the ESP device, then follow the on-screen instructions to connect to a different wifi network:");

      Serial.print("SSID:\t\t\t");
      Serial.println(defaultSSID);

      Serial.print("Password:\t\t");
      Serial.println(DEFAULT_PASSWORD);

      Serial.print("Access point address:\t");
      Serial.println(myIP);

      Serial.println();
      Serial.println("Note: The device will reset in 5 minutes.");


      os_timer_setfn(&accessPointTimer, accessPointTimerCallback, NULL);
      os_timer_arm(&accessPointTimer, ACCESS_POINT_TIMEOUT, true);
      os_timer_disarm(&heartbeatTimer);
    }
    server.handleClient();
  }
  else{
    switch (connectionState) {

      // Check the WiFi connection
      case STATE_CHECK_WIFI_CONNECTION:

        // Are we connected ?
        if (WiFi.status() != WL_CONNECTED) {
          // Wifi is NOT connected
          digitalWrite(CONNECTION_STATUS_LED_GPIO, HIGH);
          connectionState = STATE_WIFI_CONNECT;
        } else  {
          // Wifi is connected so check Internet
          digitalWrite(CONNECTION_STATUS_LED_GPIO, LOW);
          connectionState = STATE_CHECK_INTERNET_CONNECTION;
          
          server.handleClient();
        }
        break;

      // No Wifi so attempt WiFi connection
      case STATE_WIFI_CONNECT:
        {
          // Indicate NTP no yet initialized
          ntpInitialized = false;

          digitalWrite(CONNECTION_STATUS_LED_GPIO, HIGH);
          Serial.printf("Trying to connect to WIFI network: %s", appConfig.ssid);

          // Set station mode
          WiFi.mode(WIFI_STA);

          // Start connection process
          WiFi.begin(appConfig.ssid, appConfig.password);

          // Initialize iteration counter
          uint8_t attempt = 0;

          while ((WiFi.status() != WL_CONNECTED) && (attempt++ < WIFI_CONNECTION_TIMEOUT)) {
            digitalWrite(CONNECTION_STATUS_LED_GPIO, LOW);
            Serial.print(".");
            delay(50);
            digitalWrite(CONNECTION_STATUS_LED_GPIO, HIGH);
            delay(950);
          }
          if (attempt >= WIFI_CONNECTION_TIMEOUT) {
            Serial.println();
            Serial.println("Could not connect to WiFi");
            delay(100);

            isAccessPoint=true;

            break;
          }
          digitalWrite(CONNECTION_STATUS_LED_GPIO, LOW);
          Serial.println(" Success!");
          Serial.print("IP address: ");
          Serial.println(WiFi.localIP());
          connectionState = STATE_CHECK_INTERNET_CONNECTION;
        }
        break;

      case STATE_CHECK_INTERNET_CONNECTION:

        // Do we have a connection to the Internet ?
        if (checkInternetConnection()) {
          // We have an Internet connection

          if (! ntpInitialized) {
            // We are connected to the Internet for the first time so set NTP provider
            initNTP();

            ntpInitialized = true;

            Serial.println("Connected to the Internet.");
          }

          connectionState = STATE_INTERNET_CONNECTED;
        } else  {
          connectionState = STATE_CHECK_WIFI_CONNECTION;
        }
        break;

      case STATE_INTERNET_CONNECTED:

        ArduinoOTA.handle();

        if (!PSclient.connected()) {
          PSclient.setServer(appConfig.mqttServer, appConfig.mqttPort);
          if (PSclient.connect(defaultSSID, (MQTT_CUSTOMER + String("/") + MQTT_PROJECT + String("/") + appConfig.mqttTopic + "/STATE").c_str(), 0, true, "offline" )){
            PSclient.setCallback(mqtt_callback);

            PSclient.subscribe((MQTT_CUSTOMER + String("/") + MQTT_PROJECT + String("/") + appConfig.mqttTopic + "/cmnd").c_str(), 0);

            PSclient.publish((MQTT_CUSTOMER + String("/") + MQTT_PROJECT + String("/") + appConfig.mqttTopic + "/STATE").c_str(), "online", true);
            LogEvent(EVENTCATEGORIES::Conn, 1, "Node online", WiFi.localIP().toString());
          }
        }

        if (PSclient.connected()){
          PSclient.loop();
        }

        if (needsHeartbeat){
          SendHeartbeat();
          needsHeartbeat = false;
        }

        uint8_t currentSecond = second(now());
        if ( oldSecs != currentSecond ){
          DisplayTime();
          oldSecs = currentSecond;
        }

        strip.Show();

        // Set next connection state
        connectionState = STATE_CHECK_WIFI_CONNECTION;
        break;
    }

  }
}

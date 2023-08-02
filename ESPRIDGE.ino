/*
   ESP Home Automation Bridge (c) invpe 2k23.
   Simple way to make your home smarter with Alexa
   This is based on the documentation from here: https://www.burgestrand.se/hue-api/api/lights/
*/
#include <HTTPClient.h>
#include <WiFi.h>
#include <WebServer.h>
#include "SPIFFS.h"
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <WiFiClient.h>
#include <vector>
//-----------------------//
#define VER "1.0 Quakefeller"
//-----------------------//
#define WIFI_A ""
#define WIFI_P ""
#define OTA_SUPPORT 1
//-----------------------//
WebServer Apache(80);
WiFiUDP udp;
//-----------------------//
const int SSDP_PORT = 1900;
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;
String strUID = "2fa00080-d000-11e1-9b23-001f80007bbe";
String strDescriptionXML = "<?xml version=\"1.0\"?>\
<root xmlns=\"urn:schemas-upnp-org:device-1-0\">\
  <specVersion>\
    <major>1</major>\
    <minor>0</minor>\
  </specVersion>\
  <URLBase>http://$IP$:80/</URLBase>\
  <device>\
    <deviceType>urn:schemas-upnp-org:device:Basic:1</deviceType>\
    <friendlyName>Philips hue ($IP$)</friendlyName>\
    <manufacturer>Royal Philips Electronics</manufacturer>\
    <manufacturerURL>http://www.philips.com</manufacturerURL>\
    <modelDescription>Philips hue Personal Wireless Lighting</modelDescription>\
    <modelName>Philips hue bridge 2012</modelName>\
    <modelNumber>1000000000000</modelNumber>\
    <modelURL>http://www.meethue.com</modelURL>\
    <serialNumber>93eadbeef13</serialNumber>\
    <UDN>uuid:$UID$</UDN>\
    <serviceList>\
      <service>\
        <serviceType>(null)</serviceType>\
        <serviceId>(null)</serviceId>\
        <controlURL>(null)</controlURL>\
        <eventSubURL>(null)</eventSubURL>\
        <SCPDURL>(null)</SCPDURL>\
      </service>\
    </serviceList>\
    <presentationURL>index.html</presentationURL>\
    <iconList>\
      <icon>\
        <mimetype>image/png</mimetype>\
        <height>48</height>\
        <width>48</width>\
        <depth>24</depth>\
        <url>hue_logo_0.png</url>\
      </icon>\
      <icon>\
        <mimetype>image/png</mimetype>\
        <height>120</height>\
        <width>120</width>\
        <depth>24</depth>\
        <url>hue_logo_3.png</url>\
      </icon>\
    </iconList>\
  </device>\
</root>";
//-----------------------//
struct tLight
{
  tLight()
  {
    m_bEnabled = false;
    m_uiUseCounter = 0;
  }
  String m_strName;
  String m_strDeviceID;
  String m_strTurnOnCall;
  String m_strTurnOffCall;
  bool m_bEnabled;
  uint32_t m_uiUseCounter;
  String IsEnabled()
  {
    return m_bEnabled ? "true" : "false";
  }
};
std::vector<tLight> vLights;
//-----------------------//
String SplitString(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
//-----------------------//
void SaveConfig()
{
  File configFile = SPIFFS.open("/config.txt", "w");
  if (!configFile)
  {
    Serial.println("Cant save, is SPIFFS ready and space avaialable?");
    return;
  }

  Serial.println("Saving config items: " + String(vLights.size()));
  configFile.println(String(vLights.size()));
  for (int x = 0; x < vLights.size(); x++)
  {
    configFile.println(vLights[x].m_strName);
    configFile.println(vLights[x].m_strTurnOnCall);
    configFile.println(vLights[x].m_strTurnOffCall);
    configFile.println(vLights[x].m_strDeviceID);
  }

  configFile.close();
}
//-----------------------//
bool LoadConfig()
{
  File configFile = SPIFFS.open("/config.txt", "r");
  if (!configFile)
  {
    return false;
  }

  int iCount = configFile.readStringUntil('\n').toInt();
  Serial.println("Config load items: " + String(iCount));
  for (int x = 0; x < iCount; x++)
  {
    tLight _new;

    _new.m_strName = configFile.readStringUntil('\n') ;
    _new.m_strTurnOnCall = configFile.readStringUntil('\n') ;
    _new.m_strTurnOffCall = configFile.readStringUntil('\n') ;
    _new.m_strDeviceID = configFile.readStringUntil('\n') ;

    vLights.push_back(_new);
    Serial.println("Loaded " + _new.m_strName);
  }
  configFile.close();
  return true;
}
//-----------------------//
void setup()
{
  //
  Serial.begin(115200);

  // SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("An error occurred while starting SPIFFS");
    delay(1000);
  }

  if (LoadConfig())
  {
    Serial.println("Config loaded, " + String(vLights.size()) + " devices added");
  }


  WiFi.begin(WIFI_A, WIFI_P);

  // Give it 10 seconds to connect, otherwise reboot
  uint8_t iRetries = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.println(".");
    delay(1000);
    iRetries += 1;

    if (iRetries >= 10)
      ESP.restart();
  }

  Serial.println("Connected " + WiFi.localIP().toString());

  ArduinoOTA
  .onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else  // U_SPIFFS
      type = "filesystem";
  })
  .onEnd([]() {

  })
  .onProgress([](unsigned int progress, unsigned int total) {

    yield();
  })
  .onError([](ota_error_t error) {

    ESP.restart();
  });
  ArduinoOTA.setHostname("ESPRIDGE");
  ArduinoOTA.begin();

  //
  Apache.onNotFound([  ]()
  {
    String strResponse;
    Serial.println( Apache.client().remoteIP().toString() + " -> " + Apache.uri());

    // Receiving specific light id
    if (Apache.uri().indexOf("/api/Echo/lights/") != -1)
    {
      String strID = SplitString(Apache.uri(), '/', 4);
      int iIndex = strID.toInt();

      if (iIndex < 0 || iIndex >= vLights.size())
        return;

      // Change the state of the light "/api/Echo/lights/5/state"
      if (Apache.uri().indexOf("/state") != -1)
      {
        // Alexa sends: plain: {"on":false}
        if (Apache.hasArg("plain"))
        {
          // Deserialize json
          DynamicJsonDocument jsonBuffer(512); // Adjust the size as per your JSON data
          deserializeJson(jsonBuffer, Apache.arg("plain"));

          String strEnableFlag = jsonBuffer["on"].as<String>();
          if (strEnableFlag == "false") vLights[iIndex].m_bEnabled = false;
          if (strEnableFlag == "true") vLights[iIndex].m_bEnabled = true;
          strResponse = "[{\"success\": {\"/lights/" + strID + "/state/on\": " + strEnableFlag + "}}]";
          Apache.sendContent(strResponse);

          // Call out external GET
          HTTPClient httpClient;
          uint32_t uiBytesWritten = 0;
          if (strEnableFlag == "true")httpClient.begin(vLights[iIndex].m_strTurnOnCall);
          else httpClient.begin(vLights[iIndex].m_strTurnOffCall);
          int httpCode = httpClient.GET();
          vLights[iIndex].m_uiUseCounter++;
        }
      }
      // Ask for the light status "/api/Echo/lights/5"
      else
      {
        strResponse = "{\n";
        strResponse += "\"state\": {\n";
        strResponse += "\"on\": " + vLights[iIndex].IsEnabled() + ",\n";
        strResponse += "\"reachable\": true\n";
        strResponse += "}\n";
        strResponse += "}";
      }
    }
    Apache.setContentLength(CONTENT_LENGTH_UNKNOWN);
    Apache.send(200, "application/json", "");
    Apache.sendContent(strResponse);
    Apache.sendContent(F(""));
  });
  Apache.on("/", [&]() {
    Serial.println( Apache.client().remoteIP().toString() + " -> " + Apache.uri());
    Apache.setContentLength(CONTENT_LENGTH_UNKNOWN);
    Apache.send(200, "text/html", "");
    Apache.sendContent("<html><Center><TT>");
    Apache.sendContent("<H3>ESPRIDGE</H3>");
    Apache.sendContent(VER"<BR>");
    Apache.sendContent("<a href=https://github.com/invpe/ESPRIDGE>github</a><BR>");
    Apache.sendContent("<HR>");
    Apache.sendContent("<table>");
    Apache.sendContent("<TD>Action</TD><TD>ID</TD><TD>Name</TD><TD>UseCounter</td><TR>");
    for (int x = 0; x < vLights.size(); x++)
    {
      Apache.sendContent("<TD>");
      Apache.sendContent("[<a href=/del?id=" + String(x) + ">X</a>][<a href=/edit?id=" + String(x) + ">E</a>]");
      Apache.sendContent("</TD>");

      Apache.sendContent("<TD>");
      Apache.sendContent(String(x));
      Apache.sendContent("</TD>");

      Apache.sendContent("<TD>");
      Apache.sendContent(vLights[x].m_strName);
      Apache.sendContent("</TD>");

      Apache.sendContent("<TD>");
      Apache.sendContent(String(vLights[x].m_uiUseCounter));
      Apache.sendContent("</TD>");

      Apache.sendContent("</TR>");
    }
    Apache.sendContent("</table>");
    Apache.sendContent("<BR>");

    Apache.sendContent("[<a href=/edit>Add new</a>]");
    Apache.sendContent(F(""));
  });
  Apache.on("/del", [&]() {
    Serial.println( Apache.client().remoteIP().toString() + " -> " + Apache.uri());
    if (Apache.hasArg("id"))
    {
      int iID = Apache.arg("id").toInt();
      vLights.erase(vLights.begin() + iID);
    }

    SaveConfig();
    Apache.sendHeader("Location", "/", true);
    Apache.send ( 302, "text/plain", "");
  });
  Apache.on("/edit", [&]() {
    Serial.println( Apache.client().remoteIP().toString() + " -> " + Apache.uri());
    Apache.setContentLength(CONTENT_LENGTH_UNKNOWN);
    Apache.send(200, "text/html", "");
    String strID = "NEW";
    String strName = "UniqueName";
    String strOnAction = "http://somethinghere/turnon";
    String strOffAction = "http://somethinghere/turnoff";
    if (Apache.hasArg("id"))
    {
      int iID       = Apache.arg("id").toInt();
      strID         = Apache.arg("id");
      strName       = vLights[iID].m_strName;
      strOnAction   = vLights[iID].m_strTurnOnCall;
      strOffAction  = vLights[iID].m_strTurnOffCall;
    }
    Apache.sendContent("<html><Center><TT>");
    Apache.sendContent("<form action=save method=get>");
    Apache.sendContent("ID <input type=text name=id value=" + strID + " readonly></input><BR>");
    Apache.sendContent("NAME <input type=text name=name value=" + strName + "></input><BR>");
    Apache.sendContent("ONUrl <input type=text name=onaction value=" + strOnAction + "></input><BR>");
    Apache.sendContent("OFFUrl <input type=text name=offaction value=" + strOffAction + "></input><BR>");
    Apache.sendContent("<input type=submit>");
    Apache.sendContent(F(""));
  });
  Apache.on("/save", [&]() {
    Serial.println( Apache.client().remoteIP().toString() + " -> " + Apache.uri());
    if (Apache.hasArg("id") && Apache.hasArg("name") && Apache.hasArg("onaction") && Apache.hasArg("offaction"))
    {
      if (Apache.arg("id") == "NEW")
      {
        tLight _new;
        _new.m_strName         = Apache.arg("name");
        _new.m_strTurnOnCall   = Apache.arg("onaction");
        _new.m_strTurnOffCall  = Apache.arg("offaction");

        // Needs better idea, so we don't duplicate the id :-)
        char formattedString[5];
        sprintf(formattedString, "%02X", rand() % 255);
        _new.m_strDeviceID = "00:17:88:01:08:ff:" + String(formattedString) + ":ff-0b";

        vLights.push_back(_new);
      }
      else
      {
        int iID = Apache.arg("id").toInt();
        if (vLights.size() >= 1 && iID >= 0 && iID < vLights.size())
        {
          vLights[iID].m_strName        = Apache.arg("name");
          vLights[iID].m_strTurnOnCall  = Apache.arg("onaction");
          vLights[iID].m_strTurnOffCall = Apache.arg("offaction");
        }
      }
    }
    SaveConfig();
    Apache.sendHeader("Location", "/", true);
    Apache.send ( 302, "text/plain", "");
  });
  // Respond to service description
  Apache.on("/description.xml", [&]() {
    Serial.println( Apache.client().remoteIP().toString() + " -> " + Apache.uri());
    Apache.setContentLength(CONTENT_LENGTH_UNKNOWN);
    Apache.send(200, "application/xml", "");
    String strResponse = strDescriptionXML;
    strResponse.replace("$IP$", WiFi.localIP().toString() );
    strResponse.replace("$UID$", strUID);
    Apache.sendContent(strResponse);
    Apache.sendContent(F(""));
  });

  // Register the user, this is tailored for alexa mainly
  Apache.on("/api", [&]() {
    Serial.println( Apache.client().remoteIP().toString() + " -> " + Apache.uri());
    Apache.setContentLength(CONTENT_LENGTH_UNKNOWN);
    Apache.send(200, "application/json", "");

    // Alexa sends: plain: {"devicetype": "Echo"}
    if (Apache.hasArg("plain")) {

      // Deserialize json
      DynamicJsonDocument jsonBuffer(512); // Adjust the size as per your JSON data
      deserializeJson(jsonBuffer, Apache.arg("plain"));

      // Extract the "devicetype" value from the JSON data
      String deviceType = jsonBuffer["devicetype"].as<String>();
      String strResponse = "[{\"success\": {\"username\": \"" + deviceType + "\"}}]";
      Apache.sendContent(strResponse);
    }
    Apache.sendContent(F(""));

  });
  // Retrieve list of all lights
  Apache.on("/api/Echo/lights", [&]() {
    Serial.println( Apache.client().remoteIP().toString() + " -> " + Apache.uri());
    Apache.setContentLength(CONTENT_LENGTH_UNKNOWN);
    Apache.send(200, "application/json", "");

    Apache.sendContent("{");
    for (int x = 0; x < vLights.size(); x++)
    {
      Apache.sendContent("\"" + String(x) + "\":{\n");
      Apache.sendContent("\"state\": {\n");
      Apache.sendContent("\"on\": " + vLights[x].IsEnabled() + "\n,");
      Apache.sendContent("\"reachable\": true\n");
      Apache.sendContent("},\n");
      Apache.sendContent("\"type\": \"Extended color light\",\n");
      Apache.sendContent("\"name\": \"" + vLights[x].m_strName + "\",\n");
      Apache.sendContent("\"manufacturername\": \"Signify Netherlands B.V.\",\n");
      Apache.sendContent("\"productname\": \"Hue color lamp\",\n");
      Apache.sendContent("\"uniqueid\": \"" + vLights[x].m_strDeviceID + "\"\n");
      Apache.sendContent("}\n");
      if (vLights.size() - 1 > x)
        Apache.sendContent(",\n");
    }
    Apache.sendContent("}");
  });

  Apache.begin();
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  udp.beginMulticast(IPAddress(239, 255, 255, 250), SSDP_PORT);
}
//-----------------------//
void handleSSDP() {
  int packetSize = udp.parsePacket();
  if (packetSize) {
    char packetBuffer[2048];
    int len = udp.read(packetBuffer, sizeof(packetBuffer));
    if (len > 0) {
      packetBuffer[len] = 0;
      if (strstr(packetBuffer, "M-SEARCH") != nullptr && strstr(packetBuffer, "ssdp:discover") != nullptr) {
        Serial.println( udp.remoteIP().toString() + " -> M-SEARCH");
        udp.beginPacket(udp.remoteIP(), udp.remotePort());
        udp.print(
          "HTTP/1.1 200 OK\r\n"
          "CACHE-CONTROL: max-age=86400\r\n"
          "EXT:\r\n"
          "LOCATION: http://" + WiFi.localIP().toString() + ":80/description.xml\r\n"
          "SERVER: FreeRTOS/6.0.5, UPnP/1.0, IpBridge/0.1\r\n"
          "ST: upnp:rootdevice\r\n"
          "USN: uuid:" + strUID + "::upnp:rootdevice\r\n\r\n");
        udp.endPacket();
      }
    }
  }
}
//-----------------------//
void loop() {
  tm local_tm;
  getLocalTime(&local_tm);
  time_t tTimeSinceEpoch = mktime(&local_tm);

  // Check if WiFi available, if not just boot.
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WIFI Bad, rebooting");
    ESP.restart();
  }
  Apache.handleClient();

#ifdef OTA_SUPPORT
  ArduinoOTA.handle();
#endif

  handleSSDP();
}

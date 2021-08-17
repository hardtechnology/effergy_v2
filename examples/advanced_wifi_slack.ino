// Version 2021.8.16 - August 2021

// This requires the following Libraries to be installed (see includes for links)
// ArduinoJSON 6.18.3
// ESP8266 1.0.0
// WiFiManager 0.12.0
// PubSubClient 2.6.0

//#include <FS.h>                   //this needs to be first, or it all crashes and burns...
//#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
//#include <DNSServer.h>
//#include <ESP8266WebServer.h>
//#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
//#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson
//#include <SPI.h>
//#include <PubSubClient.h>         //https://github.com/knolleary/pubsubclient
#include <efergy.h>

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

//SSL Client Variable
WiFiClientSecure client;

// slack webhook host and endpoint information
const char* host = "hooks.slack.com";
const char* SECRET_SlackWebhookURL = "/services/XXXXX/YYYYYYY"; //Your Slack Webhook

// Slack Channel Info, Bot User Name, and Message
const String slack_username = "Efergy";

#define DEBUG 0
#define inpin 16          //Input pin (DATA_OUT from A72C01) on pin 2 (D3) (Pin D0 on the Wemos D1 mini)
#define voltage 240

efergy Efergymon(inpin,DEBUG,voltage);  // Define Efergy Module to look after radio and packet decoding

void setup() {
  Efergymon.begin(74880);
  //setID (int id, int depth, unsigned long statusmA, int intervalsecs)
  // On mA Should be >10mA above Standby/Off to prevent false triggering
  Efergymon.setID(<transmitterid>,12,50,90);  // log Tx 12345, depth of 12 records (@6 seconds = 2 minutes), On is considered over 40mA, report @ 120 seconds
  WiFi.begin("<SSID>","<passphrase>");
  char *SOURCE_FILENAME = __FILE__;
  Serial.print("\nArduino Source File: ");
  Serial.print(SOURCE_FILENAME);
  Serial.print("\nConnecting to Wifi.");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    // wait 1 second for re-trying
    delay(1000);
  }
  Serial.print("Connected. (");
  Serial.print(WiFi.localIP());
  Serial.print(")");
  // Wait a little to allow network to stabilise
  delay(1000);
  delay(1000);
  delay(1000);
  slackPost("#general","Power Monitor Restarted...");
}

void loop() {
  if (Efergymon.mainloop()) {
    bool updateme = bool(Efergymon.getjsonevent()["changed"]);
    if (updateme) {
      bool tx_status = bool(Efergymon.getjsonevent()["status"]);
      int tx_id = int(Efergymon.getjsonevent()["id"]);
      Serial.print("\nStatus has Changed: ");
      Serial.print((tx_status ? "On" : "Off"));
      if (tx_id == 12345) {  //Washing Machine
        if ( !tx_status ) {
          slackPost("#general","@channel Washing has finished.");
        } else {
          slackPost("#general","Washing has started.");
        }
      }

    }
  yield();
  }
}

void slackPost(const String slack_channel, const String slack_message) {
  // create a secure connection using WiFiClientSecure
  client.setInsecure();
  if (client.connect("hooks.slack.com", 443)) {   // hooks.slack.com
    String PostData="payload={\
       \"text\": \"" + slack_message + "\",\
       \"icon_emoji\": \":bell:\",\
       \"channel\": \"" + slack_channel + "\",\
       \"username\": \"" + slack_username + "\",\
       \"blocks\": [\
           {\
            \"type\": \"section\",\
            \"text\": {\
                \"text\": \"" + slack_message + "\" ,\
                \"type\": \"mrkdwn\"\
                }\
           }\
       ]\
    }";
    client.print("POST ");
    client.print(SECRET_SlackWebhookURL);
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(host);
    client.println("User-Agent: ArduinoIoT/1.0");
    client.println("Connection: close");
    client.println("Content-Type: application/x-www-form-urlencoded;");
    client.print("Content-Length: ");
    client.println(PostData.length());
    client.println();
    client.println(PostData);
    client.stop();
  } else {
    Efergymon.eflog("No Client connectivity to Slack",true);
  }
}
/*
  RF 433 MHz to MQTT Gateway.
  Paul Philippov, themactep@gmail.com
*/

#include <ESP8266WiFi.h>
#include <RCSwitch.h>
#include <PubSubClient.h>

#define WIFI_SSID  "myssid"
#define WIFI_PASS  "mypass"

#define MQTT_HOST  "192.168.0.0"
#define MQTT_USER  "mqttuser"
#define MQTT_PASS  "mqttpass"
#define MQTT_NAME  "rf433_gateway"
#define MQTT_TOPIC  "home/rf433/"

#define RX_PIN  D7

#define SIGNAL_DEBOUNCE_SEC 3

WiFiClient wifiClient;
PubSubClient mqttClient;

RCSwitch rx = RCSwitch();

uint32_t last_signal_value = 0;
uint32_t last_signal_timeout = 0;

void setup() {
  Serial.begin(115200);

  Serial.println();
  Serial.printf("Connecting to WiFi network %s ", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  mqttClient.setClient(wifiClient);
  mqttClient.setServer(MQTT_HOST, 1883);

  rx.enableReceive(RX_PIN);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection lost. Restarting.");
    delay(5000);
    abort();
  }

  if (! mqttClient.connected()) {
    Serial.println("No connection to MQTT.");
    Serial.print("State: ");
    Serial.println(mqttClient.state());
    while (! mqttClient.connected() ) {
      Serial.print("Connecting to MQTT broker.");
      if (mqttClient.connect(MQTT_NAME, MQTT_USER, MQTT_PASS)) {
        Serial.println(" connected.");
      } else {
        Serial.println(" failed.");
        Serial.println(mqttClient.state());
        Serial.println("Will try again in 5 seconds...");
        delay(5000);
      }
    }
  }

  if (millis() > last_signal_timeout) {
    last_signal_value = 0;
    last_signal_timeout = 0;
  }

  if (rx.available()) {
    uint32_t signal_value = rx.getReceivedValue();

    if (signal_value == 0) {
      Serial.println("Unknown encoding.");
    } else if (signal_value == last_signal_value) {
      Serial.println("\""); // ditto mark
    } else {
      Serial.println(signal_value);

      String topicString = MQTT_TOPIC;
      topicString += signal_value;
      uint8_t l = topicString.length() + 1;
      char topic[l];
      topicString.toCharArray(topic, l);
      mqttClient.publish(topic, "on");

      last_signal_value = signal_value;
      last_signal_timeout = millis() + (SIGNAL_DEBOUNCE_SEC * 1000);
    }

    rx.resetAvailable();
  }

  mqttClient.loop();
}

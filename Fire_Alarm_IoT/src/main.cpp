#include <Arduino.h>
#include <DHT.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>
#include "secrets/wifi.h"
#include "secrets/mqtt.h"
#include "wifi_connect.h"
#include <WiFiClientSecure.h>
#include <Ticker.h>
#include "ca_cert.h"

#define DHT_PIN 4
#define DHT_TYPE DHT12
#define MQ2_PIN 34
#define LED_PIN 25
#define BUZZER_PIN 26
#define SERVO_PIN 27

DHT dht(DHT_PIN, DHT_TYPE);
Servo servo;

bool alarmOff = false;
float temp, hum;
int gasLevel;

namespace
{
    const char *ssid = WiFiSecrets::ssid;
    const char *password = WiFiSecrets::pass;
    const char *Temp_topic = "Temperature";
    const char *Hum_topic = "Humidity";
    const char *gasLevel_topic = "Gas Level";
    const char *alarm_topic = "Alarm Control";
    unsigned int publish_count = 0;
    uint16_t keepAlive = 15;    // seconds (default is 15)
    uint16_t socketTimeout = 5; // seconds (default is 15)
}

WiFiClientSecure tlsClient;
PubSubClient mqttClient(tlsClient);

Ticker mqttPulishTicker;

void mqttPublish()
{
    temp = dht.readTemperature();
    hum = dht.readHumidity();
    gasLevel = analogRead(MQ2_PIN);

    if (isnan(temp) || isnan(hum))
    {
      Serial.println("Error occoured while reading DHT22!");
    }

    Serial.printf("Temperature: %.2f°C\nHumidity: %.2f%%\nGas Level: %d\n", temp, hum, gasLevel);
    
    Serial.print("Publishing to MQTT...");
    mqttClient.publish(Temp_topic, String(temp).c_str(), false);
    mqttClient.publish(Hum_topic, String(hum).c_str(), false);
    mqttClient.publish(gasLevel_topic, String(gasLevel).c_str(), false);

    if (temp > 70 || gasLevel > 300)
    {
      digitalWrite(LED_PIN, HIGH);
      digitalWrite(BUZZER_PIN, HIGH);
      servo.write(90);
      Serial.println("FIRE!!!");
    } else {
      digitalWrite(LED_PIN, LOW);
      digitalWrite(BUZZER_PIN, LOW);
      servo.write(0);
    }
  delay(2000);
}

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
    String message;
    for (int i = 0; i < length; i++)
    {
        Serial.print((char)payload[i]);
        message += (char)payload[i];
    }
    Serial.println();
    message.trim();

    if (String(topic) == 0)
    {
      if (message == "OFF")
      {
        alarmOff = true;
        digitalWrite(LED_PIN, LOW);
        digitalWrite(BUZZER_PIN, LOW);
        servo.write(0);
        Serial.println("Alarm turned off!");
      }
    }
}

void setup()
{
    Serial.begin(115200);
    delay(10);
    setup_wifi(ssid, password);
    tlsClient.setCACert(ca_cert);

    mqttClient.setCallback(mqttCallback);
    mqttClient.setServer(MQTT::broker, MQTT::port);
    mqttPulishTicker.attach(1, mqttPublish);

    dht.begin();
    servo.attach(SERVO_PIN);
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUZZER_PIN, OUTPUT);
}

void mqttReconnect()
{
    while (!mqttClient.connected())
    {
        Serial.println("Attempting MQTT connection...");
        String client_id = "esp32-client-";
        client_id += String(WiFi.macAddress());
        if (mqttClient.connect(client_id.c_str(), MQTT::username, MQTT::password))
        {
            Serial.print(client_id);
            Serial.println(" connected");
            mqttClient.subscribe(alarm_topic);
        }
        else
        {
            Serial.print("MTTT connect failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" try again in 1 seconds");
            delay(1000);
        }
    }
}

void loop()
{
    if (!mqttClient.connected())
    {
        mqttReconnect();
    }
    mqttClient.loop();
}
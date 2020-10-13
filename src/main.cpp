#include "pins_arduino.h"
#include "config.h"
#include <Arduino.h>
#include "SenseConfig.h"
#include "Sense.h"
Sense sense;

// #include <bluetooth/SenseBLE.h>
#include <SenseWifi.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "time.h"

#include "Climate.h"
#include "WeatherForcast.h"

bool ready = false;
bool WiFiReady = false;
long lastReconnectAttempt = 0;

String deviceName = "SystemAir-ESP32";
String MQTTTopicString = "systemair/vr400-esp32/";
String MQTTUsernameString;
String MQTTPwdString;
String MQTTHostString;
uint16_t MQTTPortInt;

boolean MQTTReconnect();
void senseWiFiEvent(WiFiEvent_t event);

void uptime();
void publishDiscovery();
void setSubscribeTopics();

void callback(char *topic, byte *payload, unsigned int length);     // MQTT Subscribe callback
void sensorPayloadCallback(const char *topic, const char *payload); // Sensor Payload callback

// SenseBLE senseBLE;
SenseWifi senseWifi(senseWiFiEvent);
Climate climate(sensorPayloadCallback);
WiFiClient client;
PubSubClient senseMQTT("", 1883, callback, client);
WeatherForcast wf;

// instantiate ModbusMaster object
ModbusMaster node;

unsigned long previousMillis = 0;
const unsigned long interval = 60 * 1000;

void senseWiFiEvent(WiFiEvent_t event)
{
    switch (event)
    {
    case SYSTEM_EVENT_STA_GOT_IP:
        digitalWrite(BUILTIN_LED, HIGH);
        WiFiReady = true;
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        digitalWrite(BUILTIN_LED, LOW);
        WiFiReady = false;
        if (senseMQTT.connected())
        {
            lastReconnectAttempt = 0;
            senseMQTT.disconnect();
        }
        break;
    default:
        break;
    }
}

boolean MQTTReconnect()
{
    if (MQTTPwdString.length() > 0)
    {
        senseMQTT.setServer(MQTTHostString.c_str(), MQTTPortInt);
        senseMQTT.connect(deviceName.c_str(), MQTTUsernameString.c_str(), MQTTPwdString.c_str());
        if (senseMQTT.connect(deviceName.c_str(), MQTTUsernameString.c_str(), MQTTPwdString.c_str()))
        {
            setSubscribeTopics();
            uptime();
            publishDiscovery();
        }
    }
    return senseMQTT.connected();
}

void setSubscribeTopics()
{
    senseMQTT.setCallback(callback);
    String fanTopic = MQTTTopicString + "fan/set";
    senseMQTT.subscribe(fanTopic.c_str());
    String tempTopic = MQTTTopicString + "temperature/set";
    senseMQTT.subscribe(tempTopic.c_str());
    String modeTopic = MQTTTopicString + "mode/set";
    senseMQTT.subscribe(modeTopic.c_str());
}

void callback(char *topic, byte *payload, unsigned int length)
{
    String stringTopic = topic;
    String value;
    for (int i = 0; i < length; i++)
    {
        value += (char)payload[i];
    }

    String fanTopic = MQTTTopicString + "fan/set";
    if (stringTopic == fanTopic)
    {
        climate.setFanSpeedString(value);
    }

    String temperatureTopic = MQTTTopicString + "temperature/set";
    if (stringTopic == temperatureTopic)
    {
        climate.setTargetTemperature(value.toDouble());
    }

    String modeTopic = MQTTTopicString + "mode/set";
    if (stringTopic == modeTopic)
    {
        climate.setModeState(value);
    }
}

void sensorPayloadCallback(const char *topic, const char *payload)
{
    if (senseWifi.connected() && senseMQTT.connected())
    {
        String mqTopic = MQTTTopicString;
        mqTopic += topic;
        senseMQTT.publish(mqTopic.c_str(), payload);
    }
}

void preTransmission()
{
    digitalWrite(BUILTIN_LED, LOW);
    digitalWrite(MAX485_RE_NEG, HIGH);
    digitalWrite(MAX485_DE, HIGH);
}

void postTransmission()
{
    // https://github.com/4-20ma/ModbusMaster/issues/93
    delay(2); //DE is pulled down too quiclky on ESP8266 and cuts off the Modbus message in the CRC
    digitalWrite(MAX485_RE_NEG, LOW);
    digitalWrite(MAX485_DE, LOW);
    digitalWrite(BUILTIN_LED, HIGH);
}

void setup()
{
    pinMode(BUILTIN_LED, OUTPUT);
    pinMode(ORANGE_LED, OUTPUT);
    digitalWrite(ORANGE_LED, HIGH);
    Serial.begin(19200);

    pinMode(MAX485_RE_NEG, OUTPUT);
    pinMode(MAX485_DE, OUTPUT);
    // Init in receive mode
    digitalWrite(MAX485_RE_NEG, LOW);
    digitalWrite(MAX485_DE, LOW);

    sense.begin();

    // senseBLE.begin();
    senseWifi.begin();

    deviceName = Sense::getDeviceName();
    MQTTUsernameString = Sense::getMQTTUser();
    MQTTPwdString = Sense::getMQTTPwd();
    MQTTTopicString = Sense::getMQTTTopic();
    MQTTHostString = Sense::getMQTTHost();
    MQTTPortInt = Sense::getMQTTPort();
    delay(100);

    // Modbus slave ID 1
    node.begin(1, Serial);
    // Callbacks allow us to configure the RS485 transceiver correctly
    node.preTransmission(preTransmission);
    node.postTransmission(postTransmission);
    climate.begin(node);
}

void publishDiscovery()
{
    if (senseWifi.connected() && senseMQTT.connected())
    {
        // String stateTopic = MQTTTopicString;
        // stateTopic += "state";

        // String tempCmd = MQTTTopicString;
        // tempCmd += "targetTempCmd";

        StaticJsonDocument<300> doc;

        doc["name"] = "VR400";
        doc["unique_id"] = deviceName.c_str();
        // doc["mode_cmd_t"] = "homeassistant/climate/livingroom/thermostatModeCmd";
        // doc["mode_stat_t"] = "homeassistant/climate/livingroom/state";
        // doc["mode_stat_tpl"] = "";
        // doc["avty_t"] = "homeassistant/climate/livingroom/available";
        // doc["pl_avail"] = "online";
        // doc["pl_not_avail"] = "offline";
        // doc["temp_cmd_t"] = tempCmd.c_str();
        // doc["temp_stat_t"] = stateTopic.c_str();
        // doc["temp_stat_tpl"] = "";
        // doc["curr_temp_t"] = stateTopic.c_str();
        // doc["curr_temp_tpl"] = "";
        // doc["min_temp"] = "10";
        // doc["max_temp"] = "30";
        // doc["temp_step"] = "1";
        // doc["modes"] =  ["off", "heat"]

        String payload;
        serializeJson(doc, payload);
        doc.clear();

        senseMQTT.publish("homeassistant/climate/VR400/config", payload.c_str());
    }
}

void uptime()
{
    StaticJsonDocument<120> doc;

    doc["uptime"] = millis() / 60000;
    doc["freeHeep"] = ESP.getFreeHeap();
    doc["rssi"] = WiFi.RSSI();
    doc["version"] = STR(VERSION);

    String payload;
    serializeJson(doc, payload);
    sensorPayloadCallback("uptime", payload.c_str());
}

void loop()
{
    if (senseWifi.connected() && WiFiReady)
    {
        if (senseMQTT.connected())
        {
            senseMQTT.loop();
        }
        else
        {
            long now = millis();
            if (now - lastReconnectAttempt > 5000)
            {
                lastReconnectAttempt = now;
                if (MQTTReconnect())
                {
                    lastReconnectAttempt = 0;
                }
            }
        }
        wf.loop();
    }

    // senseBLE.loop();

    if (!ready)
    {
        // Data is read and ready to send
        ready = true;
        publishDiscovery();
        // uint16_t test1 = 15;
        // uint16_t test = test1 << 0;

        // Serial.println(test);
    }

    climate.loop();

    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval)
    {
        previousMillis += interval;
        uptime();
    }
}

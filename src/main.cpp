#include "pins_arduino.h"
#include "config.h"
#include <Arduino.h>
#include "SenseConfig.h"
#include "Sense.h"
Sense sense;

#include <bluetooth/SenseBLE.h>
#include <SenseWifi.h>
#include <WiFi.h>
#include <ModbusMaster.h>
#include <PubSubClient.h>
#include "time.h"

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
void callback(char *topic, byte *payload, unsigned int length);
void senseWiFiEvent(WiFiEvent_t event);

void uptime();
void printLocalTime();
void publishDiscovery();
void setSubscribeTopics();

void callback(char *topic, byte *payload, unsigned int length);     // MQTT Subscribe callback
void sensorPayloadCallback(const char *topic, const char *payload); // Sensor Payload callback

SenseBLE senseBLE;
SenseWifi senseWifi(senseWiFiEvent);
WiFiClient client;
PubSubClient senseMQTT("", 1883, callback, client);

// instantiate ModbusMaster object
ModbusMaster node;

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

void setFan(String value)
{
    uint16_t target = 0;
    if (value == "low")
    {
        target = 0x01;
    }
    else if (value == "medium")
    {
        target = 0x02;
    }
    else if (value == "high")
    {
        target = 0x03;
    }
    node.writeSingleRegister(0x64, target);
}
/*
def get_rotor_state(self):
if not savecair:
req.modbusregister(206,0)
    self.exchanger_mode= req.response
req.modbusregisters(350,2)
        self.rotor_state = req.response[0]
        if req.response[1]:
    self.rotor_active = "Yes"
    self.exchanger_speed= 100
        else: self.rotor_active = "No"
*/

void callback(char *topic, byte *payload, unsigned int length)
{
    String fanTopic = MQTTTopicString + "fan/set";
    String stringTopic = topic;
    String value;
    for (int i = 0; i < length; i++)
    {
        value += (char)payload[i];
    }
    if (stringTopic == fanTopic)
    {
        setFan(value);
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
    digitalWrite(MAX485_RE_NEG, 1);
    digitalWrite(MAX485_DE, 1);
}

void postTransmission()
{
    // https://github.com/4-20ma/ModbusMaster/issues/93
    delay(2); //DE is pulled down too quiclky on ESP8266 and cuts off the Modbus message in the CRC
    digitalWrite(MAX485_RE_NEG, 0);
    digitalWrite(MAX485_DE, 0);
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

    senseBLE.begin();
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

void publishState()
{
    digitalWrite(ORANGE_LED, LOW);
    uint8_t result;
    result = node.readInputRegisters(0xD5, 5);
    if (result == node.ku8MBSuccess)
    {
        StaticJsonDocument<92> doc;

        doc["mode"] = "heat";
        doc["target_temp"] = 23.0f;
        doc["current_temp"] = (double)node.getResponseBuffer(0x01) / 10.0;

        String payload;
        serializeJson(doc, payload);
        doc.clear();

        sensorPayloadCallback("state", payload.c_str());
    }
    digitalWrite(ORANGE_LED, HIGH);
}

void getSystemName()
{
    digitalWrite(ORANGE_LED, LOW);
    uint8_t result;
    result = node.readInputRegisters(0x1F4, 1);
    if (result == node.ku8MBSuccess)
    {
        StaticJsonDocument<92> doc;

        doc["name"] = node.getResponseBuffer(0x00);

        String payload;
        serializeJson(doc, payload);
        doc.clear();

        sensorPayloadCallback("system", payload.c_str());
    }
    digitalWrite(ORANGE_LED, HIGH);
}

void getHeater()
{
    digitalWrite(ORANGE_LED, LOW);
    uint8_t result;
    result = node.readInputRegisters(0xC8, 1);
    if (result == node.ku8MBSuccess)
    {
        StaticJsonDocument<92> doc;

        doc["heater"] = node.getResponseBuffer(0x00);

        String payload;
        serializeJson(doc, payload);
        doc.clear();

        sensorPayloadCallback("heater", payload.c_str());
    }
    digitalWrite(ORANGE_LED, HIGH);
}

void getFanspeed()
{
    digitalWrite(ORANGE_LED, LOW);
    uint8_t result;
    result = node.readInputRegisters(0x64, 1);
    if (result == node.ku8MBSuccess)
    {
        String speed = "off";

        switch (node.getResponseBuffer(0x00))
        {
        case 0:
            speed = "off";
            break;
        case 1:
            speed = "low";
            break;
        case 2:
            speed = "medium";
            break;
        case 3:
            speed = "high";
            break;

        default:
            break;
        }

        sensorPayloadCallback("fan/state", speed.c_str());
    }
    digitalWrite(ORANGE_LED, HIGH);
}

void getTemperatures()
{
    digitalWrite(ORANGE_LED, LOW);
    uint8_t result;
    result = node.readInputRegisters(0xD5, 5);
    if (result == node.ku8MBSuccess)
    {
        StaticJsonDocument<92> doc;

        doc["pre-supply"] = (double)node.getResponseBuffer(0x00) / 10.0;
        doc["extract"] = (double)node.getResponseBuffer(0x01) / 10.0;
        doc["exhaust"] = (double)node.getResponseBuffer(0x02) / 10.0;
        doc["supply"] = (double)node.getResponseBuffer(0x03) / 10.0;
        doc["inlet"] = (double)node.getResponseBuffer(0x04) / 10.0;

        String payload;
        serializeJson(doc, payload);
        doc.clear();

        sensorPayloadCallback("temperature/state", payload.c_str());
    }
    digitalWrite(ORANGE_LED, HIGH);
}

void getFanSpeedLevel()
{
    digitalWrite(ORANGE_LED, LOW);
    uint8_t result;
    result = node.readInputRegisters(0x65, 6);
    if (result == node.ku8MBSuccess)
    {
        StaticJsonDocument<92> doc;

        doc["sf1"] = node.getResponseBuffer(0x00);
        doc["sf2"] = node.getResponseBuffer(0x01);
        doc["sf3"] = node.getResponseBuffer(0x02);
        doc["ef1"] = node.getResponseBuffer(0x03);
        doc["ef2"] = node.getResponseBuffer(0x04);
        doc["ef3"] = node.getResponseBuffer(0x05);

        String payload;
        serializeJson(doc, payload);
        doc.clear();

        sensorPayloadCallback("fanspeedLevel", payload.c_str());
    }
    digitalWrite(ORANGE_LED, HIGH);
}

void getModeState()
{
    digitalWrite(ORANGE_LED, LOW);
    String mode = "fan_only";
    sensorPayloadCallback("mode/state", mode.c_str());
    digitalWrite(ORANGE_LED, HIGH);
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

    getSystemName();
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
    }

    senseBLE.loop();

    if (!ready)
    {
        // Data is read and ready to send
        ready = true;
        publishDiscovery();
    }

    getModeState();
    delay(500);
    getFanspeed();
    delay(500);
    getTemperatures();
    delay(500);
    getHeater();
    // getFanSpeedLevel();

    delay(1000);
}

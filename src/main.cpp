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

#include <WiFi.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>

bool ready = false;
bool WiFiReady = false;
long lastReconnectAttempt = 0;

String deviceName = "SystemAir-ESP32";
String chipId = "VTR300-esp32";
String MQTTTopicString = "systemair/VTR300-esp32/";
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
// WeatherForcast wf;

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
        senseMQTT.setBufferSize(1024);
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
    String tempTopic = MQTTTopicString + "temp/set";
    senseMQTT.subscribe(tempTopic.c_str());
    String modeTopic = MQTTTopicString + "mode/set";
    senseMQTT.subscribe(modeTopic.c_str());
    String presetTopic = MQTTTopicString + "preset/set";
    senseMQTT.subscribe(presetTopic.c_str());
    String otaTopic = MQTTTopicString + "system/ota/set";
    senseMQTT.subscribe(otaTopic.c_str());
    String homeAssistantStatus = "homeassistant/status";
    senseMQTT.subscribe(homeAssistantStatus.c_str());
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

    String temperatureTopic = MQTTTopicString + "temp/set";
    if (stringTopic == temperatureTopic)
    {
        double target = value.toDouble();
        int16_t targetInt = target * 10;
        climate.setTargetTemperature(targetInt);
    }

    String modeTopic = MQTTTopicString + "mode/set";
    if (stringTopic == modeTopic)
    {
        climate.setModeState(value);
    }

    String presetTopic = MQTTTopicString + "preset/set";
    if (stringTopic == presetTopic)
    {
        climate.setPresetState(value);
    }

    String otaTopic = MQTTTopicString + "system/ota/set";
    if (stringTopic == otaTopic && value != "")
    {
        StaticJsonDocument<120> doc;
        doc["state"] = "received";
        doc["version"] = STR(VERSION);
        doc["file"] = value;

        String payload;
        serializeJson(doc, payload);
        sensorPayloadCallback("system/ota", payload.c_str());

        setClock();
        WiFiClientSecure client;
        client.setCACert(OTA_ROOT_CA);

        // Reading data over SSL may be slow, use an adequate timeout
        client.setTimeout(12000 / 1000); // timeout argument is defined in seconds for setTimeout

        t_httpUpdate_return ret = httpUpdate.update(client, "ha.dukkelunden.no", 443, value, STR(VERSION));

        switch (ret)
        {
        case HTTP_UPDATE_FAILED:
            doc["state"] = "failed";
            doc["error"] = httpUpdate.getLastErrorString().c_str();
            break;

        case HTTP_UPDATE_NO_UPDATES:
            doc["state"] = "failed";
            doc["error"] = "no updates";
            break;

        case HTTP_UPDATE_OK:
            doc["state"] = "ok";
            break;
        }
        String payload2;
        serializeJson(doc, payload2);
        sensorPayloadCallback("system/ota", payload2.c_str());
    }

    String homeAssistantStatus = "homeassistant/status";
    if (stringTopic == homeAssistantStatus)
    {
        if (value == "online")
        {
            publishDiscovery();
        }
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
    delay(2); // DE is pulled down too quiclky on ESP8266 and cuts off the Modbus message in the CRC
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
    chipId = Sense::getChipId();
    MQTTUsernameString = Sense::getMQTTUser();
    MQTTPwdString = Sense::getMQTTPwd();
    MQTTTopicString = Sense::getMQTTTopic();
    MQTTHostString = Sense::getMQTTHost();
    MQTTPortInt = Sense::getMQTTPort();
    delay(100);

    // Modbus slave ID 1
    node.begin(2, Serial);
    // Callbacks allow us to configure the RS485 transceiver correctly
    node.preTransmission(preTransmission);
    node.postTransmission(postTransmission);
    climate.begin(node);
}

void publishDiscoveryClimate()
{
    DynamicJsonDocument doc(1024);

    doc["name"] = "Ventilation";
    doc["uniq_id"] = chipId.c_str();
    doc["~"] = MQTTTopicString;

    doc["dev"]["name"] = "Ventilation";
    doc["dev"]["mdl"] = "Save VTR300/B";
    doc["dev"]["mf"] = "SystemAir";
    doc["dev"]["ids"][0] = chipId.c_str();
    doc["dev"]["sw"] = STR(VERSION);

    doc["target_temperature_low"] = 12;
    doc["target_temperature_high"] = 30;

    doc["pr_mode_cmd_t"] = "~preset/set";
    doc["pr_mode_stat_t"] = "~mode/state";
    doc["pr_mode_val_tpl"] = "{{ value_json.um }}";
    doc["pr_modes"][0] = UserModes[0];
    doc["pr_modes"][1] = UserModes[1];
    doc["pr_modes"][2] = UserModes[2];
    doc["pr_modes"][3] = UserModes[3];
    doc["pr_modes"][4] = UserModes[4];
    doc["pr_modes"][5] = UserModes[5];
    doc["pr_modes"][6] = UserModes[6];
    doc["pr_modes"][7] = UserModes[15];

    doc["mode_cmd_t"] = "~mode/set";
    doc["mode_stat_t"] = "~mode/state";
    doc["mode_stat_tpl"] = "{{ value_json.m }}";
    doc["modes"][0] = "off";
    doc["modes"][1] = "auto";

    doc["temp_cmd_t"] = "~temp/set";
    doc["temp_stat_t"] = "~temp/state";
    doc["temp_stat_tpl"] = "{{ value_json.sp_s }}";

    doc["curr_temp_t"] = "~temp/state";
    doc["curr_temp_tpl"] = "{{ value_json.sa }}";

    doc["fan_mode_cmd_t"] = "~fan/set";
    doc["fan_mode_stat_t"] = "~fan/state";
    doc["fan_mode_stat_tpl"] = "{{ value_json.m }}";
    doc["fan_modes"][0] = FanSpeeds[1];
    doc["fan_modes"][1] = FanSpeeds[2];
    doc["fan_modes"][2] = FanSpeeds[3];
    doc["fan_modes"][3] = FanSpeeds[4];

    String payload;
    serializeJson(doc, payload);
    doc.clear();

    String topic = String("homeassistant/climate/" + chipId + "/config");

    senseMQTT.publish(topic.c_str(), payload.c_str());
}

void publishDiscoveryFilter()
{
    String uniqId = String(chipId + "_filter_remaining").c_str();

    DynamicJsonDocument doc(1024);

    doc["name"] = "Ventilation filter remaining";
    doc["uniq_id"] = uniqId;
    doc["~"] = MQTTTopicString;

    doc["dev"]["name"] = "Ventilation";
    doc["dev"]["mdl"] = "Save VTR300/B";
    doc["dev"]["mf"] = "SystemAir";
    doc["dev"]["ids"][0] = chipId.c_str();
    doc["dev"]["sw"] = STR(VERSION);

    doc["stat_t"] = "~filter/state";
    doc["json_attr_t"] = "~filter/state";
    doc["val_tpl"] = "{{ value_json.p }}";
    doc["unit_of_meas"] = "%";
    doc["ic"] = "mdi:air-filter";

    String payload;
    serializeJson(doc, payload);
    doc.clear();

    String topic = String("homeassistant/sensor/" + uniqId + "/config");

    senseMQTT.publish(topic.c_str(), payload.c_str());
}

void publishDiscoveryOutdoorTemp()
{
    String uniqId = String(chipId + "_outdoor_temp").c_str();

    DynamicJsonDocument doc(1024);

    doc["name"] = "Ventilation outdoor temperature";
    doc["uniq_id"] = uniqId;
    doc["~"] = MQTTTopicString;

    doc["dev"]["name"] = "Ventilation";
    doc["dev"]["mdl"] = "Save VTR300/B";
    doc["dev"]["mf"] = "SystemAir";
    doc["dev"]["ids"][0] = chipId.c_str();
    doc["dev"]["sw"] = STR(VERSION);

    doc["stat_t"] = "~temp/state";
    doc["json_attr_t"] = "~temp/state";
    doc["val_tpl"] = "{{ value_json.oa | float }}";
    doc["dev_cla"] = "temperature";
    doc["unit_of_meas"] = "°C";
    doc["stat_cla"] = "measurement";

    String payload;
    serializeJson(doc, payload);
    doc.clear();

    String topic = String("homeassistant/sensor/" + uniqId + "/config");

    senseMQTT.publish(topic.c_str(), payload.c_str());
}

void publishDiscoverySupplyTemp()
{
    String uniqId = String(chipId + "_supply_temp").c_str();

    DynamicJsonDocument doc(1024);

    doc["name"] = "Ventilation supply air temperature";
    doc["uniq_id"] = uniqId;
    doc["~"] = MQTTTopicString;

    doc["dev"]["name"] = "Ventilation";
    doc["dev"]["mdl"] = "Save VTR300/B";
    doc["dev"]["mf"] = "SystemAir";
    doc["dev"]["ids"][0] = chipId.c_str();
    doc["dev"]["sw"] = STR(VERSION);

    doc["stat_t"] = "~temp/state";
    doc["json_attr_t"] = "~temp/state";
    doc["val_tpl"] = "{{ value_json.sa }}";
    doc["dev_cla"] = "temperature";
    doc["unit_of_meas"] = "°C";
    doc["stat_cla"] = "measurement";

    String payload;
    serializeJson(doc, payload);
    doc.clear();

    String topic = String("homeassistant/sensor/" + uniqId + "/config");

    senseMQTT.publish(topic.c_str(), payload.c_str());
}

void publishDiscoveryOutputSATC()
{
    String uniqId = String(chipId + "_output_satc").c_str();

    DynamicJsonDocument doc(1024);

    doc["name"] = "Ventilation output satc";
    doc["uniq_id"] = uniqId;
    doc["~"] = MQTTTopicString;

    doc["dev"]["name"] = "Ventilation";
    doc["dev"]["mdl"] = "Save VTR300/B";
    doc["dev"]["mf"] = "SystemAir";
    doc["dev"]["ids"][0] = chipId.c_str();
    doc["dev"]["sw"] = STR(VERSION);

    doc["stat_t"] = "~temp/state";
    doc["json_attr_t"] = "~temp/state";
    doc["val_tpl"] = "{{ value_json.o_s | float }}";
    doc["unit_of_meas"] = "%";
    doc["stat_cla"] = "measurement";

    String payload;
    serializeJson(doc, payload);
    doc.clear();

    String topic = String("homeassistant/sensor/" + uniqId + "/config");

    senseMQTT.publish(topic.c_str(), payload.c_str());
}

void publishDiscovery()
{
    if (senseWifi.connected() && senseMQTT.connected())
    {
        publishDiscoveryClimate();
        delay(100);
        publishDiscoveryFilter();
        delay(100);
        publishDiscoveryOutdoorTemp();
        delay(100);
        publishDiscoverySupplyTemp();
        delay(100);
        publishDiscoveryOutputSATC();
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
        // wf.loop();
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

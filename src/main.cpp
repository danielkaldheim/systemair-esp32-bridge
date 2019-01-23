/*

  RS485_HalfDuplex.pde - example using ModbusMaster library to communicate
  with EPSolar LS2024B controller using a half-duplex RS485 transceiver.

  This example is tested against an EPSolar LS2024B solar charge controller.
  See here for protocol specs:
  http://www.solar-elektro.cz/data/dokumenty/1733_modbus_protocol.pdf

  Library:: ModbusMaster
  Author:: Marius Kintel <marius at kintel dot net>

  Copyright:: 2009-2016 Doc Walker

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  Notes:
  https://github.com/4-20ma/ModbusMaster/issues/86
*/
#include <Arduino.h>
#include <ModbusMaster.h>
// #include <WiFi.h>
// #include <PubSubClient.h>

// #define MQTT_KEEPALIVE 30
// #define MQTTTopic "oregon/"
// #define MQTTled 0
// #define MQTTHost "mqtt.crudus.no"
// #define MQTTPort 1883
// #define MQTTUsername "daniel"
// #define MQTTPassword "BL100512916y"

/*!
  We're using a MAX485-compatible RS485 Transceiver.
  Rx/Tx is hooked up to the hardware serial port at 'Serial'.
  The Data Enable and Receiver Enable pins are hooked up as follows:
*/
#define MAX485_DE 16
#define MAX485_RE_NEG 17

// instantiate ModbusMaster object
ModbusMaster node;

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
    // pinMode(MAX485_RE_NEG, OUTPUT);
    // pinMode(MAX485_DE, OUTPUT);
    // // Init in receive mode
    // digitalWrite(MAX485_RE_NEG, 0);
    // digitalWrite(MAX485_DE, 0);

    // // Modbus communication runs at 9600 baud
    // Serial.begin(9600);

    // // Modbus slave ID 1
    // node.begin(1, Serial);
    // // Callbacks allow us to configure the RS485 transceiver correctly
    // node.preTransmission(preTransmission);
    // node.postTransmission(postTransmission);
}

// bool state = true;

void loop()
{
    // uint8_t result;

    // Toggle the coil at address 0x0002 (Manual Load Control)
    // result = node.writeSingleCoil(0x0002, state);
    // state = !state;

    // http://www.simplymodbus.ca/FAQ.htm
    // https://github.com/BeamCtrl/Airiana/blob/master/airiana-core.py
    // result = node.readInputRegisters(0x65, 6); // https://github.com/BeamCtrl/Airiana/blob/master/airiana-core.py#L740
    // if (result == node.ku8MBSuccess)
    // {
    //     // Serial.println(node.getResponseBuffer(0x00));
    //     // Serial.println(node.getResponseBuffer(0x01));
    //     // Serial.println(node.getResponseBuffer(0x02));
    //     // Serial.println(node.getResponseBuffer(0x03));
    //     // Serial.println(node.getResponseBuffer(0x04));
    //     // Serial.println(node.getResponseBuffer(0x05));

    //     // Serial.print("Vbatt: ");
    //     // Serial.println(node.getResponseBuffer(0x04) / 100.0f);
    //     // Serial.print("Vload: ");
    //     // Serial.println(node.getResponseBuffer(0xC0) / 100.0f);
    //     // Serial.print("Pload: ");
    //     // Serial.println((node.getResponseBuffer(0x0D) +
    //     //                     node.getResponseBuffer(0x0E)
    //     //                 << 16) /
    //     //                100.0f);
    // }

    delay(1000);
}

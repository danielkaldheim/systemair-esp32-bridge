/*
 * Climate.h
 *
 *  Created on: Oct 12, 2020
 *      Author: danielkaldheim
 */

#ifndef COMPONENTS_SYSTEMAIR_CLIMATE_H_
#define COMPONENTS_SYSTEMAIR_CLIMATE_H_
#include "SenseConfig.h"
#include "config.h"
#include <ArduinoJson.h>
#include <ModbusMaster.h>

static const char *SystemNames[] = {
    "VR400",
    "VR700",
    "VR700DK",
    "VR400DE",
    "VTC300",
    "VTC700",
    "VTR150K",
    "VTR200B",
    "VSR300",
    "VSR500",
    "VSR150",
    "VTR300",
    "VTR500",
    "VSR300DE",
    "VTC200",
    "VTC100"};

static const char *UserModes[] = {
    "Auto",
    "Manuell",
    "Gjester",
    "Refresh",
    "Peis",
    "Borte",
    "Ferie",
    "Cooker hood",
    "Vacuum cleaner",
    "CD1",
    "CD2",
    "CD3",
    "Pressure guard"};

static const char *RotorStates[] = {
    "Normal",
    "Rotor Fault",
    "Rotor Fault Detected",
    "Summer Mode transitioning",
    "Summer Mode",
    "Leaving Summer Mode",
    "Manual Summer Mode",
    "Rotor Cleaning in Summer Mode",
    "Rotor cleaning in manual summer mode",
    "Fans off",
    "Rotor Cleaning during fans off",
    "Rotor Fault, but conditions normal"};

static const char *FanSpeeds[] = {
    "undefined",
    "Av",
    "Lav",
    "Normal",
    "Høy",
};

static const char *Modes[] = {
    "off",
    "heat",
    "fan_only"};

class Climate
{
public:
    Climate();
    Climate(PAYLOAD_CALLBACK_SIGNATURE);
    virtual ~Climate();
    void begin(ModbusMaster &node);
    void loop();

    Climate &setCallback(PAYLOAD_CALLBACK_SIGNATURE);

    void getAirflow();
    void getFanSpeed();
    boolean setTargetTemperature(uint16_t value);
    void getTemperatures();
    void getModeState();
    void getSystemName();
    void getHeater();
    void getFilterStatus();
    void getRotorState();

    boolean setModeState(String value);
    boolean setPresetState(String value);
    boolean setFanSpeed(uint16_t value);
    boolean setFanSpeedString(String value);
    boolean setHeater(uint16_t value);
    boolean setExchangerMode(uint8_t value);

private:
    PAYLOAD_CALLBACK_SIGNATURE;
    ModbusMaster _node;

    unsigned long previousMillis = 0;
    unsigned long previousLessCriticalMillis = 0;
    const unsigned long interval = 1 * 1000;              // Every second
    const unsigned long lessCriticalinterval = 10 * 1000; // Every 10 second

    uint16_t airflowEf;
    uint16_t airflowSf;
    uint16_t fanSpeed;
    double targetTemperature;
    double temperatureExhaust;
    double temperatureExtract;
    double temperatureInlet;
    double temperaturePreSupply;
    double temperatureSupply;
    String mode;
    String systemName;
    uint16_t heater;
    double filterRemaining;
    int filterLimit;
    uint16_t filter;
    uint16_t exchangerMode;
    int exchangerSpeed;
    uint16_t rotorState;
    boolean rotorActive;

}; // Climate

#endif // COMPONENTS_SYSTEMAIR_CLIMATE_H_

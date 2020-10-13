#include "Climate.h"

Climate::Climate()
{
}

Climate::Climate(PAYLOAD_CALLBACK_SIGNATURE)
{
    setCallback(callback);
}

Climate::~Climate()
{
}

void Climate::begin(ModbusMaster &node)
{
    _node = node;

    mode = "fan_only";
    getHeater();
    delay(100);
    getSystemName();
    delay(100);
    getFanSpeed();
    delay(100);
    getTemperatures();
    delay(100);
}

void Climate::loop()
{
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval)
    {
        previousMillis += interval;
        getRotorState();
        delay(100);
        getFanSpeed();
        delay(100);
        getTemperatures();
        delay(100);
        getModeState();
    }
    if (currentMillis - previousLessCriticalMillis >= lessCriticalinterval)
    {
        previousLessCriticalMillis += lessCriticalinterval;
        getFilterStatus();
        delay(100);
        getSystemName();
    }
}

Climate &Climate::setCallback(PAYLOAD_CALLBACK_SIGNATURE)
{
    this->callback = callback;
    return *this;
}

void Climate::getAirflow()
{
    digitalWrite(ORANGE_LED, LOW);
    uint8_t result = _node.readInputRegisters(0x65, 6);
    if (result == _node.ku8MBSuccess)
    {
        uint16_t sf[3] = {_node.getResponseBuffer(0x00), _node.getResponseBuffer(0x01), _node.getResponseBuffer(0x02)};
        uint16_t ef[3] = {_node.getResponseBuffer(0x03), _node.getResponseBuffer(0x04), _node.getResponseBuffer(0x05)};
        uint16_t tmp = fanSpeed;
        if (tmp <= 0)
        {
            tmp = 1;
        }
        airflowSf = sf[tmp - 1];
        airflowEf = ef[tmp - 1];
        if (fanSpeed == 0)
        {
            airflowSf = 0;
            airflowEf = 0;
        }

        if (callback)
        {
            StaticJsonDocument<92> doc;

            doc["sf"] = airflowSf;
            doc["ef"] = airflowEf;

            String payload;
            serializeJson(doc, payload);
            doc.clear();

            callback("airflow/state", payload.c_str());
        }
    }
    digitalWrite(ORANGE_LED, HIGH);
}

void Climate::getFanSpeed()
{
    digitalWrite(ORANGE_LED, LOW);
    uint8_t result = _node.readInputRegisters(0x64, 1);
    if (result == _node.ku8MBSuccess)
    {
        uint16_t response = _node.getResponseBuffer(0x00);
        if (callback)
        {
            callback("fan/state", FanSpeeds[fanSpeed]);
        }
        fanSpeed = response;
    }
    digitalWrite(ORANGE_LED, HIGH);
}

boolean Climate::setFanSpeed(uint16_t value)
{
    digitalWrite(ORANGE_LED, LOW);
    if (value >= 0x04 || value < 0x00)
    {
        value = 0x00;
    }
    uint8_t result = _node.writeSingleRegister(0x64, value);

    if (result == _node.ku8MBSuccess)
    {
        if (fanSpeed == 0 && !rotorActive)
        {
            mode = "off";
        }
        else if (fanSpeed > 0 && !rotorActive)
        {
            mode = "fan_only";
        }
        else if (fanSpeed > 0 && rotorActive)
        {
            mode = "heat";
        }
        fanSpeed = value;
        getAirflow();
        digitalWrite(ORANGE_LED, HIGH);
        return true;
    }
    digitalWrite(ORANGE_LED, HIGH);
    return false;
}

boolean Climate::setFanSpeedString(String value)
{
    uint16_t target = 0;
    if (value == "Off")
    {
        target = 0x00;
    }
    else if (value == "Low")
    {
        target = 0x01;
    }
    else if (value == "Normal")
    {
        target = 0x02;
    }
    else if (value == "High")
    {
        target = 0x03;
    }
    return setFanSpeed(target);
}

void Climate::setTargetTemperature(double value)
{
    targetTemperature = value;
}

void Climate::getTemperatures()
{
    digitalWrite(ORANGE_LED, LOW);
    uint8_t result = _node.readInputRegisters(0xD5, 5);
    if (result == _node.ku8MBSuccess)
    {
        temperaturePreSupply = (double)_node.getResponseBuffer(0x00) / 10.0;
        temperatureExtract = (double)_node.getResponseBuffer(0x01) / 10.0;
        temperatureExhaust = (double)_node.getResponseBuffer(0x02) / 10.0;
        temperatureSupply = (double)_node.getResponseBuffer(0x03) / 10.0;
        temperatureInlet = (double)_node.getResponseBuffer(0x04) / 10.0;

        if (callback)
        {
            StaticJsonDocument<130> doc;

            doc["pre-supply"] = temperaturePreSupply;
            doc["extract"] = temperatureExtract;
            doc["exhaust"] = temperatureExhaust;
            doc["supply"] = temperatureSupply;
            doc["inlet"] = temperatureInlet;

            String payload;
            serializeJson(doc, payload);
            doc.clear();

            callback("temperature/state", payload.c_str());
        }
    }
    digitalWrite(ORANGE_LED, HIGH);
}

boolean Climate::setModeState(String value)
{
    mode = value;
    if (value == "heat")
    {
        if (exchangerMode == 0)
        {
            setExchangerMode(0x05);
        }
        if (fanSpeed == 0)
        {
            setFanSpeed(0x02);
        }
    }
    else if (value == "fan_only")
    {
        if (exchangerMode > 0)
        {
            setExchangerMode(0x00);
        }
        if (fanSpeed == 0)
        {
            setFanSpeed(0x02);
        }
    }
    else if (value == "off")
    {
        setExchangerMode(0x00);
        if (fanSpeed > 0)
        {
            setFanSpeed(0x00);
        }
    }
}

void Climate::getModeState()
{
    digitalWrite(ORANGE_LED, LOW);
    if (callback)
    {
        callback("mode/state", mode.c_str());
    }
    digitalWrite(ORANGE_LED, HIGH);
}

void Climate::getSystemName()
{
    digitalWrite(ORANGE_LED, LOW);
    uint8_t result = _node.readInputRegisters(0x1F4, 1);
    if (result == _node.ku8MBSuccess)
    {
        uint16_t value = _node.getResponseBuffer(0x00);
        systemName = String(SystemNames[value]);

        if (callback)
        {
            StaticJsonDocument<92> doc;

            doc["name"] = systemName;

            String payload;
            serializeJson(doc, payload);
            doc.clear();

            callback("system/state", payload.c_str());
        }
    }
    digitalWrite(ORANGE_LED, HIGH);
}

boolean Climate::setHeater(uint16_t value)
{
    digitalWrite(ORANGE_LED, LOW);
    uint8_t result = _node.writeSingleRegister(0xC8, value);
    if (result == _node.ku8MBSuccess)
    {
        heater = value;
        digitalWrite(ORANGE_LED, HIGH);
        return true;
    }
    return false;
    digitalWrite(ORANGE_LED, HIGH);
}

void Climate::getHeater()
{
    digitalWrite(ORANGE_LED, LOW);
    uint8_t result = _node.readInputRegisters(0xC8, 1);
    if (result == _node.ku8MBSuccess)
    {
        heater = _node.getResponseBuffer(0x00);
    }
    digitalWrite(ORANGE_LED, HIGH);
}

void Climate::getFilterStatus()
{
    digitalWrite(ORANGE_LED, LOW);
    uint8_t result = _node.readInputRegisters(0x258, 1);
    if (result == _node.ku8MBSuccess)
    {
        filterLimit = (int)_node.getResponseBuffer(0x00) * 31;

        result = _node.readInputRegisters(0x259, 1);
        if (result == _node.ku8MBSuccess)
        {
            filter = _node.getResponseBuffer(0x00);
            filterRemaining = round(100 * (1 - ((double)filter / filterLimit)));
            if (filterRemaining < 0)
            {
                filterRemaining = 0;
            }
        }

        if (callback)
        {
            StaticJsonDocument<50> doc;

            doc["limit"] = filterLimit;
            doc["remaining"] = filterRemaining;

            String payload;
            serializeJson(doc, payload);
            doc.clear();

            callback("filter/state", payload.c_str());
        }
    }
    digitalWrite(ORANGE_LED, HIGH);
}

boolean Climate::setExchangerMode(uint8_t value)
{
    digitalWrite(ORANGE_LED, LOW);
    uint8_t result = _node.writeSingleRegister(0xCE, value);
    if (result == _node.ku8MBSuccess)
    {
        heater = value;
        digitalWrite(ORANGE_LED, HIGH);
        return true;
    }
    return false;
    digitalWrite(ORANGE_LED, HIGH);
}

void Climate::getRotorState()
{
    digitalWrite(ORANGE_LED, LOW);
    uint8_t result = _node.readInputRegisters(0xCE, 1);
    if (result == _node.ku8MBSuccess)
    {
        exchangerMode = _node.getResponseBuffer(0x00);
    }

    result = _node.readInputRegisters(0x15E, 2);
    if (result == _node.ku8MBSuccess)
    {
        rotorState = _node.getResponseBuffer(0x00);
        if (_node.getResponseBuffer(0x01))
        {
            mode = "heat";
            rotorActive = true;
            exchangerSpeed = 100;
        }
        else
        {
            rotorActive = false;
            if (fanSpeed > 0)
            {
                mode = "fan_only";
            }
            else
            {
                mode = "off";
            }
        }

        if (callback)
        {
            StaticJsonDocument<200> doc;

            doc["rotorState"] = RotorStates[rotorState];
            doc["rotorActive"] = rotorActive;
            doc["exchangerMode"] = exchangerMode;
            doc["exchangerSpeed"] = exchangerSpeed;

            String payload;
            serializeJson(doc, payload);
            doc.clear();

            callback("rotor/state", payload.c_str());
        }
    }
    digitalWrite(ORANGE_LED, HIGH);
}

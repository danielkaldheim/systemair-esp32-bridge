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

    mode = "off";
    // getHeater();
    // delay(100);
    // getSystemName();
    delay(100);
    getFanSpeed();
    delay(100);
    getTemperatures();
    delay(100);
    getModeState();
}

void Climate::loop()
{
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval)
    {
        previousMillis += interval;
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
        // delay(100);
        // getSystemName();
    }
}

Climate &Climate::setCallback(PAYLOAD_CALLBACK_SIGNATURE)
{
    this->callback = callback;
    return *this;
}

void Climate::getAirflow()
{
    // digitalWrite(ORANGE_LED, LOW);
    // uint8_t result = _node.readInputRegisters(0x65, 6);
    // if (result == _node.ku8MBSuccess)
    // {
    //     uint16_t sf[3] = {_node.getResponseBuffer(0x00), _node.getResponseBuffer(0x01), _node.getResponseBuffer(0x02)};
    //     uint16_t ef[3] = {_node.getResponseBuffer(0x03), _node.getResponseBuffer(0x04), _node.getResponseBuffer(0x05)};
    //     uint16_t tmp = fanSpeed;
    //     if (tmp <= 0)
    //     {
    //         tmp = 1;
    //     }
    //     airflowSf = sf[tmp - 1];
    //     airflowEf = ef[tmp - 1];
    //     if (fanSpeed == 0)
    //     {
    //         airflowSf = 0;
    //         airflowEf = 0;
    //     }

    //     if (callback)
    //     {
    //         StaticJsonDocument<92> doc;

    //         doc["sf"] = airflowSf;
    //         doc["ef"] = airflowEf;

    //         String payload;
    //         serializeJson(doc, payload);
    //         doc.clear();

    //         callback("airflow/state", payload.c_str());
    //     }
    // }
    // else
    // {
    //     digitalWrite(BUILTIN_LED, LOW);
    //     if (callback)
    //     {
    //         StaticJsonDocument<92> doc;

    //         if (result == _node.ku8MBIllegalDataAddress)
    //         {
    //             doc["error"] = "Illegal data address";
    //         }
    //         else if (result == _node.ku8MBIllegalDataValue)
    //         {
    //             doc["error"] = "Illegal data value";
    //         }
    //         else if (result == _node.ku8MBIllegalFunction)
    //         {
    //             doc["error"] = "Illegal function";
    //         }
    //         else if (result == _node.ku8MBInvalidCRC)
    //         {
    //             doc["error"] = "Invalid CRC";
    //         }
    //         else if (result == _node.ku8MBInvalidFunction)
    //         {
    //             doc["error"] = "Invalid function";
    //         }
    //         else if (result == _node.ku8MBInvalidSlaveID)
    //         {
    //             doc["error"] = "Invalid Slave ID";
    //         }
    //         else if (result == _node.ku8MBSlaveDeviceFailure)
    //         {
    //             doc["error"] = "Slave device Failure";
    //         }
    //         else if (result == _node.ku8MBResponseTimedOut)
    //         {
    //             doc["error"] = "Response timed out";
    //         }
    //         else
    //         {
    //             doc["error"] = "Unknown error";
    //         }
    //         doc["code"] = result;

    //         String payload;
    //         serializeJson(doc, payload);
    //         doc.clear();

    //         callback("airflow/error", payload.c_str());
    //     }
    //     digitalWrite(BUILTIN_LED, HIGH);
    // }

    // digitalWrite(ORANGE_LED, HIGH);
}

void Climate::getFanSpeed()
{
    digitalWrite(ORANGE_LED, LOW);
    uint8_t result = _node.readInputRegisters(0x46A, 2);
    if (result == _node.ku8MBSuccess)
    {
        if (callback)
        {
            StaticJsonDocument<130> doc;

            int saf = _node.getResponseBuffer(0x00);
            doc["saf"] = saf;
            doc["eaf"] = _node.getResponseBuffer(0x01);
            doc["m"] = FanSpeeds[saf];

            String payload;
            serializeJson(doc, payload);
            doc.clear();

            callback("fan/state", payload.c_str());
        }
    }
    else
    {
        digitalWrite(BUILTIN_LED, LOW);
        if (callback)
        {
            StaticJsonDocument<92> doc;

            if (result == _node.ku8MBIllegalDataAddress)
            {
                doc["error"] = "Illegal data address";
            }
            else if (result == _node.ku8MBIllegalDataValue)
            {
                doc["error"] = "Illegal data value";
            }
            else if (result == _node.ku8MBIllegalFunction)
            {
                doc["error"] = "Illegal function";
            }
            else if (result == _node.ku8MBInvalidCRC)
            {
                doc["error"] = "Invalid CRC";
            }
            else if (result == _node.ku8MBInvalidFunction)
            {
                doc["error"] = "Invalid function";
            }
            else if (result == _node.ku8MBInvalidSlaveID)
            {
                doc["error"] = "Invalid Slave ID";
            }
            else if (result == _node.ku8MBSlaveDeviceFailure)
            {
                doc["error"] = "Slave device Failure";
            }
            else if (result == _node.ku8MBResponseTimedOut)
            {
                doc["error"] = "Response timed out";
            }
            else
            {
                doc["error"] = "Unknown error";
            }
            doc["code"] = result;

            String payload;
            serializeJson(doc, payload);
            doc.clear();

            callback("fan/error", payload.c_str());
        }
        digitalWrite(BUILTIN_LED, HIGH);
    }
    digitalWrite(ORANGE_LED, HIGH);
}

boolean Climate::setFanSpeed(uint16_t value)
{
    digitalWrite(ORANGE_LED, LOW);
    uint8_t result = _node.writeSingleRegister(0x46A, value);
    uint8_t result2 = _node.writeSingleRegister(0x46B, value);
    if (result == _node.ku8MBSuccess && result2 == _node.ku8MBSuccess)
    {
        digitalWrite(ORANGE_LED, HIGH);
        getFanSpeed();
        return true;
    }
    else
    {
        if (callback)
        {
            StaticJsonDocument<92> doc;

            if (result == _node.ku8MBIllegalDataAddress || result2 == _node.ku8MBIllegalDataAddress)
            {
                doc["error"] = "Illegal data address";
            }
            else if (result == _node.ku8MBIllegalDataValue || result2 == _node.ku8MBIllegalDataValue)
            {
                doc["error"] = "Illegal data value";
            }
            else if (result == _node.ku8MBIllegalFunction || result2 == _node.ku8MBIllegalFunction)
            {
                doc["error"] = "Illegal function";
            }
            else if (result == _node.ku8MBInvalidCRC || result2 == _node.ku8MBInvalidCRC)
            {
                doc["error"] = "Invalid CRC";
            }
            else if (result == _node.ku8MBInvalidFunction || result2 == _node.ku8MBInvalidFunction)
            {
                doc["error"] = "Invalid function";
            }
            else if (result == _node.ku8MBInvalidSlaveID || result2 == _node.ku8MBInvalidSlaveID)
            {
                doc["error"] = "Invalid Slave ID";
            }
            else if (result == _node.ku8MBSlaveDeviceFailure || result2 == _node.ku8MBSlaveDeviceFailure)
            {
                doc["error"] = "Slave device Failure";
            }
            else if (result == _node.ku8MBResponseTimedOut || result2 == _node.ku8MBResponseTimedOut)
            {
                doc["error"] = "Response timed out";
            }
            else
            {
                doc["error"] = "Unknown error";
            }
            doc["code"] = result;

            String payload;
            serializeJson(doc, payload);
            doc.clear();

            callback("fan/error", payload.c_str());
        }
    }
    digitalWrite(ORANGE_LED, HIGH);
    return false;
}

boolean Climate::setFanSpeedString(String value)
{
    for (unsigned int i = 0; i < sizeof(FanSpeeds); i++)
    {
        if (value == FanSpeeds[i])
        {
            return setFanSpeed(i);
        }
    }
    return false;
}

boolean Climate::setTargetTemperature(uint16_t value)
{
    digitalWrite(ORANGE_LED, LOW);
    uint8_t result = _node.writeSingleRegister(0x7D0, value);
    if (result == _node.ku8MBSuccess)
    {
        digitalWrite(ORANGE_LED, HIGH);
        getTemperatures();
        return true;
    }
    else
    {
        if (callback)
        {
            StaticJsonDocument<92> doc;

            if (result == _node.ku8MBIllegalDataAddress)
            {
                doc["error"] = "Illegal data address";
            }
            else if (result == _node.ku8MBIllegalDataValue)
            {
                doc["error"] = "Illegal data value";
            }
            else if (result == _node.ku8MBIllegalFunction)
            {
                doc["error"] = "Illegal function";
            }
            else if (result == _node.ku8MBInvalidCRC)
            {
                doc["error"] = "Invalid CRC";
            }
            else if (result == _node.ku8MBInvalidFunction)
            {
                doc["error"] = "Invalid function";
            }
            else if (result == _node.ku8MBInvalidSlaveID)
            {
                doc["error"] = "Invalid Slave ID";
            }
            else if (result == _node.ku8MBSlaveDeviceFailure)
            {
                doc["error"] = "Slave device Failure";
            }
            else if (result == _node.ku8MBResponseTimedOut)
            {
                doc["error"] = "Response timed out";
            }
            else
            {
                doc["error"] = "Unknown error";
            }
            doc["code"] = result;

            String payload;
            serializeJson(doc, payload);
            doc.clear();

            callback("temp/error", payload.c_str());
        }
    }
    digitalWrite(ORANGE_LED, HIGH);
    return false;
}

void Climate::getTemperatures()
{
    digitalWrite(ORANGE_LED, LOW);
    uint8_t result = _node.readInputRegisters(0x2F44, 3);
    if (result == _node.ku8MBSuccess)
    {
        // temperaturePreSupply = (double)_node.getResponseBuffer(0x00) / 10.0;
        // temperatureExtract = (double)_node.getResponseBuffer(0x01) / 10.0;
        // temperatureExhaust = (double)_node.getResponseBuffer(0x02) / 10.0;
        // temperatureSupply = (double)_node.getResponseBuffer(0x03) / 10.0;
        // temperatureInlet = (double)_node.getResponseBuffer(0x04) / 10.0;

        if (callback)
        {
            StaticJsonDocument<130> doc;

            // doc["fp"] = (double)_node.getResponseBuffer(0x00) / 10.0; // frostProtection
            doc["oa"] = (double)_node.getResponseBuffer(0x01) / 10.0; // outdoorAir
            doc["sa"] = (double)_node.getResponseBuffer(0x02) / 10.0; // supplyAir
            // doc["ra"] = (double)_node.getResponseBuffer(0x03) / 10.0; // roomAir
            // doc["ea"] = (double)_node.getResponseBuffer(0x04) / 10.0; // extractAir

            uint8_t result = _node.readInputRegisters(0x2F49, 3);
            if (result == _node.ku8MBSuccess)
            {
                // doc["ec"] = (double)_node.getResponseBuffer(0x00) / 10.0; // extraController
                // doc["e"] = (double)_node.getResponseBuffer(0x01) / 10.0; // efficiency
                doc["oh"] = (double)_node.getResponseBuffer(0x02) / 10.0; // overHeat
            }

            uint8_t result2 = _node.readInputRegisters(0x7D1, 1);
            if (result2 == _node.ku8MBSuccess)
            {
                doc["sp"] = (double)_node.getResponseBuffer(0x00) / 10.0; // setpoint
            }

            uint8_t result3 = _node.readInputRegisters(0x805, 2);
            if (result3 == _node.ku8MBSuccess)
            {
                doc["sp_s"] = (double)_node.getResponseBuffer(0x00) / 10.0; // setpoint satc
                doc["o_s"] = (double)_node.getResponseBuffer(0x01);         // Output of the SATC 0-100%
            }

            uint8_t result4 = _node.readInputRegisters(0x80D, 1);
            if (result4 == _node.ku8MBSuccess)
            {
                doc["sa_s"] = (double)_node.getResponseBuffer(0x00) / 10.0; // SATC setpoint value
            }

            // doc["pre-supply"] = temperaturePreSupply;
            // doc["extract"] = temperatureExtract;
            // doc["exhaust"] = temperatureExhaust;
            // doc["supply"] = temperatureSupply;
            // doc["inlet"] = temperatureInlet;

            String payload;
            serializeJson(doc, payload);
            doc.clear();

            callback("temp/state", payload.c_str());
        }
    }
    else
    {
        if (callback)
        {
            StaticJsonDocument<92> doc;

            if (result == _node.ku8MBIllegalDataAddress)
            {
                doc["error"] = "Illegal data address";
            }
            else if (result == _node.ku8MBIllegalDataValue)
            {
                doc["error"] = "Illegal data value";
            }
            else if (result == _node.ku8MBIllegalFunction)
            {
                doc["error"] = "Illegal function";
            }
            else if (result == _node.ku8MBInvalidCRC)
            {
                doc["error"] = "Invalid CRC";
            }
            else if (result == _node.ku8MBInvalidFunction)
            {
                doc["error"] = "Invalid function";
            }
            else if (result == _node.ku8MBInvalidSlaveID)
            {
                doc["error"] = "Invalid Slave ID";
            }
            else if (result == _node.ku8MBSlaveDeviceFailure)
            {
                doc["error"] = "Slave device Failure";
            }
            else if (result == _node.ku8MBResponseTimedOut)
            {
                doc["error"] = "Response timed out";
            }
            else
            {
                doc["error"] = "Unknown error";
            }
            doc["code"] = result;

            String payload;
            serializeJson(doc, payload);
            doc.clear();

            callback("temp/error", payload.c_str());
        }
    }
    digitalWrite(ORANGE_LED, HIGH);
}

boolean Climate::setPresetState(String value)
{

    for (unsigned int i = 0; i < sizeof(UserModes); i++)
    {
        if (value == UserModes[i])
        {
            unsigned int preset = i + 1;

            digitalWrite(ORANGE_LED, LOW);
            uint8_t result = _node.writeSingleRegister(0x489, preset);
            if (result == _node.ku8MBSuccess)
            {
                digitalWrite(ORANGE_LED, HIGH);
                getModeState();
                return true;
            }
            else
            {
                if (callback)
                {
                    StaticJsonDocument<92> doc;

                    if (result == _node.ku8MBIllegalDataAddress)
                    {
                        doc["error"] = "Illegal data address";
                    }
                    else if (result == _node.ku8MBIllegalDataValue)
                    {
                        doc["error"] = "Illegal data value";
                    }
                    else if (result == _node.ku8MBIllegalFunction)
                    {
                        doc["error"] = "Illegal function";
                    }
                    else if (result == _node.ku8MBInvalidCRC)
                    {
                        doc["error"] = "Invalid CRC";
                    }
                    else if (result == _node.ku8MBInvalidFunction)
                    {
                        doc["error"] = "Invalid function";
                    }
                    else if (result == _node.ku8MBInvalidSlaveID)
                    {
                        doc["error"] = "Invalid Slave ID";
                    }
                    else if (result == _node.ku8MBSlaveDeviceFailure)
                    {
                        doc["error"] = "Slave device Failure";
                    }
                    else if (result == _node.ku8MBResponseTimedOut)
                    {
                        doc["error"] = "Response timed out";
                    }
                    else
                    {
                        doc["error"] = "Unknown error";
                    }
                    doc["code"] = result;

                    String payload;
                    serializeJson(doc, payload);
                    doc.clear();

                    callback("preset/error", payload.c_str());
                }
            }
            digitalWrite(ORANGE_LED, HIGH);
            return false;
        }
    }

    return false;
}

boolean Climate::setModeState(String value)
{
    // mode = value;
    // if (value == "heat")
    // {
    //     if (exchangerMode == 0)
    //     {
    //         setExchangerMode(0x05);
    //     }
    //     if (fanSpeed == 0)
    //     {
    //         setFanSpeed(0x02);
    //     }
    // }
    // else if (value == "fan_only")
    // {
    //     if (exchangerMode > 0)
    //     {
    //         setExchangerMode(0x00);
    //     }
    //     if (fanSpeed == 0)
    //     {
    //         setFanSpeed(0x02);
    //     }
    // }
    // else if (value == "off")
    // {
    //     setExchangerMode(0x00);
    //     if (fanSpeed > 0)
    //     {
    //         setFanSpeed(0x00);
    //     }
    // }
    return false;
}

void Climate::getModeState()
{
    digitalWrite(ORANGE_LED, LOW);
    uint8_t result = _node.readInputRegisters(0x488, 1);
    if (result == _node.ku8MBSuccess)
    {
        if (callback)
        {
            StaticJsonDocument<92> doc;
            uint16_t value = _node.getResponseBuffer(0x00);
            doc["um"] = String(UserModes[value]);

            doc["val"] = value;

            switch (value)
            {
            case 0:
                mode = "auto";
                break;
                // case 1:
                //     mode = "heat_cool";
                //     break;

            default:
                mode = "auto";
                break;
            }

            doc["m"] = mode;

            String payload;
            serializeJson(doc, payload);
            doc.clear();

            callback("mode/state", payload.c_str());
            // callback("user_mode/state", mode.c_str());
            // callback("mode/state", mode.c_str());
        }
    }
    else
    {
        digitalWrite(BUILTIN_LED, LOW);
        if (callback)
        {
            StaticJsonDocument<92> doc;

            if (result == _node.ku8MBIllegalDataAddress)
            {
                doc["error"] = "Illegal data address";
            }
            else if (result == _node.ku8MBIllegalDataValue)
            {
                doc["error"] = "Illegal data value";
            }
            else if (result == _node.ku8MBIllegalFunction)
            {
                doc["error"] = "Illegal function";
            }
            else if (result == _node.ku8MBInvalidCRC)
            {
                doc["error"] = "Invalid CRC";
            }
            else if (result == _node.ku8MBInvalidFunction)
            {
                doc["error"] = "Invalid function";
            }
            else if (result == _node.ku8MBInvalidSlaveID)
            {
                doc["error"] = "Invalid Slave ID";
            }
            else if (result == _node.ku8MBSlaveDeviceFailure)
            {
                doc["error"] = "Slave device Failure";
            }
            else if (result == _node.ku8MBResponseTimedOut)
            {
                doc["error"] = "Response timed out";
            }
            else
            {
                doc["error"] = "Unknown error";
            }
            doc["code"] = result;

            String payload;
            serializeJson(doc, payload);
            doc.clear();

            callback("mode/error", payload.c_str());
        }
        digitalWrite(BUILTIN_LED, HIGH);
    }
    digitalWrite(ORANGE_LED, HIGH);
}

void Climate::getSystemName()
{
    // digitalWrite(ORANGE_LED, LOW);
    // uint8_t result = _node.readInputRegisters(0x1F4, 1);
    // if (result == _node.ku8MBSuccess)
    // {
    //     uint16_t value = _node.getResponseBuffer(0x00);
    //     systemName = String(SystemNames[value]);

    //     if (callback)
    //     {
    //         StaticJsonDocument<92> doc;

    //         doc["name"] = systemName;
    //         doc["value"] = value;

    //         String payload;
    //         serializeJson(doc, payload);
    //         doc.clear();

    //         callback("system/state", payload.c_str());
    //     }
    // }
    // else
    // {
    //     digitalWrite(BUILTIN_LED, LOW);
    //     if (callback)
    //     {
    //         StaticJsonDocument<92> doc;

    //         if (result == _node.ku8MBIllegalDataAddress)
    //         {
    //             doc["error"] = "Illegal data address";
    //         }
    //         else if (result == _node.ku8MBIllegalDataValue)
    //         {
    //             doc["error"] = "Illegal data value";
    //         }
    //         else if (result == _node.ku8MBIllegalFunction)
    //         {
    //             doc["error"] = "Illegal function";
    //         }
    //         else if (result == _node.ku8MBInvalidCRC)
    //         {
    //             doc["error"] = "Invalid CRC";
    //         }
    //         else if (result == _node.ku8MBInvalidFunction)
    //         {
    //             doc["error"] = "Invalid function";
    //         }
    //         else if (result == _node.ku8MBInvalidSlaveID)
    //         {
    //             doc["error"] = "Invalid Slave ID";
    //         }
    //         else if (result == _node.ku8MBSlaveDeviceFailure)
    //         {
    //             doc["error"] = "Slave device Failure";
    //         }
    //         else if (result == _node.ku8MBResponseTimedOut)
    //         {
    //             doc["error"] = "Response timed out";
    //         }
    //         else
    //         {
    //             doc["error"] = "Unknown error";
    //         }
    //         doc["code"] = result;

    //         String payload;
    //         serializeJson(doc, payload);
    //         doc.clear();

    //         callback("system/error", payload.c_str());
    //     }
    //     digitalWrite(BUILTIN_LED, HIGH);
    // }
    // digitalWrite(ORANGE_LED, HIGH);
}

boolean Climate::setHeater(uint16_t value)
{
    digitalWrite(ORANGE_LED, LOW);
    // uint8_t result = _node.writeSingleRegister(0xC8, value);
    // if (result == _node.ku8MBSuccess)
    // {
    //     heater = value;
    //     digitalWrite(ORANGE_LED, HIGH);
    //     return true;
    // }
    digitalWrite(ORANGE_LED, HIGH);
    // return false;
    return true;
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
    uint8_t result = _node.readInputRegisters(0x1B58, 1);
    if (result == _node.ku8MBSuccess)
    {
        filterLimit = (int)_node.getResponseBuffer(0x00) * 31;

        result = _node.readInputRegisters(0x1B5C, 2);
        if (result == _node.ku8MBSuccess)
        {
            uint16_t filterRemainingLow = _node.getResponseBuffer(0x00);
            uint16_t filterRemainingHigh = _node.getResponseBuffer(0x01);

            uint32_t combined = (filterRemainingHigh << 16) | filterRemainingLow;
            double daysRemaining = combined / 86400; // Get days
            filterRemaining = round(100 * (double)daysRemaining / filterLimit);

            if (filterRemaining < 0)
            {
                filterRemaining = 0;
            }
            else if (filterRemaining > 100)
            {
                filterRemaining = 100;
            }

            if (callback)
            {
                StaticJsonDocument<50> doc;

                doc["l"] = filterLimit;
                doc["d"] = daysRemaining;
                doc["p"] = filterRemaining;

                String payload;
                serializeJson(doc, payload);
                doc.clear();

                callback("filter/state", payload.c_str());
            }
        }
    }
    else
    {
        digitalWrite(BUILTIN_LED, LOW);
        if (callback)
        {
            StaticJsonDocument<92> doc;

            if (result == _node.ku8MBIllegalDataAddress)
            {
                doc["error"] = "Illegal data address";
            }
            else if (result == _node.ku8MBIllegalDataValue)
            {
                doc["error"] = "Illegal data value";
            }
            else if (result == _node.ku8MBIllegalFunction)
            {
                doc["error"] = "Illegal function";
            }
            else if (result == _node.ku8MBInvalidCRC)
            {
                doc["error"] = "Invalid CRC";
            }
            else if (result == _node.ku8MBInvalidFunction)
            {
                doc["error"] = "Invalid function";
            }
            else if (result == _node.ku8MBInvalidSlaveID)
            {
                doc["error"] = "Invalid Slave ID";
            }
            else if (result == _node.ku8MBSlaveDeviceFailure)
            {
                doc["error"] = "Slave device Failure";
            }
            else if (result == _node.ku8MBResponseTimedOut)
            {
                doc["error"] = "Response timed out";
            }
            else
            {
                doc["error"] = "Unknown error";
            }
            doc["code"] = result;

            String payload;
            serializeJson(doc, payload);
            doc.clear();

            callback("filter/error", payload.c_str());
        }
        digitalWrite(BUILTIN_LED, HIGH);
    }
    digitalWrite(ORANGE_LED, HIGH);
}

boolean Climate::setExchangerMode(uint8_t value)
{
    digitalWrite(ORANGE_LED, LOW);
    // uint8_t result = _node.writeSingleRegister(0xCE, value);
    // if (result == _node.ku8MBSuccess)
    // {
    //     heater = value;
    //     digitalWrite(ORANGE_LED, HIGH);
    //     return true;
    // }
    digitalWrite(ORANGE_LED, HIGH);
    // return false;
    return true;
}

void Climate::getRotorState()
{
    // digitalWrite(ORANGE_LED, LOW);
    // uint8_t result = _node.readInputRegisters(0xCE, 1);
    // if (result == _node.ku8MBSuccess)
    // {
    //     exchangerMode = _node.getResponseBuffer(0x00);
    // }

    // result = _node.readInputRegisters(0x15E, 2);
    // if (result == _node.ku8MBSuccess)
    // {
    //     rotorState = _node.getResponseBuffer(0x00);
    //     if (_node.getResponseBuffer(0x01))
    //     {
    //         // mode = "heat";
    //         rotorActive = true;
    //         exchangerSpeed = 100;
    //     }
    //     else
    //     {
    //         rotorActive = false;
    //         // if (fanSpeed > 0)
    //         // {
    //         //     mode = "fan_only";
    //         // }
    //         // else
    //         // {
    //         //     mode = "off";
    //         // }
    //     }

    //     if (callback)
    //     {
    //         StaticJsonDocument<200> doc;

    //         doc["rotorState"] = RotorStates[rotorState];
    //         doc["rotorActive"] = rotorActive;
    //         doc["exchangerMode"] = exchangerMode;
    //         doc["exchangerSpeed"] = exchangerSpeed;

    //         String payload;
    //         serializeJson(doc, payload);
    //         doc.clear();

    //         callback("rotor/state", payload.c_str());
    //     }
    // }
    // else
    // {
    //     digitalWrite(BUILTIN_LED, LOW);
    //     if (callback)
    //     {
    //         StaticJsonDocument<92> doc;

    //         if (result == _node.ku8MBIllegalDataAddress)
    //         {
    //             doc["error"] = "Illegal data address";
    //         }
    //         else if (result == _node.ku8MBIllegalDataValue)
    //         {
    //             doc["error"] = "Illegal data value";
    //         }
    //         else if (result == _node.ku8MBIllegalFunction)
    //         {
    //             doc["error"] = "Illegal function";
    //         }
    //         else if (result == _node.ku8MBInvalidCRC)
    //         {
    //             doc["error"] = "Invalid CRC";
    //         }
    //         else if (result == _node.ku8MBInvalidFunction)
    //         {
    //             doc["error"] = "Invalid function";
    //         }
    //         else if (result == _node.ku8MBInvalidSlaveID)
    //         {
    //             doc["error"] = "Invalid Slave ID";
    //         }
    //         else if (result == _node.ku8MBSlaveDeviceFailure)
    //         {
    //             doc["error"] = "Slave device Failure";
    //         }
    //         else if (result == _node.ku8MBResponseTimedOut)
    //         {
    //             doc["error"] = "Response timed out";
    //         }
    //         else
    //         {
    //             doc["error"] = "Unknown error";
    //         }
    //         doc["code"] = result;

    //         String payload;
    //         serializeJson(doc, payload);
    //         doc.clear();

    //         callback("rotor/error", payload.c_str());
    //     }
    //     digitalWrite(BUILTIN_LED, HIGH);
    // }
    // digitalWrite(ORANGE_LED, HIGH);
}

uint32_t combined(uint16_t low, uint16_t high)
{
    return (static_cast<uint32_t>(high) << 16) | ((static_cast<uint32_t>(low)) & 0xFFFF);
}
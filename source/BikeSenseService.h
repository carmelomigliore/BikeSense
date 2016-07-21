/* mbed Microcontroller Library
 * Copyright (c) 2006-2013 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __BLE_BIKESENSE_SERVICE_H__
#define __BLE_BIKESENSE_SERVICE_H__
#include "mbed-drivers/mbed.h"


class BikeSenseService {
public:
    const static uint16_t BIKESENSE_SERVICE_UUID = 0xA000;
    const static uint16_t GAS_VALUE_CHARACTERISTIC_UUID = 0xA001;
    const static uint16_t TEMP_VALUE_CHARACTERISTIC_UUID = 0xA002;
    const static uint16_t HUMIDITY_VALUE_CHARACTERISTIC_UUID = 0xA003;
    const static uint16_t BATTERY_VALUE_CHARACTERISTIC_UUID = 0xA004;
    const static uint16_t COMMAND_CHARACTERISTIC_UUID = 0xA005;
    const static uint16_t POTHOLE_CHARACTERISTIC_UUID = 0xA006;
    const static uint16_t ANTITHEFT_CHARACTERISTIC_UUID = 0xA007;
    const static uint16_t KEY_CHARACTERISTIC_UUID = 0xA008;
    const static uint16_t ALARM_CHARACTERISTIC_UUID = 0xA009;
    const static uint16_t MQTT_CHARACTERISTIC_UUID = 0xA00A;

    BikeSenseService(BLEDevice &_ble, uint16_t initialValueForGASCharacteristic, int16_t initialValueForTEMPCharacteristic, uint16_t initialValueForHUMIDITYCharacteristic, 
		uint16_t initialValueForBATTERYCharacteristic, uint16_t initialValueForCOMMANDCharacteristic, uint8_t initialValueForPOTHOLECharacteristic, uint8_t initialValueForANTITHEFTCharacteristic, uint8_t initialValueForALARMCharacteristic, uint8_t initialValueForMQTTCharacteristic) :
        ble(_ble), gasValue(GAS_VALUE_CHARACTERISTIC_UUID, &initialValueForGASCharacteristic), tempValue(TEMP_VALUE_CHARACTERISTIC_UUID, &initialValueForTEMPCharacteristic),
	humidityValue(HUMIDITY_VALUE_CHARACTERISTIC_UUID, &initialValueForHUMIDITYCharacteristic), batteryValue(BATTERY_VALUE_CHARACTERISTIC_UUID, &initialValueForBATTERYCharacteristic),
	command(COMMAND_CHARACTERISTIC_UUID, &initialValueForCOMMANDCharacteristic), pothole(POTHOLE_CHARACTERISTIC_UUID, &initialValueForPOTHOLECharacteristic, GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY), 
	antitheft(ANTITHEFT_CHARACTERISTIC_UUID ,&initialValueForANTITHEFTCharacteristic, GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY), key(KEY_CHARACTERISTIC_UUID, initialkey), alarm(ALARM_CHARACTERISTIC_UUID, &initialValueForALARMCharacteristic, GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY), mqtt(MQTT_CHARACTERISTIC_UUID, &initialValueForMQTTCharacteristic)
    {
        GattCharacteristic *charTable[] = {&gasValue, &tempValue, &humidityValue, &batteryValue, &command, &pothole, &antitheft, &key, &alarm, &mqtt};
        GattService         bikeSenseService(BIKESENSE_SERVICE_UUID, charTable, sizeof(charTable) / sizeof(GattCharacteristic *));
        ble.addService(bikeSenseService);
    }

    GattAttribute::Handle_t getCommandHandle() const {
        return command.getValueHandle();
    }
    
    GattAttribute::Handle_t getKeyHandle() const {
    	return key.getValueHandle();
    }

    void updateGasValue(uint16_t newValue) {
        ble.gattServer().write(gasValue.getValueHandle(), 
            (uint8_t *)&newValue, sizeof(uint16_t));
    }

    void updateTempValue(int16_t newValue) {
        ble.gattServer().write(tempValue.getValueHandle(), 
            (uint8_t *)&newValue, sizeof(int16_t));
    }

    void updateHumidityValue(uint16_t newValue) {
        ble.gattServer().write(humidityValue.getValueHandle(), 
            (uint8_t *)&newValue, sizeof(uint16_t));
    }

    void updateBatteryValue(uint16_t newValue) {
        ble.gattServer().write(batteryValue.getValueHandle(), 
            (uint8_t *)&newValue, sizeof(uint16_t));
    }
    
    void updatePotholeValue(uint8_t newValue) {
        ble.updateCharacteristicValue(pothole.getValueHandle(), 
            (uint8_t *)&newValue, sizeof(uint8_t));
    }
    
    void updateAntitheftValue(uint8_t newValue) {
        ble.updateCharacteristicValue(antitheft.getValueHandle(), 
            (uint8_t *)&newValue, sizeof(uint8_t));
    }
    
    void updateAlarmValue(uint8_t newValue) {
        ble.updateCharacteristicValue(alarm.getValueHandle(), 
            (uint8_t *)&newValue, sizeof(uint8_t));
    }
    
    void updateMqttValue(uint8_t newValue) {
        ble.updateCharacteristicValue(mqtt.getValueHandle(), 
            (uint8_t *)&newValue, sizeof(uint8_t));
    }

private:
    BLEDevice                         &ble;
    ReadOnlyGattCharacteristic<uint16_t>  gasValue;
    ReadOnlyGattCharacteristic<int16_t>  tempValue;
    ReadOnlyGattCharacteristic<uint16_t>  humidityValue;
    ReadOnlyGattCharacteristic<uint16_t>  batteryValue;
    ReadWriteGattCharacteristic<uint16_t>  command;
    ReadOnlyGattCharacteristic<uint8_t> pothole;
    ReadOnlyGattCharacteristic<uint8_t> antitheft;
    ReadOnlyGattCharacteristic<uint8_t> alarm;
    ReadOnlyGattCharacteristic<uint8_t> mqtt;
    WriteOnlyArrayGattCharacteristic<char,17> key;
    char initialkey[17];
};

#endif /* #ifndef __BLE_BIKESENSE_SERVICE_H__ */

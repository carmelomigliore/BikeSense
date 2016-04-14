#include "mbed-drivers/mbed.h"
#include "x-nucleo-iks01a1/x_nucleo_iks01a1.h"

typedef struct {
    int32_t AXIS_X;
    int32_t AXIS_Y;
    int32_t AXIS_Z;
} AxesRaw_TypeDef;

class Sensors {

public:
Sensors(){
	mems_expansion_board = X_NUCLEO_IKS01A1::Instance();
	gyroscope = mems_expansion_board->GetGyroscope();
	accelerometer = mems_expansion_board->GetAccelerometer();
	magnetometer = mems_expansion_board->magnetometer;
	humidity_sensor = mems_expansion_board->ht_sensor;
	pressure_sensor = mems_expansion_board->pt_sensor;
	temp_sensor1 = mems_expansion_board->ht_sensor;
	temp_sensor2 = mems_expansion_board->pt_sensor;
	bikeSenseServicePtr = NULL;
	mbed::util::FunctionPointer0<void> ptr(this,&Sensors::readValues);
	minar::Scheduler::postCallback(ptr.bind()).period(minar::milliseconds(30000));
}

float getTemperature(){
	return TEMPERATURE_Value;
}

float getPressure(){
	return PRESSURE_Value;
}

float getHumidity(){
	return HUMIDITY_Value;
}

AxesRaw_TypeDef readAccelerometer(){
	CALL_METH(accelerometer, Get_X_Axes, (int32_t *)&ACC_Value, 0);
	return ACC_Value;
}

void setBluetoothServicePtr(BikeSenseService *bikeSenseServicePtr){
	this->bikeSenseServicePtr = bikeSenseServicePtr;
}


void readValues(){
	DEBUG_PRINT("Updating sensors values\n");
	CALL_METH(temp_sensor1, GetTemperature, &TEMPERATURE_Value, 0.0f);
	CALL_METH(pressure_sensor, GetPressure, &PRESSURE_Value, 0.0f);
	CALL_METH(humidity_sensor, GetHumidity, &HUMIDITY_Value, 0.0f);
	if(bikeSenseServicePtr != NULL){
		bikeSenseServicePtr->updateTempValue((int16_t)TEMPERATURE_Value);
		bikeSenseServicePtr->updateHumidityValue((uint16_t)HUMIDITY_Value);
	}
}

private:
X_NUCLEO_IKS01A1 *mems_expansion_board = X_NUCLEO_IKS01A1::Instance();
GyroSensor *gyroscope = mems_expansion_board->GetGyroscope();
MotionSensor *accelerometer = mems_expansion_board->GetAccelerometer();
MagneticSensor *magnetometer = mems_expansion_board->magnetometer;
HumiditySensor *humidity_sensor = mems_expansion_board->ht_sensor;;
PressureSensor *pressure_sensor = mems_expansion_board->pt_sensor;
TempSensor *temp_sensor1 = mems_expansion_board->ht_sensor;
TempSensor *temp_sensor2 = mems_expansion_board->pt_sensor;

float TEMPERATURE_Value;
float HUMIDITY_Value;
float PRESSURE_Value;
float PRESSURE_Temp_Value;
AxesRaw_TypeDef MAG_Value;
AxesRaw_TypeDef ACC_Value;
AxesRaw_TypeDef GYR_Value;
BikeSenseService *bikeSenseServicePtr;
};

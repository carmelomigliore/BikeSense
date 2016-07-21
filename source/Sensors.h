#include "mbed-drivers/mbed.h"
#include "x-nucleo-iks01a1/x_nucleo_iks01a1.h"
#include "LMP91000.h"
#include <cstdlib>

#define ACCELEROMETER_PERIOD 500
#define ENVSENSORS_PERIOD 1000
#define SDA D14
#define SCL D15
#define ANALOGIN A3
#define MENABLE D7


typedef struct {
    int32_t AXIS_X;
    int32_t AXIS_Y;
    int32_t AXIS_Z;
} AxesRaw_TypeDef;

class Sensors {

public:
Sensors(mbed::util::FunctionPointer0<void> myJoltCallback, mbed::util::FunctionPointer0<void> mySensorsReadCallback): joltCallback(myJoltCallback), sensorsReadCallback(mySensorsReadCallback), lmp(SDA,SCL), analogin(ANALOGIN), mEnable(MENABLE), accValues(){
	mEnable = 1;
	wait_ms(50);
	mems_expansion_board = X_NUCLEO_IKS01A1::Instance();
	gyroscope = mems_expansion_board->GetGyroscope();
	accelerometer = mems_expansion_board->GetAccelerometer();
	magnetometer = mems_expansion_board->magnetometer;
	humidity_sensor = mems_expansion_board->ht_sensor;;
	pressure_sensor = mems_expansion_board->pt_sensor;
	temp_sensor1 = mems_expansion_board->ht_sensor;
	temp_sensor2 = mems_expansion_board->pt_sensor;
	mEnable = 0;
	wait_ms(50);
	Old_ACC_Value.AXIS_X = 0;
	Old_ACC_Value.AXIS_Y = 0;
	Old_ACC_Value.AXIS_Z = 1000;
	uint8_t res = lmp.configure( 
            LMP91000_TIA_GAIN_350K,
            LMP91000_REF_SOURCE_INT | LMP91000_INT_Z_20PCT,
            LMP91000_FET_SHORT_DISABLED | LMP91000_OP_MODE_AMPEROMETRIC                  
      );
      mEnable = 1;
}

~Sensors(){
}

void startReading(){
	mbed::util::FunctionPointer0<void> ptr (this, &Sensors::readAccelerometer);
	accelerometerHandle = minar::Scheduler::postCallback(ptr.bind()).period(minar::milliseconds(ACCELEROMETER_PERIOD)).getHandle();
	
	//mbed::util::FunctionPointer0<void> ptr2 (this, &Sensors::readSensorValues);
	//envsensorsHandle = minar::Scheduler::postCallback(ptr2.bind()).period(minar::milliseconds(ENVSENSORS_PERIOD)).getHandle();
}

void stopReading(){
	minar::Scheduler::cancelCallback(accelerometerHandle);
	minar::Scheduler::cancelCallback(envsensorsHandle);
}

std::vector<int32_t>& getAccValues(){
	return accValues;
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

float getMonoxide(){
	return monoxide;
}

int getMaxX(){
	return maxX;
}

int getMaxY(){
	return maxY;
}

int getMaxZ(){
	return maxZ;
}

int getMinX(){
	return minX;
}

int getMinY(){
	return minY;
}

int getMinZ(){
	return minZ;
}

void resetMinMax(){
	minX = 0;
	minY = 0;
	minZ = 1000;
	maxX = 0;
	maxY = 0;
	maxZ = 1000;
}

void readSensorValues(){
	
	CALL_METH(temp_sensor1, GetTemperature, &TEMPERATURE_Value, 0.0f);
	CALL_METH(pressure_sensor, GetPressure, &PRESSURE_Value, 0.0f);
	CALL_METH(humidity_sensor, GetHumidity, &HUMIDITY_Value, 0.0f);
	DEBUG_PRINT("\nTemperature: %d", (int)TEMPERATURE_Value);
	int read = 0;
	for(int i =0; i< 10; i++){
		read += analogin.read_u16();
	}
	DEBUG_PRINT("Read: %d", (int)(read/10));
	read = (read / 10) - 12500;
	float volt = (read * 3.3) / 65535;
	float current = volt / 350000;
	monoxide = current / 4e-9; // 4e-9 = sensitivity of the sensor, must be changed
	minar::Scheduler::postCallback(sensorsReadCallback.bind());
}

private:

void resetJoltSensitivity(){
	insensitiveToJolt = false;
}

void readAccelerometer(){
	CALL_METH(accelerometer, Get_X_Axes, (int32_t *)&ACC_Value, 0);
	if (ACC_Value.AXIS_X < minX)
		minX = ACC_Value.AXIS_X;
	else if(ACC_Value.AXIS_Y > maxX)
		maxX = ACC_Value.AXIS_X;
	if (ACC_Value.AXIS_Y < minY)
		minY = ACC_Value.AXIS_Y;
	else if(ACC_Value.AXIS_Y > maxY)
		maxY = ACC_Value.AXIS_Y;
		
	if (ACC_Value.AXIS_Z < minZ)
		minZ = ACC_Value.AXIS_Z;
	else if(ACC_Value.AXIS_Z > maxZ)
		maxZ = ACC_Value.AXIS_Z;
		
	if(!insensitiveToJolt && (abs(ACC_Value.AXIS_X - Old_ACC_Value.AXIS_X) > 300 || abs(ACC_Value.AXIS_Y - Old_ACC_Value.AXIS_Y) > 300 || abs(ACC_Value.AXIS_Z - Old_ACC_Value.AXIS_Z) > 300 )){
		minar::Scheduler::postCallback(joltCallback.bind());
		insensitiveToJolt=true;
		minar::Scheduler::postCallback(mbed::util::FunctionPointer0<void>(this, &Sensors::resetJoltSensitivity).bind()).delay(minar::milliseconds(3000));	
	}
	Old_ACC_Value = ACC_Value;
	if(accValues.size()<72){
		accValues.push_back(ACC_Value.AXIS_X);
		accValues.push_back(ACC_Value.AXIS_Y);
		accValues.push_back(ACC_Value.AXIS_Z);
	}
	
}

X_NUCLEO_IKS01A1 *mems_expansion_board;
GyroSensor *gyroscope;
MotionSensor *accelerometer;
MagneticSensor *magnetometer;
HumiditySensor *humidity_sensor;
PressureSensor *pressure_sensor;
TempSensor *temp_sensor1;
TempSensor *temp_sensor2;

float TEMPERATURE_Value;
float HUMIDITY_Value;
float PRESSURE_Value;
float PRESSURE_Temp_Value;
float monoxide=0;
bool insensitiveToJolt = false;
LMP91000 lmp;
AnalogIn analogin;
DigitalOut mEnable;
int minX=0, maxX=0, minY=0, maxY=0, minZ=1000, maxZ=1000;
AxesRaw_TypeDef MAG_Value;
AxesRaw_TypeDef ACC_Value, Old_ACC_Value;
AxesRaw_TypeDef GYR_Value;
std::vector<int32_t> accValues;

mbed::util::FunctionPointer0<void> joltCallback, sensorsReadCallback;
minar::callback_handle_t accelerometerHandle;
minar::callback_handle_t envsensorsHandle;
};

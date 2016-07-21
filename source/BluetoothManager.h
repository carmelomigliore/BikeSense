#include "mbed-drivers/mbed.h"
#include "ble/BLE.h"
#include "BikeSenseService.h"

class BluetoothManager {

	public:
	
	BluetoothManager(mbed::util::FunctionPointer0<void> bleCallback, mbed::util::FunctionPointer1<void,uint16_t> myCommandCallback, mbed::util::FunctionPointer1<void,const char*> myStringCallback): bleInitCompleteCallback(bleCallback), commandCallback(myCommandCallback), stringCallback(myStringCallback){
		
	}
	
	~BluetoothManager(){
		if(bikeSenseServicePtr != NULL){
			delete bikeSenseServicePtr;
		}
	}
	
	void init(){
		BLE::Instance().init<BluetoothManager>(this, &BluetoothManager::initComplete);
	}
	
	void initComplete(BLE::InitializationCompleteCallbackContext *params){
		DEBUG_PRINT("\nBLE init complete");
		BLE&        ble   = params->ble;
		ble_error_t error = params->error;

		if (error != BLE_ERROR_NONE) {
			/* In case of error, forward the error handling to onBleInitError */
			onBleInitError(ble, error);
			return;
		}

		/* Ensure that it is the default instance of BLE */
		if(ble.getInstanceID() != BLE::DEFAULT_INSTANCE) {
			return;
		}
		
		ble.gap().onConnection<BluetoothManager>(this, &BluetoothManager::connectionCallback);
		ble.gap().onDisconnection<BluetoothManager>(this, &BluetoothManager::disconnectionCallback);
		ble.gattServer().onDataWritten<BluetoothManager>(this, &BluetoothManager::onDataWrittenCallback);

	
		bikeSenseServicePtr = new BikeSenseService (ble, 0,2,0,4,0,0,0,0,0 /* initial values */);
		
		/* setup advertising */
		ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED
		     | GapAdvertisingData::LE_GENERAL_DISCOVERABLE);
		ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::
		    COMPLETE_LIST_16BIT_SERVICE_IDS, 
		    (uint8_t *)uuid16_list, sizeof(uuid16_list));
		ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::
		    COMPLETE_LOCAL_NAME, 
		    (uint8_t *)DEVICE_NAME, sizeof(DEVICE_NAME));
		ble.gap().setAdvertisingType(GapAdvertisingParams::
		    ADV_CONNECTABLE_UNDIRECTED);
		/* 1000ms. */
		ble.gap().setAdvertisingInterval(1000);
		button.fall(this, &BluetoothManager::temporaryAdvertising);
		//temporaryAdvertising();
		
		DEBUG_PRINT("\nBLE init complete END\n");
		minar::Scheduler::postCallback(bleInitCompleteCallback.bind());
	}
	
	void onDataWrittenCallback(const GattWriteCallbackParams *params) {
		if ((params->handle == bikeSenseServicePtr->getCommandHandle())) {
			uint16_t* data = (uint16_t*)(params->data);
			minar::Scheduler::postCallback(commandCallback.bind(*data));
			
		}else if(params->handle == bikeSenseServicePtr->getKeyHandle()){
			strncpy(authstring, (const char*)(params->data), 20);
			minar::Scheduler::postCallback(stringCallback.bind((const char*)authstring));
			DEBUG_PRINT("Received key: %s",(const char*)(params->data));
		}
        }
        
        void updateAntitheft(bool antitheft){
        	if(bikeSenseServicePtr != NULL)
        		bikeSenseServicePtr->updateAntitheftValue(antitheft);
        }
        
        void updateSensorsValue(int16_t temperature, uint16_t humidity, uint16_t monoxide){
        	DEBUG_PRINT("\ntemperature: %d", temperature);
        	if(bikeSenseServicePtr != NULL){
        		bikeSenseServicePtr->updateGasValue(monoxide);
        		bikeSenseServicePtr->updateTempValue(temperature);
        		bikeSenseServicePtr->updateHumidityValue(humidity);
        	}
        }
        
        void updatePothole(bool pothole){
        	if(bikeSenseServicePtr != NULL)
        		bikeSenseServicePtr->updatePotholeValue(pothole);
        }
        
        void updateAlarm(bool alarm){
        	if(bikeSenseServicePtr != NULL)
        		bikeSenseServicePtr->updateAlarmValue(alarm);
        }
        
        void updateMqtt(bool mqtt){
        	if(bikeSenseServicePtr != NULL)
        		bikeSenseServicePtr->updateMqttValue(mqtt);
        }
        
        void getMacAddress(char hexadecimalNumber[]){
        	uint8_t addr[6];
        	BLEProtocol::AddressType_t type;
        	BLE::Instance().gap().getAddress(&type, addr); //bug ST library?
        	
		    int i = 0;
		    int quotient, temp;
		    for(int j = 5; j>= 0; j--){
		    	quotient = addr[j];
		    	
			 while(quotient!=0){
		 		temp = quotient % 16;

	      			//To convert integer into character
	     		    	if( temp < 10)
		   			temp = temp + 48;
			    	else
					temp = temp + 55;

				hexadecimalNumber[i++]= temp;
				quotient = quotient / 16;
			    }
			    if(addr[j]< 16)
		    		hexadecimalNumber[i++]= 48;
		    	temp = hexadecimalNumber[i-1];
		    	hexadecimalNumber[i-1] = hexadecimalNumber[i-2];
		    	hexadecimalNumber[i-2] = temp;
			if(j> 0)	
		    		hexadecimalNumber[i++] = ':';
		    }
		hexadecimalNumber[i] = 0;
        }
        
        void temporaryAdvertising(){
        	DEBUG_PRINT("Start advertising\n");
        	minar::Scheduler::postCallback(mbed::util::FunctionPointer0<void>(this, &BluetoothManager::startAdvertising).bind());
        	minar::Scheduler::postCallback(mbed::util::FunctionPointer0<void>(this, &BluetoothManager::stopAdvertising).bind()).delay(minar::milliseconds(30000)); 	
        }
        
        void startAdvertising(){
        	BLE::Instance().gap().startAdvertising();
        }
        
        void stopAdvertising(){
        	BLE::Instance().gap().stopAdvertising();
        }
        
	
	void disconnectionCallback(const Gap::DisconnectionCallbackParams_t *params)
	{
	      temporaryAdvertising();
	      connected = false;
	      
	}
	
	void connectionCallback(const Gap::ConnectionCallbackParams_t *params)
	{
		stopAdvertising();
		connected= true;
	}
	
	bool isConnected(){
		return connected;
	}
	
	private:
	void onBleInitError(BLE &ble, ble_error_t error)
	{
	    /* Initialization error handling should go here */
	     DEBUG_PRINT("\nEzechiele 25-17");
	}
	
	mbed::util::FunctionPointer0<void> bleInitCompleteCallback;
	mbed::util::FunctionPointer1<void,uint16_t> commandCallback;
	mbed::util::FunctionPointer1<void,const char*> stringCallback;
	BikeSenseService *bikeSenseServicePtr;
	static const char DEVICE_NAME[10];
	static const uint16_t uuid16_list[1];
	static char authstring[20];
	static InterruptIn button;
	bool connected=false;;
};
const char BluetoothManager::DEVICE_NAME[10] = "BikeSense";
const uint16_t BluetoothManager::uuid16_list[1] = {BikeSenseService::BIKESENSE_SERVICE_UUID};
char BluetoothManager::authstring[20];
InterruptIn BluetoothManager::button(USER_BUTTON);

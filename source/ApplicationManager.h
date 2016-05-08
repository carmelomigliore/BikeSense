#include "mbed-drivers/mbed.h"
#include "sal-iface-serialmodem/FONA808.h"
#include "MQTTManager.h"
#include "BluetoothManager.h"
#include "Sensors.h"
#include "arduinojson/ArduinoJson.h"
#include "sal-stack-lwip/lwipv4_init.h"

#define INPUT_TOPIC "SiteWhere/input/json"
#define FONA_TX D8
#define FONA_RX D2
#define FONA_RESET D12
#define GPS_PERIOD 40000

#define APN "ibox.tim.it"
#define DEFAULTMQTTUSERNAME "device"
#define DEFAULTMQTTPASSWORD "password"
#define DEFAULTHOSTNAME "ec2-52-29-108-135.eu-central-1.compute.amazonaws.com"
#define DEFAULTAUTHSTRING "gokuvegeta123"
#define PORT 1883

#define ANTITHEFT_ON 1
#define ANTITHEFT_OFF 2

#ifdef DEBUGPRINT
#define DEBUG_PRINT(fmt, args...)    printf(fmt, ## args)
#else
#define DEBUG_PRINT(fmt, args...)    /* Don't do anything in release builds */
#endif

class ApplicationManager{

	public:
	
	ApplicationManager(): myFona(FONA_TX,FONA_RX,FONA_RESET, mbed::util::FunctionPointer3<void, float, float, float>(this, &ApplicationManager::onGPSRead), mbed::util::FunctionPointer0<void>(this, &ApplicationManager::onGPSFail)), 
	bluetooth(mbed::util::FunctionPointer0<void>(this, &ApplicationManager::onBLEInitComplete), mbed::util::FunctionPointer1<void, uint16_t>(this, &ApplicationManager::bluetoothCommandReceived), mbed::util::FunctionPointer1<void, const char*> (this, &ApplicationManager::bluetoothStringReceived)), 
	sensors(mbed::util::FunctionPointer0<void>(this, &ApplicationManager::joltDetected), mbed::util::FunctionPointer0<void>(this, &ApplicationManager::sendMeasures)){
		//printf("Size: %d\n", sizeof(myFona));
		strncpy(apn, APN, 30);
		strncpy(authstring, DEFAULTAUTHSTRING, 80);
		strncpy(mqtthostname, DEFAULTHOSTNAME, 80);
		strncpy(mqttusername, DEFAULTMQTTUSERNAME, 40);
		strncpy(mqttpassword, DEFAULTMQTTPASSWORD, 40);
		timestamp[0] = 0;
	}
	
	~ApplicationManager(){
		if(mqtt!=NULL)
			delete mqtt;
	}
	
	void start(){
		DEBUG_PRINT("Starting the app\n");
		bluetooth.init();
	}
	
	private:
	
	void onBLEInitComplete(){
		DEBUG_PRINT("Connecting FONA\n");
		int ret;
		do{
			ret = myFona.connect(apn, NULL,NULL, mbed::util::FunctionPointer0<void>(this, &ApplicationManager::onFonaConnect));
		}while(ret!=0);
	}
	
	void bluetoothCommandReceived(uint16_t command){
		switch(command){
			case ANTITHEFT_ON:
				setAntitheft(true);
				break;
			case ANTITHEFT_OFF:
				setAntitheft(false);
				break;
		}
	}
	
	void bluetoothStringReceived(const char* string){
		DEBUG_PRINT("Bluetooth received %s", string);
		strncpy(authstring, string, 80);
	}
	
	void onMQTTConnect(){
		DEBUG_PRINT("OnMQTTConnect\n");
		mqttconnected=true;
		sensors.startReading();
		if(!gpsScheduled){
			minar::Scheduler::postCallback(mbed::util::FunctionPointer0<void>(this, &ApplicationManager::readGPS).bind());
			gpsScheduled= true;
		}
		//minar::Scheduler::postCallback(mbed::util::FunctionPointer0<void>(this, &ApplicationManager::setFakePosition).bind()).period(minar::milliseconds(5000));
	}
	
	void onMQTTDisconnect(){
		mqttconnected=false;
		mqtt->connect();  //si può fare meglio?

	}
	
	void onFonaConnect(){
		//DEBUG_PRINT("OnFonaConnect\n");
		if(!ipstackinitialized){
			lwipv4_socket_init();
			ipstackinitialized = true;
		}
		mqtt = new MQTTManager((char*)mqttusername, (char *)mqttpassword, mbed::util::FunctionPointer0<void>(this, &ApplicationManager::onMQTTConnect), mbed::util::FunctionPointer0<void>(this, &ApplicationManager::onMQTTDisconnect), (char*)mqtthostname, PORT);
		mqtt->connect();
	}
	
	bool isConnected(){
		return mqttconnected;
	}
	
	void sendMeasures(){
		bluetooth.updateSensorsValue((int16_t)sensors.getTemperature(), (uint16_t)sensors.getHumidity(), (uint16_t)sensors.getMonoxide());
		if(timestamp[0]!=0 && mqttconnected){
		    StaticJsonBuffer<500> jsonmaker;
		    JsonObject& root = jsonmaker.createObject();
		    root["hardwareId"] = authstring;
		    root["type"] = "DeviceMeasurements";
		    JsonObject& request = root.createNestedObject("request");
		    request["updateState"] = true;
		    request["eventDate"] = timestamp;
		    JsonObject& measurements = request.createNestedObject("measurements");
		    measurements["temperature"].set(sensors.getTemperature(),1);
		    measurements["humidity"] = (int)sensors.getHumidity();
		    measurements["pressure"] = (int)sensors.getPressure();
		    measurements["monoxide"].set(sensors.getMonoxide(),1);
		    measurements["minX"] = sensors.getMinX();
		    measurements["maxX"] = sensors.getMaxX();
		    measurements["minY"] = sensors.getMinY();
		    measurements["maxY"] = sensors.getMaxY();
		    measurements["minZ"] = sensors.getMinZ();
		    measurements["maxZ"] = sensors.getMaxZ();
		    DEBUG_PRINT("MinX: %d\n", sensors.getMinX());
		    char buf[500];
		    root.printTo(buf,500);
		    mqtt->publish(INPUT_TOPIC,buf);
		    sensors.resetMinMax();
    		}
	}
	
	void enableGPS(bool enable){
		if(enable != gpsenabled){
			myFona.enableGPS(enable);
			gpsenabled=enable;
		}
	}
	
		
	void readGPS(){
		enableGPS(true);
		myFona.readGPSInfo();
	}
	
	void onGPSRead(float latitude, float longitude, float altitude){
		strncpy(timestamp, myFona.getTimestamp(),25);
		if(timestamp[0]!=0 && mqttconnected){
			StaticJsonBuffer<500> jsonmaker;
		    	JsonObject& root = jsonmaker.createObject();
		    	root["hardwareId"] = authstring;
		   	root["type"] = "DeviceLocation";
		   	JsonObject& request = root.createNestedObject("request");
		   	request["latitude"].set(latitude,5);
		   	request["longitude"].set(longitude,5);
		   	request["elevation"].set(altitude,2);
		   	request["updateState"] = true;
			request["eventDate"] = timestamp;
		   	char buf[500];
		   	root.printTo(buf,300);
		   	mqtt->publish(INPUT_TOPIC,buf);
		}
		minar::Scheduler::postCallback(mbed::util::FunctionPointer0<void>(this, &ApplicationManager::readGPS).bind()).delay(minar::milliseconds(GPS_PERIOD));
	}
	
	void onGPSFail(){
		DEBUG_PRINT("GPS has not made the FIX yet\n");
		minar::Scheduler::postCallback(mbed::util::FunctionPointer0<void>(this, &ApplicationManager::readGPS).bind()).delay(minar::milliseconds(GPS_PERIOD));
	}
	
	void joltDetected(){
		DEBUG_PRINT("Jolt detected\n");
		if(!antitheft){
			sendHole();
		}
		else{
			sendJoltAlarm();
		}
	}
	
	void sendHole(){
		if(timestamp[0]!=0 && mqttconnected){
			StaticJsonBuffer<500> jsonmaker;
			JsonObject& root = jsonmaker.createObject();
		    	root["hardwareId"] = authstring;
		   	root["type"] = "DeviceAlert";
		   	JsonObject& request = root.createNestedObject("request");
		   	request["type"] = "pothole";
		   	request["level"] = "Warning";
		   	request["message"] = "Pothole detected";
		   	request["updateState"] = true;
		   	request["eventDate"] = timestamp;
		   	JsonObject& metadata = request.createNestedObject("metadata");
		   	char buf[500];
		    	root.printTo(buf,500);
		   	mqtt->publish(INPUT_TOPIC,buf);
		}
	}
	
	void sendJoltAlarm(){
		if(mqttconnected){
			StaticJsonBuffer<500> jsonmaker;	//send also if timestamp isn't available (no GPS fix)
			JsonObject& root = jsonmaker.createObject();
		    	root["hardwareId"] = authstring;
		   	root["type"] = "DeviceAlert";
		   	JsonObject& request = root.createNestedObject("request");
		   	request["type"] = "antitheft.alarm";
		   	request["level"] = "Critical";
		   	request["message"] = "Jolt antitheft alarm";
		   	request["updateState"] = true;
		   	request["eventDate"] = timestamp;
		   	JsonObject& metadata = request.createNestedObject("metadata");
		   	char buf[500];
		    	root.printTo(buf,500);
		   	mqtt->publish(INPUT_TOPIC,buf);
	   	}
	}
	
	void setAntitheft(bool enable){
		if(mqttconnected){
			StaticJsonBuffer<500> jsonmaker;	//send also if timestamp isn't available (no GPS fix)
			JsonObject& root = jsonmaker.createObject();
		    	root["hardwareId"] = authstring;
		   	root["type"] = "DeviceAlert";
		   	JsonObject& request = root.createNestedObject("request");
		   	request["type"] = enable ? "antitheft.on" : "antitheft.off";
		   	request["level"] = "Info";
		   	request["message"] = enable ? "Antitheft system activated" : "Antitheft system deactivated";
		   	request["updateState"] = true;
		   	request["eventDate"] = timestamp;
		   	JsonObject& metadata = request.createNestedObject("metadata");
		   	char buf[500];
		    	root.printTo(buf,500);
		   	mqtt->publish(INPUT_TOPIC,buf);
		   	antitheft = enable;
		   	bluetooth.updateAntitheft(enable);
	   	}
	}
	
	void setFakePosition(){
		if(!antitheft){
			if(++fakeposindex==52){
				fakeposindex=0;
			}
			latitude=fakepositions[fakeposindex][1];
			longitude=fakepositions[fakeposindex][0];
		}
		strncpy(timestamp, "2016-05-10T19:42:03.000Z",25);
	}
	
	
	FONA808 myFona;
	BluetoothManager bluetooth;
	MQTTManager *mqtt;
	Sensors sensors;
	bool gpsScheduled = false;
	static char apn[30];
	static char mqttusername[40];
	static char mqttpassword[40];
	static char mqtthostname[80];
	static char timestamp[25];
	static char authstring[80];
	float latitude, longitude, altitude;
	bool mqttconnected=false, gpsenabled=false, antitheft = false, ipstackinitialized = false;
	int fakeposindex=0;
	static float fakepositions[53][2];
};

char ApplicationManager::apn[30];
char ApplicationManager::mqttusername[40];
char ApplicationManager::mqttpassword[40];
char ApplicationManager::mqtthostname[80];
char ApplicationManager::authstring[80];
char ApplicationManager::timestamp[25];
/*float ApplicationManager::fakepositions[53][2] = {{7.661740779876708,45.06115410805569},{7.6621055603027335,45.06163911918919},{7.662513256072997,45.0622302209452},{7.662942409515381,45.062836472808016},{7.663350105285645,45.0634578742955},{7.663757801055907,45.06400348954482},{7.664208412170409,45.064564255346944},{7.664444446563721,45.06497345935262},{7.6647233963012695,45.065367504885955},{7.66495943069458,45.06574639226037},{7.665302753448486,45.06618589846858},{7.66568899154663,45.06671633249558},{7.666032314300537,45.06723160655151},{7.666611671447754,45.067186141380475},{7.667191028594971,45.06700428033471},{7.667641639709473,45.066837573867794},{7.668199539184569,45.06661024608395},{7.668821811676025,45.066398072670026},{7.669465541839599,45.06620105379482},{7.670152187347412,45.06594341270263},{7.670624256134032,45.06551906013708},{7.671182155609131,45.06539781596832},{7.67195463180542,45.06527657154241},{7.672770023345947,45.06501892628354},{7.673542499542235,45.064730968443584},{7.673907279968261,45.06459456685522},{7.674615383148194,45.064352074338984},{7.67519474029541,45.06413989254349},{7.675881385803222,45.06386708622066},{7.676653861999511,45.063594278596014},{7.677297592163086,45.06338209398792},{7.677855491638183,45.06350334243186},{7.67819881439209,45.06397317772311},{7.678391933441161,45.06429145104921},{7.67869234085083,45.06471581272761},{7.6788854598999015,45.06504923755073},{7.67918586730957,45.06535234933873},{7.679443359375,45.065716081362844},{7.6796579360961905,45.066019189614984},{7.67993688583374,45.066382917396055},{7.680301666259766,45.06689819445748},{7.680559158325194,45.067292226723325},{7.680881023406982,45.06773172104626},{7.681288719177246,45.06823183116537},{7.681546211242675,45.068671318263945},{7.681739330291748,45.06897441084582},{7.681975364685059,45.06953512788536},{7.682404518127441,45.07003522222574},{7.68268346786499,45.07055046636354},{7.683284282684325,45.0707929325859},{7.6844000816345215,45.07083839488808},{7.685215473175048,45.0705656205326},{7.686009407043457,45.07027769063389}};*/

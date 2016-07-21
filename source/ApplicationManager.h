#include "mbed-drivers/mbed.h"
#include "sal-iface-serialmodem/FONA808.h"
#include "MQTTManager.h"
#include "BluetoothManager.h"
#include "Sensors.h"
#include "arduinojson/ArduinoJson.h"
#include "sal-stack-lwip/lwipv4_init.h"
#include <sstream>
#include <iterator>


#define INPUT_TOPIC "SiteWhere/input/jsonbatch"
#define FONA_TX D8
#define FONA_RX D2
#define FONA_RESET D12
#define DEFAULT_PERIOD 15000
#define ANTITHEFT_PERIOD 30000

#define APN "ibox.tim.it"
#define DEFAULTMQTTUSERNAME "device"
#define DEFAULTMQTTPASSWORD "password"
#define DEFAULTHOSTNAME "ec2-52-59-21-67.eu-central-1.compute.amazonaws.com"
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
		int ret, i=0;
		do{
			ret = myFona.connect(apn, NULL,NULL, mbed::util::FunctionPointer0<void>(this, &ApplicationManager::onFonaConnect));
			i++;
		}while(ret!=0 && i<3);
		if(i == 3){
			fonaispresent = false;
			sensors.startReading();
			sensors.readSensorValues();
		}
			
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
		switch(string[0]){
			case 'U': //USERNAME
				strncpy(mqttusername, string+1, 80);
				DEBUG_PRINT("USER: %s\n",mqttusername);
				break;
			case 'P': //PASSWORD
				strncpy(mqttpassword, string+1, 80);
				DEBUG_PRINT("PASSWORD: %s\n",mqttpassword);
				break;
			case 'D': //DOMAIN
				strncpy(mqtthostname, string+1, 80);
				DEBUG_PRINT("DOMAIN: %s\n",mqtthostname);
				mqtt->disconnect();
				delete mqtt;
				mqtt = new MQTTManager((char*)mqttusername, (char *)mqttpassword, mbed::util::FunctionPointer0<void>(this, &ApplicationManager::onMQTTConnect), mbed::util::FunctionPointer0<void>(this, &ApplicationManager::onMQTTDisconnect), (char*)mqtthostname, PORT);
				break;
			case 'p': //PORT
				break;
			default:
				DEBUG_PRINT("Mannaggia i pupi\n");
				
		}
		//strncpy(authstring, string, 80);
	}
	
	void onMQTTConnect(){
		DEBUG_PRINT("OnMQTTConnect\n");
		mqttconnected=true;
		sensors.startReading();
		//if(!gpsScheduled){
		minar::Scheduler::postCallback(mbed::util::FunctionPointer0<void>(this, &ApplicationManager::readGPS).bind());
			//gpsScheduled= true;
		//}
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
		DEBUG_PRINT("Sending measures\n");
		bluetooth.updateSensorsValue((int16_t)sensors.getTemperature(), (uint16_t)sensors.getHumidity(), (uint16_t)sensors.getMonoxide());
		if(timestamp[0]!=0 && mqttconnected && !bluetooth.isConnected()){
		    StaticJsonBuffer<1500> jsonmaker;
		    JsonObject& root = jsonmaker.createObject();
		    /*root["hardwareId"] = authstring;
		    root["type"] = "DeviceMeasurements";
		    JsonObject& request = root.createNestedObject("request");
		    request["updateState"] = true;
		    request["eventDate"] = timestamp;
		    JsonObject& measurements = request.createNestedObject("measurements");
		    measurements["t"].set(sensors.getTemperature(),1);
		    measurements["h"] = (int)sensors.getHumidity();
		    measurements["p"] = (int)sensors.getPressure();
		    measurements["m"].set(sensors.getMonoxide(),1);
		    //JsonArray& accvalues = measurements.createNestedArray("accvalues");
		    measurements["minX"] = sensors.getMinX();
		    measurements["maxX"] = sensors.getMaxX();
		    measurements["minY"] = sensors.getMinY();
		    measurements["maxY"] = sensors.getMaxY();
		    measurements["minZ"] = sensors.getMinZ();
		    measurements["maxZ"] = sensors.getMaxZ();
		    std::vector<int32_t>& vect = sensors.getAccValues();
		    std::ostringstream oss;
		    DEBUG_PRINT("Before for each\n");
		    //for(int i = 0; i< vect.size(); i++){
		    	std::copy(vect.begin(), vect.end()-1, std::ostream_iterator<int32_t>(oss, ","));
		   // }
		    oss << vect.back();
		    DEBUG_PRINT("After foreach, %d\n", vect.back());
		    JsonObject& metadata = request.createNestedObject("metadata");
		    //metadata["accValues"] = oss.str().c_str(); 
		    DEBUG_PRINT("MinX: %d\n", sensors.getMinX());
		    
		    
		    DEBUG_PRINT("Json: %s", buf);*/
		    DEBUG_PRINT("BARABBA0\n");
		    char hexadecimalNumber[30];
		    bluetooth.getMacAddress(hexadecimalNumber);  
		    DEBUG_PRINT("BARABBA1\n");   
		    root["hardwareId"] = hexadecimalNumber;
		    JsonObject& request = jsonmaker.createObject();
		    request["latitude"].set(latitude,5);
		    request["longitude"].set(longitude,5);
		    request["elevation"].set(altitude,2);
		    request["updateState"] = true;
		    request["eventDate"] = timestamp;
		    JsonArray& locations = root.createNestedArray("locations");
		    locations.add(request);
		    DEBUG_PRINT("BARABBA3\n");
		    JsonObject& dummy = jsonmaker.createObject();
		    JsonObject& measurements = dummy.createNestedObject("measurements");
		    measurements["t"].set(sensors.getTemperature(),1);
		    measurements["h"] = (int)sensors.getHumidity();
		    measurements["p"] = (int)sensors.getPressure();
		    measurements["g"].set(sensors.getMonoxide(),1);
		    measurements["minX"] = sensors.getMinX();
		    measurements["maxX"] = sensors.getMaxX();
		    measurements["minY"] = sensors.getMinY();
		    measurements["maxY"] = sensors.getMaxY();
		    measurements["minZ"] = sensors.getMinZ();
		    measurements["maxZ"] = sensors.getMaxZ();
		    
		    JsonArray& measurearray = root.createNestedArray("measurements");
		    measurearray.add(dummy);
		    char buf[1500];
		    root.printTo(buf,1500);
		    DEBUG_PRINT("BARABBA4\n");
		    //mqtt->publish(INPUT_TOPIC,buf);
		    mqtt->publish(INPUT_TOPIC,buf);
		    DEBUG_PRINT("BARABBA5\n");
		    sensors.resetMinMax();
		    
    		}
    		if(fonaispresent)
    			minar::Scheduler::postCallback(mbed::util::FunctionPointer0<void>(this, &ApplicationManager::readGPS).bind()).delay(minar::milliseconds(period));
    		else
    			minar::Scheduler::postCallback(mbed::util::FunctionPointer0<void>(&sensors, &Sensors::readSensorValues).bind()).delay(minar::milliseconds(period));
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
	DEBUG_PRINT("Sending locations\n");
		strncpy(timestamp, myFona.getTimestamp(),25);
		this->latitude = latitude;
		this->longitude = longitude;
		this->altitude = altitude;
		/*if(timestamp[0]!=0 && mqttconnected){
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
		}*/
		//minar::Scheduler::postCallback(mbed::util::FunctionPointer0<void>(this, &ApplicationManager::readGPS).bind()).delay(minar::milliseconds(GPS_PERIOD));
		sensors.readSensorValues();
	}
	
	void onGPSFail(){
		DEBUG_PRINT("GPS has not made the FIX yet\n");
		//minar::Scheduler::postCallback(mbed::util::FunctionPointer0<void>(this, &ApplicationManager::readGPS).bind()).delay(minar::milliseconds(GPS_PERIOD));
		sensors.readSensorValues();
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
		bluetooth.updatePothole(true);
		DEBUG_PRINT("pothole");
		/*if(timestamp[0]!=0 && mqttconnected){
			StaticJsonBuffer<1500> jsonmaker;
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
		}*/
	}
	
	void sendJoltAlarm(){
		DEBUG_PRINT("Jolt Alarm\n");
		bluetooth.updateAlarm(true);
		if(mqttconnected){
			StaticJsonBuffer<1500> jsonmaker;	//send also if timestamp isn't available (no GPS fix)
			JsonObject& root = jsonmaker.createObject();
			char hexadecimalNumber[30];
		        bluetooth.getMacAddress(hexadecimalNumber);   
			root["hardwareId"] = hexadecimalNumber;
			    JsonObject& request = jsonmaker.createObject();
			    request["latitude"].set(latitude,5);
			    request["longitude"].set(longitude,5);
			    request["elevation"].set(altitude,2);
			    request["updateState"] = true;
			    request["eventDate"] = timestamp;
			    JsonArray& locations = root.createNestedArray("locations");
			    locations.add(request);
			 
			JsonArray& alerts =  root.createNestedArray("alerts");    
		   	//root["type"] = "DeviceAlert";
		   	JsonObject& alert = jsonmaker.createObject();
		   	alert["type"] = "antitheft.alarm";
		   	alert["level"] = "Critical";
		   	alert["message"] = "Jolt antitheft alarm";
		   	alert["updateState"] = true;
		   	alert["eventDate"] = timestamp;
		   	alerts.add(alert);
		   	char buf[600];
		    	root.printTo(buf,600);
		   	mqtt->publish(INPUT_TOPIC,buf);
	   	}
	}
	
	void setAntitheft(bool enable){
		
		if(bluetooth.isConnected()){
	   		antitheft = enable;
		   	bluetooth.updateAntitheft(enable);
		   	period = enable?ANTITHEFT_PERIOD:DEFAULT_PERIOD;
		   	if(!enable)
		   		bluetooth.updateAlarm(false);
	   	}
		else if(fonaispresent && mqttconnected){
			StaticJsonBuffer<1500> jsonmaker;	//send also if timestamp isn't available (no GPS fix)
			JsonObject& root = jsonmaker.createObject();
			char hexadecimalNumber[30];
		        bluetooth.getMacAddress(hexadecimalNumber);   
			root["hardwareId"] = hexadecimalNumber;
			    JsonObject& request = jsonmaker.createObject();
			    request["latitude"].set(latitude,5);
			    request["longitude"].set(longitude,5);
			    request["elevation"].set(altitude,2);
			    request["updateState"] = true;
			    request["eventDate"] = timestamp;
			    JsonArray& locations = root.createNestedArray("locations");
			    locations.add(request);
			 
			JsonArray& alerts =  root.createNestedArray("alerts");    
		   	//root["type"] = "DeviceAlert";
		   	JsonObject& alert = jsonmaker.createObject();
		   	alert["type"] = enable ? "antitheft.on" : "antitheft.off";
		   	alert["level"] = "Info";
		   	alert["message"] = enable ? "Antitheft system activated" : "Antitheft system deactivated";
		   	alert["updateState"] = true;
		   	alert["eventDate"] = timestamp;
		   	alerts.add(alert);
		   	char buf[600];
		    	root.printTo(buf,600);
		   	mqtt->publish(INPUT_TOPIC,buf);
		   	antitheft = enable;
		   	bluetooth.updateAntitheft(enable);
		   	period = enable?ANTITHEFT_PERIOD:DEFAULT_PERIOD;
		   	if(!enable)
		   		bluetooth.updateAlarm(false);
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
	static char apn[30];
	static char mqttusername[40];
	static char mqttpassword[40];
	static char mqtthostname[80];
	static char timestamp[25];
	static char authstring[80];
	float latitude, longitude, altitude;
	bool mqttconnected=false, gpsenabled=false, antitheft = false, ipstackinitialized = false, fonaispresent = true;
	int fakeposindex=0;
	int period = DEFAULT_PERIOD;
	static float fakepositions[53][2];
};

char ApplicationManager::apn[30];
char ApplicationManager::mqttusername[40];
char ApplicationManager::mqttpassword[40];
char ApplicationManager::mqtthostname[80];
char ApplicationManager::authstring[80];
char ApplicationManager::timestamp[25];
/*float ApplicationManager::fakepositions[53][2] = {{7.661740779876708,45.06115410805569},{7.6621055603027335,45.06163911918919},{7.662513256072997,45.0622302209452},{7.662942409515381,45.062836472808016},{7.663350105285645,45.0634578742955},{7.663757801055907,45.06400348954482},{7.664208412170409,45.064564255346944},{7.664444446563721,45.06497345935262},{7.6647233963012695,45.065367504885955},{7.66495943069458,45.06574639226037},{7.665302753448486,45.06618589846858},{7.66568899154663,45.06671633249558},{7.666032314300537,45.06723160655151},{7.666611671447754,45.067186141380475},{7.667191028594971,45.06700428033471},{7.667641639709473,45.066837573867794},{7.668199539184569,45.06661024608395},{7.668821811676025,45.066398072670026},{7.669465541839599,45.06620105379482},{7.670152187347412,45.06594341270263},{7.670624256134032,45.06551906013708},{7.671182155609131,45.06539781596832},{7.67195463180542,45.06527657154241},{7.672770023345947,45.06501892628354},{7.673542499542235,45.064730968443584},{7.673907279968261,45.06459456685522},{7.674615383148194,45.064352074338984},{7.67519474029541,45.06413989254349},{7.675881385803222,45.06386708622066},{7.676653861999511,45.063594278596014},{7.677297592163086,45.06338209398792},{7.677855491638183,45.06350334243186},{7.67819881439209,45.06397317772311},{7.678391933441161,45.06429145104921},{7.67869234085083,45.06471581272761},{7.6788854598999015,45.06504923755073},{7.67918586730957,45.06535234933873},{7.679443359375,45.065716081362844},{7.6796579360961905,45.066019189614984},{7.67993688583374,45.066382917396055},{7.680301666259766,45.06689819445748},{7.680559158325194,45.067292226723325},{7.680881023406982,45.06773172104626},{7.681288719177246,45.06823183116537},{7.681546211242675,45.068671318263945},{7.681739330291748,45.06897441084582},{7.681975364685059,45.06953512788536},{7.682404518127441,45.07003522222574},{7.68268346786499,45.07055046636354},{7.683284282684325,45.0707929325859},{7.6844000816345215,45.07083839488808},{7.685215473175048,45.0705656205326},{7.686009407043457,45.07027769063389}};*/

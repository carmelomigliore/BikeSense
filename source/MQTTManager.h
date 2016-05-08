#include "mbed-drivers/mbed.h"
#include "mqttclient/MQTTSocket.h"
#include "mqttclient/MQTTAsync.h"

#ifdef DEBUGPRINT
#define DEBUG_PRINT(fmt, args...)    printf(fmt, ## args)
#else
#define DEBUG_PRINT(fmt, args...)    /* Don't do anything in release builds */
#endif


class MQTTManager{

	public: 
	MQTTManager(char* username, char* password, mbed::util::FunctionPointer0<void> connCallback, mbed::util::FunctionPointer0<void> discCallback, char* hostname, int myport=1883): connectionCallback(connCallback), disconnectionCallback(discCallback), port(myport){
		strncpy(this->hostname, hostname, 80);
		strncpy(this->username, username, 40);
		strncpy(this->password, password, 40);
		network = new MQTTSocket(mbed::util::FunctionPointer0<void>(this, &MQTTManager::onSocketConnected), mbed::util::FunctionPointer0<void>(this, &MQTTManager::onSocketDisconnected));
		client = new MQTT::Async<MQTTSocket,Countdown,DummyThread,DummyMutex>(network);
		instance = this;
	}
	
	~MQTTManager(){
		delete client;
		delete network;
	}
	
	void connect(){
		DEBUG_PRINT("MQTT Connect %s\n", hostname);
		network->connect((char*)hostname,port);
	}
	
	int publish(char* topic, char* payload){
		MQTT::Message message;
		message.qos = MQTT::QOS0;
		message.retained = false;
		message.dup = false;
		message.payload = (void*)payload;
		message.payloadlen = strlen(payload)+1;
		DEBUG_PRINT("Trying to publish something to topic %s\n",topic);
		return client->publish([](MQTT::Async<MQTTSocket, Countdown, DummyThread, DummyMutex>::Result* res){(void)res;}, topic,&message);
	}
	
	int subscribe(char* topic, MQTT::messageHandler callback){
		return client->subscribe(NULL,topic, MQTT::QOS0, callback);
	}
	
	
	private:
	void onSocketConnected(){
		MQTTPacket_connectData data = MQTTPacket_connectData_initializer;       
		data.MQTTVersion = 3;
		data.clientID.cstring = "mbed-device";
		data.username.cstring = username;
		data.password.cstring = password;
		MQTT::Async<MQTTSocket, Countdown, DummyThread, DummyMutex>::resultHandler myResHandler = &onConnectMQTT;
		client->connect(myResHandler, &data);
	}
	
	void onSocketDisconnected(){
		minar::Scheduler::postCallback(disconnectionCallback.bind());
		mqttconnected = false;
	}
	
	static void onConnectMQTT(MQTT::Async<MQTTSocket, Countdown, DummyThread, DummyMutex>::Result* res){
		instance->mqttconnected = true;
		minar::Scheduler::postCallback(instance->connectionCallback.bind());
	}
	
	MQTT::Async<MQTTSocket,Countdown,DummyThread,DummyMutex> *client;
	mbed::util::FunctionPointer0<void> connectionCallback, disconnectionCallback;
	MQTTSocket *network;
	static char username[40];
	static char password[40];
	static char hostname[80];
	int port;
	int numtries=0;
	bool mqttconnected = false;
	static MQTTManager *instance;
};

char MQTTManager::username[40];
char MQTTManager::password[40];
char MQTTManager::hostname[80];
MQTTManager* MQTTManager::instance;
 


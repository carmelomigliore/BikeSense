#include "mbed-drivers/mbed.h"
#include "sal-iface-serialmodem/FONA808.h"
#include "sockets/v0/TCPStream.h"
#include "sockets/v0/UDPSocket.h"
#include "sal-stack-lwip/lwipv4_init.h"
#include "ble/BLE.h"
//#include "ble/services/UARTService.h"
#include "BikeSenseService.h"
#include "Sensors.h"

// 
//#ifndef DEBUG
//#define DEBUG
//#endif

#ifdef DEBUGPRINT
#define DEBUG_PRINT(fmt, args...)    printf(fmt, ## args)
#else
#define DEBUG_PRINT(fmt, args...)    /* Don't do anything in release builds */
#endif

static const char DEVICE_NAME[] = "BikeSense";
static const uint16_t uuid16_list[] = {BikeSenseService::BIKESENSE_SERVICE_UUID/*, UARTServiceShortUUID*/};
BikeSenseService *bikeSenseServicePtr;
//UARTService *uartServicePtr;
//BLEDevice   ble;

static FONA808 myFona(D8,D2,D12);
//static MMA8451Q acc(D14,D15,PA_0,0x1d<<1);
Sensors nucleoSensors;

int x=100,y=100,z=100;
bool flag = false;
uint8_t anti = 0;
int posindex = 0;

float positions[][2] = {{7.661740779876708,45.06115410805569},{7.6621055603027335,45.06163911918919},{7.662513256072997,45.0622302209452},{7.662942409515381,45.062836472808016},{7.663350105285645,45.0634578742955},{7.663757801055907,45.06400348954482},{7.664208412170409,45.064564255346944},{7.664444446563721,45.06497345935262},{7.6647233963012695,45.065367504885955},{7.66495943069458,45.06574639226037},{7.665302753448486,45.06618589846858},{7.66568899154663,45.06671633249558},{7.666032314300537,45.06723160655151},{7.666611671447754,45.067186141380475},{7.667191028594971,45.06700428033471},{7.667641639709473,45.066837573867794},{7.668199539184569,45.06661024608395},{7.668821811676025,45.066398072670026},{7.669465541839599,45.06620105379482},{7.670152187347412,45.06594341270263},{7.670624256134032,45.06551906013708},{7.671182155609131,45.06539781596832},{7.67195463180542,45.06527657154241},{7.672770023345947,45.06501892628354},{7.673542499542235,45.064730968443584},{7.673907279968261,45.06459456685522},{7.674615383148194,45.064352074338984},{7.67519474029541,45.06413989254349},{7.675881385803222,45.06386708622066},{7.676653861999511,45.063594278596014},{7.677297592163086,45.06338209398792},{7.677855491638183,45.06350334243186},{7.67819881439209,45.06397317772311},{7.678391933441161,45.06429145104921},{7.67869234085083,45.06471581272761},{7.6788854598999015,45.06504923755073},{7.67918586730957,45.06535234933873},{7.679443359375,45.065716081362844},{7.6796579360961905,45.066019189614984},{7.67993688583374,45.066382917396055},{7.680301666259766,45.06689819445748},{7.680559158325194,45.067292226723325},{7.680881023406982,45.06773172104626},{7.681288719177246,45.06823183116537},{7.681546211242675,45.068671318263945},{7.681739330291748,45.06897441084582},{7.681975364685059,45.06953512788536},{7.682404518127441,45.07003522222574},{7.68268346786499,45.07055046636354},{7.683284282684325,45.0707929325859},{7.6844000816345215,45.07083839488808},{7.685215473175048,45.0705656205326},{7.686009407043457,45.07027769063389}};

float longitude = 7.662717103958131;
float latitude = 45.062343893659175;

static void clearFlag(){
	flag = false;
}

static void setPosition(){
	if(!anti){
		latitude=positions[posindex][1];
		longitude=positions[posindex][0];
		if(++posindex==52){
			posindex=0;
		}
	}
} 

using namespace mbed::Sockets::v0;

class Connection { 
private:
	TCPStream _sock;
	uint16_t* send;
	bool connected;
	SocketAddr sockaddr;

public:
	Connection(): _sock(SOCKET_STACK_LWIP_IPV4), connected(false){
		_sock.open(SOCKET_AF_INET4);
		_sock.setNagle(false);
		 send = new uint16_t[16];
      		 for(int i =0; i< 16; i++){
	  	 send[i] = i;
		}
	}
	void onReceive(Socket *s) {
        //DEBUG_PRINT("\nOnReceive");
	uint16_t result[64];
	size_t size = 20;
	socket_error_t err = s->recv(result, &size);
        DEBUG_PRINT("MBED: Socket Error: %s (%d)\r\n", socket_strerror(err), err);
	DEBUG_PRINT("\nThe result is %f",result[0]);
	send[0] = rand();
        send[1] = rand();
        send[2] = rand();
        socket_error_t err2 = _sock.send(send, 16*sizeof(float));
        DEBUG_PRINT("MBED: Socket Error: %s (%d)\r\n", socket_strerror(err2), err2);
    }

   
    void onConnect(TCPStream *s) {
    	connected=true;
  	DEBUG_PRINT("\nConnected!"); 
        s->setOnReadable(TCPStream::ReadableHandler_t(this, &Connection::onReceive));
        s->setOnDisconnect(TCPStream::DisconnectHandler_t(this, &Connection::onDisconnect));
        mbed::util::FunctionPointer0<void> ptr(this,&Connection::sendPosition);
         minar::Scheduler::postCallback(ptr.bind()).period(minar::milliseconds(4000));

         minar::Scheduler::postCallback(setPosition).period(minar::milliseconds(4000));
        /*send[0] = 666;
	       // send[1] = acc.getAccY();
		//send[2] = acc.getAccZ();
		socket_error_t err = _sock.send(send, 16*sizeof(uint16_t));
		DEBUG_PRINT("MBED: Socket Error: %s (%d)\r\n", socket_strerror(err), err);
		s->error_check(err);*/
  }
  
  void sendHole(){
  		DEBUG_PRINT("Sending Hole!\n");
  	       send[0] = 666;
  	       float* aux = (float*)&(send[1]);
  	       aux[0] = latitude;
  	       aux[1] = longitude;
	       // send[1] = acc.getAccY();
		//send[2] = acc.getAccZ();
		socket_error_t err = _sock.send(send, 16*sizeof(uint16_t));
		DEBUG_PRINT("MBED: Socket Error: %s (%d)\r\n", socket_strerror(err), err);
		//s->error_check(err);
  }
  
  void sendAnti(){
  	send[0] = 668;
       // send[1] = acc.getAccY();
	//send[2] = acc.getAccZ();
	socket_error_t err = _sock.send(send, 16*sizeof(uint16_t));
	DEBUG_PRINT("MBED: Socket Error: %s (%d)\r\n", socket_strerror(err), err);
	//s->error_check(err);
  }
  
  void sendPosition(){
	  	DEBUG_PRINT("Sending position\n");
	  	send[0] = 665;
  	        float* aux = (float*)&(send[1]);
  	        aux[0] = latitude;
  	        aux[1] = longitude;
  	        aux[2] = rand(); // GAS VALUE TO BE READ
  	        aux[3] = nucleoSensors.getTemperature();
  	        aux[4] = nucleoSensors.getHumidity();
  	        aux[5] = nucleoSensors.getPressure();
  		socket_error_t err = _sock.send(send, 16*sizeof(uint16_t));
		DEBUG_PRINT("MBED: Socket Error: %s (%d)\r\n", socket_strerror(err), err);
  }
  
   bool isConnected(){
   	return connected;
   }

   void onDisconnect(TCPStream *s) {
   	DEBUG_PRINT("Disconnected! Reconnecting now...");
       // s->close();
        //connected=false;
        connect(sockaddr);
       // DEBUG_PRINT("{{%s}}\r\n",(error()?"failure":"success"));
        //DEBUG_PRINT("{{end}}\r\n");
    }

    socket_error_t connect(SocketAddr sa) {
    		
    		sockaddr = sa;
      		socket_error_t err = _sock.connect(sa, 25, TCPStream::ConnectHandler_t(this, &Connection::onConnect));
		//return _sock.resolve(address,UDPSocket::DNSHandler_t(this, &Resolver::onDNS));
		//DEBUG_PRINT("\nerr: %d",err);
		DEBUG_PRINT("MBED: Socket Error: %s (%d)\r\n", socket_strerror(err), err);
		
    }
    
    socket_error_t connect() {
    	connect(sockaddr);
    }
    
    void disconnect(){
    	printf("Disonnect\n");
    	_sock.close();
    }
    

};

static Connection *conn;


static void readMma(){
	DEBUG_PRINT("\nReadMMA\n");
	uint16_t send[40];
	int tempx, tempy, tempz; 
	//acc.registerDump();
	AxesRaw_TypeDef axes = nucleoSensors.readAccelerometer();
	tempx = axes.AXIS_X;
	tempy = axes.AXIS_Y;
	tempz = axes.AXIS_Z;
	DEBUG_PRINT("\nReadMMA2\n");	
	if((tempz - z > 250 || tempz - z < -250) && !flag){
		flag = true;
		conn->sendHole();
		bikeSenseServicePtr->updatePotholeValue(1);
		printf("Buca!\n");
		minar::Scheduler::postCallback(clearFlag).delay(minar::milliseconds(3000));
	}
	x = tempx;
	y = tempy;
	z = tempz;
	printf("X: %d\n", x);
	printf("Y: %d\n", y);
	printf("Z: %d\n", z);
	
}


class Resolver {
private:
    UDPSocket _dns;
    Connection *myconn;
public:
    Resolver(Connection *conn) : _dns(SOCKET_STACK_LWIP_IPV4) {
        
 	_dns.open(SOCKET_AF_INET4);
 	myconn= conn;
    }

    void onDNS(Socket *s, struct socket_addr addr, const char *domain) {
        (void) s;
        DEBUG_PRINT("\nonDNS enter\n");
        SocketAddr sa;
        char buf[16];
        sa.setAddr(&addr);
        sa.fmtIPv4(buf,sizeof(buf));
        DEBUG_PRINT("Resolved %s to %s\r\n", domain, buf);
        myconn->connect(sa);
        minar::Scheduler::postCallback(readMma).delay(minar::milliseconds(4000)).period(minar::milliseconds(300));
    }

    
   
    socket_error_t resolve(const char * address) {
        //DEBUG_PRINT("Resolving %s...\r\n", address);
        return _dns.resolve(address,UDPSocket::DNSHandler_t(this, &Resolver::onDNS));
    }
};

static Resolver *r;

static void blinky(void) {
    static DigitalOut led(LED1);
    led=!led;
}

static void setAnti(){
	bikeSenseServicePtr->updateAntitheftValue(!anti);
	anti = !anti;
}

static void enableAnti(void){
	//printf("Testing connection");
	//conn->disconnect();
	//conn->connect();
	mbed::util::FunctionPointer0<void> ptr(conn,&Connection::sendAnti);
        minar::Scheduler::postCallback(ptr.bind());
	//conn->sendAlarm();
	minar::Scheduler::postCallback(setAnti);
		
}

static InterruptIn butt(USER_BUTTON);


/*static void enterSTOP(void){
  
  /* User push-button (External line 13) will be used to wakeup the system from STOP mode */
  //BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI);

  /* Enable Power Clock */
//  __HAL_RCC_PWR_CLK_ENABLE();
  
  /* Ensure that MSI is wake-up system clock */ 
/*  __HAL_RCC_WAKEUPSTOP_CLK_CONFIG(RCC_STOP_WAKEUPCLOCK_MSI);

   DEBUG_PRINT("Entering stop mode\n");
   HAL_Delay(3000);
   HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
   SetSysClock();
   DEBUG_PRINT("Woken up!\n");
}*/
//static RawSerial fona(D8,D2);

static void handler(void){
   // putc(fona.getc(),stdout);
}

void resolve_dns(){
//butt.fall(dummy);
 lwipv4_socket_init();

 conn = new Connection();
  r = new Resolver(conn);
 //r->connect("mbed.org");
 r->resolve("ec2-52-29-108-135.eu-central-1.compute.amazonaws.com");
 //r->resolve("fp7-intrepid.no-ip.biz");
 /* r->connect("mbed.org");
 r->connect("mbed.org");*/
 DEBUG_PRINT("\nResolve DNS complete");
}


/**
 * This function is called when the ble initialization process has failled
 */
void onBleInitError(BLE &ble, ble_error_t error)
{
    /* Initialization error handling should go here */
     DEBUG_PRINT("\nEzechiele 25-17");
}

void disconnectionCallback(const Gap::DisconnectionCallbackParams_t *params)
{
      BLE::Instance().gap().startAdvertising();
}


void onDataWrittenCallback(const GattWriteCallbackParams *params) {
        if ((params->handle == bikeSenseServicePtr->getCommandHandle())) {
        	uint16_t* data = (uint16_t*)(params->data);
        	if(*data == 1 || *data == 2){
        		enableAnti();
        	}
        }
    }

/**
 * Callback triggered when the ble initialization process has finished
 */
void bleInitComplete(BLE::InitializationCompleteCallbackContext *params)
{
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

    ble.gap().onDisconnection(disconnectionCallback);
    ble.gattServer().onDataWritten(onDataWrittenCallback);

	
	bikeSenseServicePtr = new BikeSenseService (ble, 0,2,0,4,0,0,0 /* initial values */);
	nucleoSensors.setBluetoothServicePtr(bikeSenseServicePtr);
	//uartServicePtr = new UARTService(ble);

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
	ble.gap().startAdvertising();
	 DEBUG_PRINT("\nBLE init complete END");
}


static void fona_prova(void){
 //myFona.init();
 int ret;
 do{
 	ret = myFona.connect("ibox.tim.it",NULL,NULL);
 } while(ret!=0);
 minar::Scheduler::postCallback(resolve_dns).delay(minar::milliseconds(9000));
}

/*static void periodicRead(){
	int c;
	while((c = uartServicePtr->_getc()) != EOF){
		DEBUG_PRINT("%c",(char)c);
	}
        fflush(stdout);
	uartServicePtr->writeString("Birra\n");
}*/

/*static I2C i2c(D14,D15);
static InterruptIn i1(D11);
static InterruptIn i2(D10);*/



/*static void i2csearch(){
	printf("I2CU! Searching for I2C devices...\n");
   	int count = 0;
    	for (int address=0; address<256; address+=2) {
        	if (!i2c.write(address, NULL, 0)) { // 0 returned is ok
        	    printf(" - I2C device found at address 0x%02X\n", address);
        	    count++;
        	}
    	}	
    	printf("%d devices found\n", count);
}*/



void app_start(int, char**) {
   butt.fall(enableAnti);
    minar::Scheduler::postCallback(blinky).period(minar::milliseconds(1000));
    //minar::Scheduler::postCallback(readMma).period(minar::milliseconds(500)).delay(minar::milliseconds(2000));
    BLE &ble = BLE::Instance();
    ble.init(bleInitComplete);
    minar::Scheduler::postCallback(fona_prova);
    DEBUG_PRINT("Hello!\n");
    //minar::Scheduler::postCallback(mmaInit);
    
   // minar::Scheduler::postCallback(enterSTOP).delay(minar::milliseconds(10000));
   
   // minar::Scheduler::postCallback(periodicRead).delay(minar::milliseconds(4000)).period(minar::milliseconds(10000));
}



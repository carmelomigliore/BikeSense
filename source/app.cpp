#include "mbed-drivers/mbed.h"
#include "sal-iface-serialmodem/FONA808.h"
#include "sockets/v0/TCPStream.h"
#include "sockets/v0/UDPSocket.h"
#include "sal-stack-lwip/lwipv4_init.h"
#include "ble/BLE.h"
//#include "ble/services/UARTService.h"
#include "BikeSenseService.h"
#include "MMA8451Q.h"

// 
//#ifndef DEBUG
//#define DEBUG
//#endif

#ifdef DEBUGPRINT
#define DEBUG_PRINT(fmt, args...)    printf(fmt, ## args)
#else
#define DEBUG_PRINT(fmt, args...)    /* Don't do anything in release builds */
#endif

const static char     DEVICE_NAME[] = "BikeSense";
static const uint16_t uuid16_list[] = {BikeSenseService::BIKESENSE_SERVICE_UUID/*, UARTServiceShortUUID*/};
BikeSenseService *bikeSenseServicePtr;
//UARTService *uartServicePtr;
//BLEDevice   ble;

static FONA808 myFona(D8,D2,D12);
static MMA8451Q acc(D14,D15,PA_0,0x1d<<1);

int x=100,y=100,z=100;
bool flag = false;
uint8_t anti = 0;
int posindex = 0;

float positions[][2] = {{7.662717103958131,45.062343893659175},{7.663382291793824,45.063329047712855},{7.66398310661316,45.06413989254349},{7.664508819580079,45.06488252538225},{7.664937973022462,45.065511482384075},{7.665388584136964,45.06614043246582},{7.665785551071168,45.06668602211236},{7.666150331497193,45.06723160655154},{7.665538787841798,45.06749681932851},{7.664905786514283,45.067724143586574},{7.66424059867859,45.06797419922623},{7.663865089416505,45.067595326618004},{7.663575410842896,45.06720129644151},{7.664090394973756,45.067049745650394}};

float longitude = 7.662717103958131;
float latitude = 45.062343893659175;

static void clearFlag(){
	flag = false;
}

static void setPosition(){
	latitude=positions[posindex][1];
	longitude=positions[posindex][0];
	if(++posindex==14){
		posindex=0;
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
  	//DEBUG_PRINT("\nConnected!"); 
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
  
  void sendAlarm(){
  	send[0] = 668;
       // send[1] = acc.getAccY();
	//send[2] = acc.getAccZ();
	socket_error_t err = _sock.send(send, 16*sizeof(uint16_t));
	DEBUG_PRINT("MBED: Socket Error: %s (%d)\r\n", socket_strerror(err), err);
	//s->error_check(err);
  }
  
  void sendPosition(){
  	if(!anti){
	  	DEBUG_PRINT("Sending position\n");
	  	 send[0] = 665;
	  	       float* aux = (float*)&(send[1]);
	  	       aux[0] = latitude;
	  	       aux[1] = longitude;
	  		socket_error_t err = _sock.send(send, 16*sizeof(uint16_t));
			DEBUG_PRINT("MBED: Socket Error: %s (%d)\r\n", socket_strerror(err), err);
		}
  }
  
   bool isConnected(){
   	return connected;
   }

   void onDisconnect(TCPStream *s) {
        s->close();
        connected=false;
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
	tempx = 100*acc.getAccX();
	tempy = 100*acc.getAccY();
	tempz = 100*acc.getAccZ();
	DEBUG_PRINT("\nReadMMA2\n");	
	if((tempx - x > 50 || tempx - x < -50 || tempy-y > 50 || tempy -y < -50) && !flag){
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
	/*printf("Y: %d\n", y);
	printf("Z: %d\n", z);*/
	
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
        SocketAddr sa;
        char buf[16];
        sa.setAddr(&addr);
        sa.fmtIPv4(buf,sizeof(buf));
        DEBUG_PRINT("Resolved %s to %s\r\n", domain, buf);
        myconn->connect(sa);
        minar::Scheduler::postCallback(readMma).period(minar::milliseconds(200));
        
        
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

static void dummy(void){
	//printf("Testing connection");
	//conn->disconnect();
	//conn->connect();
	mbed::util::FunctionPointer0<void> ptr(conn,&Connection::sendAlarm);
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
 //DEBUG_PRINT("\nResolve DNS");
 lwipv4_socket_init();

 conn = new Connection();
  r = new Resolver(conn);
 //r->connect("mbed.org");
 r->resolve("ec2-52-29-108-135.eu-central-1.compute.amazonaws.com");
 /* r->connect("mbed.org");
 r->connect("mbed.org");*/
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
        	DEBUG_PRINT("OnDataWritten\n");
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
}


static void fona_prova(void){
 //myFona.init();
 int ret;
 do{
 	ret = myFona.connect("ibox.tim.it",NULL,NULL);
 } while(ret!=0);
 minar::Scheduler::postCallback(resolve_dns).delay(minar::milliseconds(8000));
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


static void mmaInit(){
	uint8_t ret = acc.getWhoAmI();
	DEBUG_PRINT("Ret: %d\n",ret);
	//acc.setPotholeInterrupt();
	DEBUG_PRINT("Pothole interrupt set\n");
	//float x = acc.getAccX();
	//printf("X: %f\n", x);
	/*printf("Accelerometer start\n");
	if(!acc.begin(0x1C)){
		printf("Birrozza\n");
	}
	else{
		printf("Birretta\n");
	}
	printf("Accelerometer begin\n");
	acc.setRange(MMA8451_RANGE_2_G);
	printf("Accelerometer range set\n");
	printf("Range: %d\n",2 << acc.getRange());*/
	
}


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
   butt.fall(dummy);
    minar::Scheduler::postCallback(blinky).period(minar::milliseconds(1000));
    //minar::Scheduler::postCallback(readMma).period(minar::milliseconds(300)).delay(minar::milliseconds(2000));
    BLE &ble = BLE::Instance();
    ble.init(bleInitComplete);
    minar::Scheduler::postCallback(fona_prova).delay(minar::milliseconds(2000));
    DEBUG_PRINT("Hello!\n");
    //minar::Scheduler::postCallback(mmaInit);
    
   // minar::Scheduler::postCallback(enterSTOP).delay(minar::milliseconds(10000));
   
   // minar::Scheduler::postCallback(periodicRead).delay(minar::milliseconds(4000)).period(minar::milliseconds(10000));
}



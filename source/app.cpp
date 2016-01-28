#include "mbed-drivers/mbed.h"
#include "sal-iface-serialmodem/FONA808.h"
#include "sockets/v0/TCPStream.h"
#include "sockets/v0/UDPSocket.h"
#include "sal-stack-lwip/lwipv4_init.h"
#include "ble/BLE.h"
#include "ble/services/iBeacon.h"

// 

using namespace mbed::Sockets::v0;
class Resolver {
private:
    TCPStream _sock;
    UDPSocket _dns;
    uint16_t* send;
public:
    Resolver() : _sock(SOCKET_STACK_LWIP_IPV4), _dns(SOCKET_STACK_LWIP_IPV4) {
        _sock.open(SOCKET_AF_INET4);
	_sock.setNagle(false);
 	_dns.open(SOCKET_AF_INET4);
       send = new uint16_t[64];
       for(int i =0; i< 64; i++){
	   send[i] = i;
	}
    }

    void onDNS(Socket *s, struct socket_addr addr, const char *domain) {
        (void) s;
        SocketAddr sa;
        char buf[16];
        sa.setAddr(&addr);
        sa.fmtIPv4(buf,sizeof(buf));
        printf("Resolved %s to %s\r\n", domain, buf);
        socket_error_t err = _sock.connect(sa, 25, TCPStream::ConnectHandler_t(this, &Resolver::onConnect));
        //return _sock.resolve(address,UDPSocket::DNSHandler_t(this, &Resolver::onDNS));
        //printf("\nerr: %d",err);
        printf("MBED: Socket Error: %s (%d)\r\n", socket_strerror(err), err);
    }

    void onReceive(Socket *s) {
        printf("\nOnReceive");
	uint16_t result[64];
	size_t size = 20;
	socket_error_t err = s->recv(result, &size);
        printf("MBED: Socket Error: %s (%d)\r\n", socket_strerror(err), err);
	printf("\nThe result is %d",result[0]);
    }

   
    void onConnect(TCPStream *s) {
  	printf("\nConnected!"); 
        s->setOnReadable(TCPStream::ReadableHandler_t(this, &Resolver::onReceive));
        s->setOnDisconnect(TCPStream::DisconnectHandler_t(this, &Resolver::onDisconnect));
        send[0] = 80;
        send[1] = 480;
        socket_error_t err = _sock.send(send, 64*sizeof(uint16_t));
        printf("MBED: Socket Error: %s (%d)\r\n", socket_strerror(err), err);
        //s->error_check(err);
  }

   void onDisconnect(TCPStream *s) {
        s->close();
       // printf("{{%s}}\r\n",(error()?"failure":"success"));
        printf("{{end}}\r\n");
    }

    socket_error_t connect(const char * address) {
        /*SocketAddr ipaddr;
        ipaddr.setAddr(SOCKET_AF_INET4, "52.29.108.135");
        printf("Connecting to %s...\r\n", address);
        socket_error_t err = _sock.connect(ipaddr, 25, TCPStream::ConnectHandler_t(this, &Resolver::onConnect));
        //return _sock.resolve(address,UDPSocket::DNSHandler_t(this, &Resolver::onDNS));
        //printf("\nerr: %d",err);
        printf("MBED: Socket Error: %s (%d)\r\n", socket_strerror(err), err);*/
    }
   
    socket_error_t resolve(const char * address) {
        printf("Resolving %s...\r\n", address);
        return _dns.resolve(address,UDPSocket::DNSHandler_t(this, &Resolver::onDNS));
    }
};


static void blinky(void) {
    static DigitalOut led(LED1);
    led=!led;
}

//static void dummy(void){
//}

/*static void enterSTOP(void){
  static InterruptIn butt(USER_BUTTON);
  butt.fall(dummy);
  /* User push-button (External line 13) will be used to wakeup the system from STOP mode */
  //BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI);

  /* Enable Power Clock */
//  __HAL_RCC_PWR_CLK_ENABLE();
  
  /* Ensure that MSI is wake-up system clock */ 
/*  __HAL_RCC_WAKEUPSTOP_CLK_CONFIG(RCC_STOP_WAKEUPCLOCK_MSI);

   printf("Entering stop mode\n");
   HAL_Delay(3000);
   HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
   SetSysClock();
   printf("Woken up!\n");
}*/
//static RawSerial fona(D8,D2);

static void handler(void){
   // putc(fona.getc(),stdout);
}

static Resolver *r;

void resolve_dns(){
 printf("\nResolve DNS");
 lwipv4_socket_init();
 r = new Resolver();
 //r->connect("mbed.org");
 r->resolve("ec2-52-29-108-135.eu-central-1.compute.amazonaws.com");
/* r->connect("mbed.org");
 r->connect("mbed.org");*/
}


static iBeacon* ibeaconPtr;

/**
 * This function is called when the ble initialization process has failled
 */
void onBleInitError(BLE &ble, ble_error_t error)
{
    /* Initialization error handling should go here */
     printf("\nEzechiele 25-17");
}

/**
 * Callback triggered when the ble initialization process has finished
 */
void bleInitComplete(BLE::InitializationCompleteCallbackContext *params)
{
    printf("\nBLE init complete");
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

    /**
     * The Beacon payload has the following composition:
     * 128-Bit / 16byte UUID = E2 0A 39 F4 73 F5 4B C4 A1 2F 17 D1 AD 07 A9 61
     * Major/Minor  = 0x1122 / 0x3344
     * Tx Power     = 0xC8 = 200, 2's compliment is 256-200 = (-56dB)
     *
     * Note: please remember to calibrate your beacons TX Power for more accurate results.
     */
    static const uint8_t uuid[] = {0xE2, 0x0A, 0x39, 0xF4, 0x73, 0xF5, 0x4B, 0xC4,
                                   0xA1, 0x2F, 0x17, 0xD1, 0xAD, 0x07, 0xA9, 0x61};
    uint16_t majorNumber = 1122;
    uint16_t minorNumber = 3344;
    uint16_t txPower     = 0xC8;
    ibeaconPtr = new iBeacon(ble, uuid, majorNumber, minorNumber, txPower);

    ble.gap().setAdvertisingInterval(1000); /* 1000ms. */
    ble.gap().startAdvertising();
}


static FONA808 myFona(D8,D2,D12);


static void fona_prova(void){
  printf("Hello!\n");
 //myFona.init();
 myFona.connect("ibox.tim.it",NULL,NULL);
 minar::Scheduler::postCallback(resolve_dns).delay(minar::milliseconds(8000));
 
}


void app_start(int, char**) {
   minar::Scheduler::postCallback(blinky).period(minar::milliseconds(1000));
    BLE &ble = BLE::Instance();
    ble.init(bleInitComplete);
   // minar::Scheduler::postCallback(enterSTOP).delay(minar::milliseconds(10000));
   //minar::Scheduler::postCallback(fona_prova);

}



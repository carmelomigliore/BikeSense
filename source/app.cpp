#include "mbed-drivers/mbed.h"
#include "sal-iface-serialmodem/FONA808.h"
#include "sockets/v0/TCPStream.h"
#include "sockets/v0/UDPSocket.h"
#include "sal-stack-lwip/lwipv4_init.h"

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
    
   /* static Timer tim;
    static int started = 0;
    if(started == 0){
    	tim.start();
        started=1;
    }
    //rasp.baud(9600);
    printf("Malvagio... buahahahahahaha %d\n",tim.read_ms());
    /*char* dino = (char*)calloc(90000,sizeof(char));
    printf("Allocated 90000 chars on heap\n");
    dino[0] = 'D';
    dino[1] = 'I';
    dino[2] = 'O';
    dino[3] = 0;
    printf("%s\n",dino);
    free (dino);
    char bubba[30100];
    printf("Allocated 30100 chars on stack\n");
    bubba[0] = 'D';
    bubba[1] = 'I';
    bubba[2] = 'O';
    bubba[3] = 0;
    printf("%s\n",bubba);*/
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


static FONA808 myFona(D8,D2,D12);


static void fona_prova(void){
  printf("Hello!\n");
 //myFona.init();
 myFona.connect("ibox.tim.it",NULL,NULL);
 minar::Scheduler::postCallback(resolve_dns).delay(minar::milliseconds(8000));
 
}


void app_start(int, char**) {
   minar::Scheduler::postCallback(blinky).period(minar::milliseconds(1000));
   // minar::Scheduler::postCallback(enterSTOP).delay(minar::milliseconds(10000));
   minar::Scheduler::postCallback(fona_prova);

}



#include "mbed-drivers/mbed.h"
#include "ApplicationManager.h"

#ifdef DEBUGPRINT
#define DEBUG_PRINT(fmt, args...)    printf(fmt, ## args)
#else
#define DEBUG_PRINT(fmt, args...)    /* Don't do anything in release builds */
#endif


static void blinky(void) {
    static DigitalOut led(LED1);
    led=!led;
}

static ApplicationManager appmanager;  

void app_start(int, char**) { 
   minar::Scheduler::postCallback(blinky).period(minar::milliseconds(1000));
   appmanager.start();
}



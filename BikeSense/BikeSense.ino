#include <Arduino.h>
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "BluefruitConfig.h"
#include "Adafruit_FONA.h"
#include "LowPower.h"
#include <avr/wdt.h>


//The setup function is called once at startup of the sketch

#define FACTORYRESET_ENABLE      1 //bluefruit rst

/*#define FONA_RX 9
#define FONA_TX 6*/
#define FONA_RST 10
#define FONA_APN F("internet.wind")

Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);
Adafruit_FONA fona = Adafruit_FONA(FONA_RST);
HardwareSerial *fonaSerial = &Serial1;


void error(const __FlashStringHelper*err) {
	Serial.println(err);
	while (1);
}

void setup()
{
	Serial.begin(115200);
	// Add your initialization code here

	if ( !ble.begin(VERBOSE_MODE) ){
		error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
	}
	Serial.println( F("OK!") );

	if ( FACTORYRESET_ENABLE ){
		/* Perform a factory reset to make sure everything is in a known state */
		Serial.println(F("Performing a factory reset: "));
		if ( ! ble.factoryReset() ){
			error(F("Couldn't factory reset"));
		}
	}
	ble.echo(false);

	fonaSerial->begin(4800);
	if (! fona.begin(*fonaSerial)) {
		error(F("FONA NOT FOUND"));
	}

	uint8_t type = fona.type();
	  Serial.println(F("FONA is OK"));
	  Serial.print(F("Found "));
	  switch (type) {
	    case FONA800L:
	      Serial.println(F("FONA 800L")); break;
	    case FONA800H:
	      Serial.println(F("FONA 800H")); break;
	    case FONA808_V1:
	      Serial.println(F("FONA 808 (v1)")); break;
	    case FONA808_V2:
	      Serial.println(F("FONA 808 (v2)")); break;
	    case FONA3G_A:
	      Serial.println(F("FONA 3G (American)")); break;
	    case FONA3G_E:
	      Serial.println(F("FONA 3G (European)")); break;
	    default:
	      Serial.println(F("???")); break;
	  }

	  // Print module IMEI number.
	  char imei[15] = {0}; // MUST use a 16 character buffer for IMEI!
	  uint8_t imeiLen = fona.getIMEI(imei);
	  if (imeiLen > 0) {
	    Serial.print("Module IMEI: "); Serial.println(imei);
	  }


	fona.enableGPS(true);

	Serial.println(F("Checking for network..."));
	while (fona.getNetworkStatus() != 1) {
		delay(500);
		Serial.println(F("Not registered to network yet..."));
	}

	fona.setGPRSNetworkSettings(FONA_APN,F(""),F(""));
	delay(2000);

	Serial.println(F("Disabling GPRS"));
	fona.enableGPRS(false);
	delay(2000);

	if (!fona.enableGPRS(true)) {
	    error(F("Failed to turn GPRS on :("));
	  }

	Serial.println(F("Connected to Cellular!"));

}

// The loop function is called in an endless loop
void loop()
{
	Serial.println("loop");
	uint16_t statuscode;
	int16_t length;
	char url[]="www.adafruit.com/testwifi/index.html";
	Serial.println(F("****"));
	if (!fona.HTTP_GET_start(url, &statuscode, (uint16_t *)&length)) {
		Serial.println("Failed!");
	}
	while (length > 0) {
		while (fona.available()) {
			char c = fona.read();
			Serial.write(c);
			length--;
			if (! length) break;
		}
	}
	Serial.println(F("\n****"));
	fona.HTTP_GET_end();
}

#include "LMP91000.h"

LMP91000::LMP91000(PinName sda, PinName scl) : i2c(sda,scl) {

}

uint8_t LMP91000::write(uint8_t reg, uint8_t data) {
 
	//i2c.start(); verificare se ci vuole start
	uint8_t tosend[2];
	tosend[0] = reg;
	tosend[1] = data;
	i2c.write(LMP91000_I2C_ADDRESS << 1, (const char*)tosend, 2);
	//i2c.write(LMP91000_I2C_ADDRESS << 1, (const char*)&data, 1, false);
      /*Wire.beginTransmission(LMP91000_I2C_ADDRESS);           // START+SLA+W
      Wire.write(reg);                                        // REG
      Wire.write(data);                                       // DATA
      Wire.endTransmission(true); // generate stop condition  // STOP*/

      // read back the value of the register
      return read(reg);
}

uint8_t LMP91000::read(uint8_t reg){
      uint8_t chr = 0;
      i2c.write(LMP91000_I2C_ADDRESS << 1, (const char*)&reg, 1, true);
      i2c.read(LMP91000_I2C_ADDRESS << 1, (char*)&chr,1);  
//      Wire.beginTransmission(LMP91000_I2C_ADDRESS);           // START+SLA+W
//      Wire.write(reg);                                        // REG
//      Wire.endTransmission(false);                            // REP START
//      Wire.requestFrom(LMP91000_I2C_ADDRESS, 1, true);        // SLA+R
//      if(Wire.available()){
//            chr = Wire.read();                                // DATA
//      }
      
      return chr;
}

uint8_t LMP91000::status(void) {
      return read(LMP91000_STATUS_REG);
}      
      
uint8_t LMP91000::lock(){ // this is the default state
      return write(LMP91000_LOCK_REG, LMP91000_WRITE_LOCK);
}

uint8_t LMP91000::unlock(){ 
      return write(LMP91000_LOCK_REG, LMP91000_WRITE_UNLOCK);
}

uint8_t LMP91000::configure(uint8_t _tiacn, uint8_t _refcn, uint8_t _modecn){
      if(status() == LMP91000_READY){
            unlock();
            write(LMP91000_TIACN_REG, _tiacn);
            write(LMP91000_REFCN_REG, _refcn);
            write(LMP91000_MODECN_REG, _modecn);
            lock();
            return 1;
      }
      return 0;
}


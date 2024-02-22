#include "ble.h"

Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);
int32_t serviceId;
int32_t reqSpeed;
int32_t reqState;
int32_t reqTorque;
int32_t reqAccel;
int32_t reqRegen;

int32_t resMotorTemp;
int32_t resInvTemp;
int32_t resTorque;
int32_t resSpeed;
int32_t resState;
int32_t resDcVolt;
int32_t resDcCurrent;

int32_t inThrottle;
int32_t inBrake;
int32_t output;
int32_t input;
int32_t status;

int32_t config1;
int32_t config2;
int32_t config3;

void Ble::setup() {

  if ( !ble.begin(VERBOSE_MODE) )
  {
    Logger::debug("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?");
  }

  /* Perform a factory reset to make sure everything is in a known state */
  Logger::debug("Performing a factory reset: ");
  if (! ble.factoryReset() ){
       Logger::debug("Couldn't factory reset");
  }

  /* Disable command echo from Bluefruit */
  ble.echo(false);

  Logger::debug("Requesting Bluefruit info:");
  /* Print Bluefruit information */
  ble.info();

  /* Change the device name to make it easier to find */
  Logger::debug("Setting device name to %s': pao EVCU");

  if (! ble.sendCommandCheckOK(F("AT+GAPDEVNAME=Pao EVCU")) ) {
    Logger::debug("Could not set device name?");
  }
  Ble::setup_main();
  Ble::setup_io();
  Ble::setup_config();

  ble.sendCommandCheckOK( F("AT+GAPSETADVDATA=02-01-06-05-02-0d-18-0a-18") );

  /* Reset the device for the new service setting changes to take effect */
  Serial.print(F("Performing a SW reset (service changes require a reset): "));
  ble.reset();

}

void Ble::updateValues(Ble::BleData *data) {
  Ble::sendValue(data->reqSpeed, reqSpeed);
  Ble::sendValue(data->reqState, reqState);
  Ble::sendValue(data->reqTorque, reqTorque);
  Ble::sendValue(data->reqAccel, reqAccel);
  Ble::sendValue(data->reqRegen, reqRegen);
  Ble::sendValue(data->resMotorTemp, resMotorTemp);
  Ble::sendValue(data->resInvTemp, resInvTemp);
  Ble::sendValue(data->resTorque, resTorque);
  Ble::sendValue(data->resSpeed, resSpeed);
  Ble::sendValue(data->resState, resState);
  Ble::sendValue(data->resDcVolt, resDcVolt);
  Ble::sendValue(data->resDcCurrent, resDcCurrent);

  byte inputByte = Ble::convertToBinary(
    data->inEnable,
    data->inReverse,
    0,0,0,0,0,0);

  byte outputByte = Ble::convertToBinary(
    data->outPreCon,
    data->outMainCon,
    data->outBrake,
    data->outCooling,
    data->outReverseLight,
    0,0,0);

  byte statusByte = Ble::convertToBinary(
    data->isFaulted,
    data->isRunning,
    data->isWarning,
    0,0,0,0,0);

  Ble::sendValue(data->inThrottle, inThrottle);
  Ble::sendValue(data->inBrake, inBrake);

  Ble::sendValue(inputByte, input);
  Ble::sendValue(outputByte, output);
  Ble::sendValue(statusByte, status);
  
  //TODO we can only have 30 characteristics to work with so we are limiting config to 5 (other 25 characteristics are taken)
  // Need to look into a new library which can support more characteristics
  //More values are avaiable in the data object so we can set these to any of the fields. *these are only used for debugging*  
  Ble::sendValue(data->configSpeedMax, config1);
  Ble::sendValue(data->configTorqueMax, config2);
  Ble::sendValue(data->configSpeedSlewRate, config3);
}


void Ble::setup_main() {

  /* Add the Service definition */
  /* Service ID should be 1 */
  Logger::debug("Adding the Service definition (UUID = 0x27B4): ");
  bool success = ble.sendCommandWithIntReply( F("AT+GATTADDSERVICE=UUID=0x27B4"), &serviceId);
  if (! success) {
    Logger::debug("Could not add service");
  }

  // DMOC REQUESTED
  ble.sendCommandWithIntReply( F("AT+GATTADDCHAR=UUID=0xFF01, PROPERTIES=0x10, MIN_LEN=1, MAX_LEN=5, VALUE=0"), &reqSpeed);
  ble.sendCommandWithIntReply( F("AT+GATTADDCHAR=UUID=0xFF02, PROPERTIES=0x10, MIN_LEN=1, MAX_LEN=5, VALUE=0"), &reqState);
  ble.sendCommandWithIntReply( F("AT+GATTADDCHAR=UUID=0xFF03, PROPERTIES=0x10, MIN_LEN=1, MAX_LEN=5, VALUE=0"), &reqTorque);
  ble.sendCommandWithIntReply( F("AT+GATTADDCHAR=UUID=0xFF04, PROPERTIES=0x10, MIN_LEN=1, MAX_LEN=5, VALUE=0"), &reqAccel);
  ble.sendCommandWithIntReply( F("AT+GATTADDCHAR=UUID=0xFF05, PROPERTIES=0x10, MIN_LEN=1, MAX_LEN=5, VALUE=0"), &reqRegen);

  // DMOC RECEIVED
  ble.sendCommandWithIntReply( F("AT+GATTADDCHAR=UUID=0xFF06, PROPERTIES=0x10, MIN_LEN=1, MAX_LEN=5, VALUE=0"), &resMotorTemp);
  ble.sendCommandWithIntReply( F("AT+GATTADDCHAR=UUID=0xFF07, PROPERTIES=0x10, MIN_LEN=1, MAX_LEN=5, VALUE=0"), &resInvTemp);
  ble.sendCommandWithIntReply( F("AT+GATTADDCHAR=UUID=0xFF08, PROPERTIES=0x10, MIN_LEN=1, MAX_LEN=5, VALUE=0"), &resTorque);
  ble.sendCommandWithIntReply( F("AT+GATTADDCHAR=UUID=0xFF09, PROPERTIES=0x10, MIN_LEN=1, MAX_LEN=5, VALUE=0"), &resSpeed);
  ble.sendCommandWithIntReply( F("AT+GATTADDCHAR=UUID=0xFF0A, PROPERTIES=0x10, MIN_LEN=1, MAX_LEN=5, VALUE=0"), &resState);
  ble.sendCommandWithIntReply( F("AT+GATTADDCHAR=UUID=0xFF0B, PROPERTIES=0x10, MIN_LEN=1, MAX_LEN=5, VALUE=0"), &resDcVolt);
  ble.sendCommandWithIntReply( F("AT+GATTADDCHAR=UUID=0xFF0C, PROPERTIES=0x10, MIN_LEN=1, MAX_LEN=5, VALUE=0"), &resDcCurrent);
}

void Ble::setup_io() {

  /* Add the Service definition */
  /* Service ID should be 1 */
  Logger::debug("Adding the Service definition (UUID = 0x27B5): ");
  bool success = ble.sendCommandWithIntReply( F("AT+GATTADDSERVICE=UUID=0x27B5"), &serviceId);
  if (! success) {
    Logger::debug("Could not add service");
  }


  // OUTPUT
  ble.sendCommandWithIntReply( F("AT+GATTADDCHAR=UUID=0xFF0D, PROPERTIES=0x10, MIN_LEN=1, MAX_LEN=9, VALUE=0"), &output);
  // INPUTS
  ble.sendCommandWithIntReply( F("AT+GATTADDCHAR=UUID=0xFF1D, PROPERTIES=0x10, MIN_LEN=1, MAX_LEN=9, VALUE=0"), &inThrottle);
  ble.sendCommandWithIntReply( F("AT+GATTADDCHAR=UUID=0xFF1E, PROPERTIES=0x10, MIN_LEN=1, MAX_LEN=9, VALUE=0"), &inBrake);
  ble.sendCommandWithIntReply( F("AT+GATTADDCHAR=UUID=0xFF0E, PROPERTIES=0x10, MIN_LEN=1, MAX_LEN=9, VALUE=0"), &input);
  // STATUS
  ble.sendCommandWithIntReply( F("AT+GATTADDCHAR=UUID=0xFF0F, PROPERTIES=0x10, MIN_LEN=1, MAX_LEN=9, VALUE=0"), &status);

}

void Ble::setup_config() {
  /* Add the Service definition */
  /* Service ID should be 1 */
  Logger::debug("Adding the Service definition (UUID = 0x27B6): ");
  bool success = ble.sendCommandWithIntReply( F("AT+GATTADDSERVICE=UUID=0x27B6"), &serviceId);
  if (! success) {
    Logger::debug("Could not add service");
  }

  //CONFIG
  ble.sendCommandWithIntReply( F("AT+GATTADDCHAR=UUID=0xFF1A, PROPERTIES=0x10, MIN_LEN=1, MAX_LEN=5, VALUE=0"), &config1);
  ble.sendCommandWithIntReply( F("AT+GATTADDCHAR=UUID=0xFF1B, PROPERTIES=0x10, MIN_LEN=1, MAX_LEN=5, VALUE=0"), &config2);
  ble.sendCommandWithIntReply( F("AT+GATTADDCHAR=UUID=0xFF1C, PROPERTIES=0x10, MIN_LEN=1, MAX_LEN=5, VALUE=0"), &config3);

  ble.sendCommandCheckOK( F("AT+GAPSETADVDATA=02-01-06-05-02-0d-18-0a-18") );

  /* Reset the device for the new service setting changes to take effect */
  Serial.print(F("Performing a SW reset (service changes require a reset): "));
  ble.reset();
}

// PRIVATE



void Ble::sendValue(int value, int id) {
  ble.print( F("AT+GATTCHAR=") );
  ble.print( id, HEX );
  ble.print( F(",") );
  ble.println(value, HEX);
}

void Ble::sendValue(bool value, int id) {
  ble.print( F("AT+GATTCHAR=") );
  ble.print( id, HEX );
  ble.print( F(",") );
  ble.println(value, HEX);
}

void Ble::sendValue(byte value, int id) {
  ble.print( F("AT+GATTCHAR=") );
  ble.print( id, HEX );
  ble.print( F(",") );
  ble.println(value, HEX);
}

byte Ble::convertToBinary(bool in1, bool in2, bool in3, bool in4, bool in5, bool in6, bool in7, bool in8){
  byte output = 0;

  output |= in1 << 1;
  output |= in2 << 2;
  output |= in3 << 3;
  output |= in4 << 4;
  output |= in5 << 5;
  output |= in6 << 6;
  output |= in7 << 7;
  output |= in8 << 8;

  return output;
}

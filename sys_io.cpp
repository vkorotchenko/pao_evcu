/*
 * sys_io.cpp
 *
 * Handles the low level details of system I/O
 *
Copyright (c) 2013 Collin Kidder, Michael Neuweiler, Charles Galpin

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

some portions based on code credited as:
Arduino Due ADC->DMA->USB 1MSPS
by stimmer

*/

#include "sys_io.h"

#undef HID_ENABLED

SPISettings spi_settings(1000000, MSBFIRST, SPI_MODE3);    

SystemIO::SystemIO()
{
    useSPIADC = true;
    
    numDigIn = NUM_DIGITAL;
    numDigOut = NUM_OUTPUT;
    numAnaIn = NUM_ANALOG;
    numAnaOut = 0;
    
    adc2Initialized = false;
    adc3Initialized = false;
    lastInitAttempt = 0;
}

void SystemIO::setSystemType(SystemType systemType) {
        sysType = PAO_EVCU6;
}

SystemType SystemIO::getSystemType() {
    return sysType;
}

void SystemIO::setup() {
    int i;
    
    //the first order of business is to figure out what hardware we are running on and fill in
    //the pin tables.

    // TODO check pins
    Logger::info("Running on Feather M0 hardware");

    // PreCharge Relay // pref 17
    pinMode(PrechargeRelay, OUTPUT);
    SystemIO::setDigitalOutput(PrechargeRelay,0);

    // Main Relay  // pref 18
    pinMode(MainContactorRelay, OUTPUT);
    SystemIO::setDigitalOutput(MainContactorRelay, 0);

    // brake pedal (Analog) // pref A0/14/ BrakeADC
    pinMode(BrakeADC, INPUT);

    //gas pedal 1 (Analog) // pref A1/15 // ThrottleADC1
    pinMode(ThrottleADC1, INPUT);

    // gas pedal 2 (Analog) // pref A2/16 // ThrottleADC2
    pinMode(ThrottleADC2, INPUT);

    // enable (Digital) // pref 17// EnablePin
    if (EnableIn < 24 ) {
        pinMode(EnableIn, INPUT);
    }

    // reverse 2 (Digital) // pref 18 // ReverseIn
    if (ReverseIn < 24 ) {
        pinMode(ReverseIn, INPUT);
    }

    //Digital // CoolFan
    if (CoolFan < 24 ) {
        pinMode(CoolFan, OUTPUT);
    }

    //Digital // BrakeLight
    if (BrakeLight < 24 ) {
        pinMode(BrakeLight, OUTPUT);
    }

    //Digital // RevLight
    if (RevLight < 24 ) {
        pinMode(RevLight, OUTPUT);
    }

    // adjust ADC
    // analogReference(AR_INTERNAL_3_0);
    analogReadResolution(10);
}

int SystemIO::numDigitalInputs()
{
    return numDigIn;
}

int SystemIO::numDigitalOutputs()
{
    return numDigOut;
}

int SystemIO::numAnalogInputs()
{
    return numAnaIn;
}

int SystemIO::numAnalogOutputs()
{
    return numAnaOut;
}

/*
get value of one of the analog inputs
On PAO_EVCU6.2 or higher
Gets reading over SPI which is still pretty fast. The SPI connected chip is 24 bit
but too much of the code for PAO_EVCU uses 16 bit integers for storage so the 24 bit values returned
are knocked down to 16 bit values before being passed along.
*/
int16_t SystemIO::getAnalogIn(uint8_t pin) {
    int base;
    if (pin > MAX_PIN) {
        return 0;
    }

    return analogRead(pin);
}

boolean SystemIO::setAnalogOut(uint8_t which, int32_t level)
{
    if (which >= numAnaOut) return false; 
    return false;
}

int32_t SystemIO::getAnalogOut(uint8_t which)
{
    if (which >= numAnaOut) return 0;  
    return false;
}


//the new pack voltage and current functions however, being new, don't have legacy problems so they're 24 bit ADC.
int32_t SystemIO::getCurrentReading()
{
    int32_t valu;
    int64_t gainTemp;
    valu = getSPIADCReading(CS1, 0);
    valu -= (adc_comp[4].offset * 32);
    valu = valu >> 3;
    gainTemp = (int64_t)((int64_t)valu * adc_comp[4].gain) / 16384ll;
    valu = (int32_t) gainTemp;
    return valu;
}

int32_t SystemIO::getPackHighReading()
{
    int32_t valu;
    int64_t gainTemp;
    valu = getSPIADCReading(CS3, 1);
    valu -= (adc_comp[5].offset * 32);
    valu = valu >> 3; //divide by 8
    gainTemp = (int64_t)((int64_t)valu * adc_comp[5].gain) / 16384ll;
    valu = (int32_t) gainTemp;
    return valu;
}

int32_t SystemIO::getPackLowReading()
{
    int32_t valu;
    int64_t gainTemp;
    valu = getSPIADCReading(CS3, 2);
    valu -= (adc_comp[6].offset * 32);
    valu >>= 3;

    gainTemp = (int64_t)((int64_t)valu * adc_comp[6].gain) / 16384ll;
    valu = (int32_t) -gainTemp;    
    return valu;
}

//get value of one of the 4 digital inputs
boolean SystemIO::getDigitalIn(uint8_t pin) {
    
    if (pin < MAX_PIN) return !(digitalRead(pin));
    return false;
}

//set output high or not
void SystemIO::setDigitalOutput(uint8_t pin, boolean active) {
    Logger::info("SET DIGITAL:  pin %d  numDigOut %d MAX_PIN %d active %s", pin, numDigOut, MAX_PIN, active);
    
    if (pin < MAX_PIN)
    {
        if (active)
            digitalWrite(pin, HIGH);
        else digitalWrite(pin, LOW);
    }
}

//get current value of output state (high?)
boolean SystemIO::getDigitalOutput(uint8_t pin) {
    
    if (pin < NUM_OUTPUT)
    {
        if (pin == 255) return false;
        return digitalRead(pin);
    }
    return false;
}

int32_t SystemIO::getSPIADCReading(int CS, int sensor)
{
    int32_t result;
    int32_t byt;
    
    if (!isInitialized()) return 0;
    
    //Logger::debug("SPI Read CS: %i Sensor: %i", CS, sensor);
    SPI.beginTransaction(spi_settings);
    digitalWrite(CS, LOW);
    if (sensor == 0) SPI.transfer(ADE7913_READ | ADE7913_AMP_READING);
    if (sensor == 1) SPI.transfer(ADE7913_READ | ADE7913_ADC1_READING);
    if (sensor == 2) SPI.transfer(ADE7913_READ | ADE7913_ADC2_READING);
    byt = SPI.transfer(0);
    result = (byt << 16);
    byt = SPI.transfer(0);
    result = result + (byt << 8);
    byt = SPI.transfer(0);
    digitalWrite(CS, HIGH);
    SPI.endTransaction();
    
    result = result + byt;
    //now we've got the whole 24 bit value but it is a signed 24 bit value so we must sign extend
    if (result & (1 << 23)) result |= (255 << 24);
    return result;
}

SystemIO systemIO;



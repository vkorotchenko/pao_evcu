/*
 * PotThrottle.cpp
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

 */

#include "PotThrottle.h"

/*
 * Constructor
 */
PotThrottle::PotThrottle() : Throttle() {
    commonName = "Potentiometer (analog) accelerator";
}

/*
 * Setup the device.
 */
void PotThrottle::setup() {
    tickHandler.detach(this); // unregister from TickHandler first

    Logger::info("add device: PotThrottle (id: %X, %X)", POTACCELPEDAL, this);

    loadConfiguration();

    Throttle::setup(); //call base class

    //set digital ports to inputs and pull them up all inputs currently active low
    //pinMode(THROTTLE_INPUT_BRAKELIGHT, INPUT_PULLUP); //Brake light switch

    tickHandler.attach(this, CFG_TICK_INTERVAL_POT_THROTTLE);
}

/*
 * Process a timer event.
 */
void PotThrottle::handleTick() {
    Throttle::handleTick(); // Call parent which controls the workflow
}

/*
 * Retrieve raw input signals from the throttle hardware.
 */
RawSignalData *PotThrottle::acquireRawSignal() {
    PotThrottleConfiguration *config = (PotThrottleConfiguration *) getConfiguration();

    rawSignal.input1 = systemIO.getAnalogIn(config->AdcPin1);
    rawSignal.input2 = systemIO.getAnalogIn(config->AdcPin2);
    return &rawSignal;
}

/*
 * Perform sanity check on the ADC input values. The values are normalized (without constraining them)
 * and the checks are performed on a 0-1000 scale with a percentage tolerance
 */
bool PotThrottle::validateSignal(RawSignalData *rawSignal) {
    PotThrottleConfiguration *config = (PotThrottleConfiguration *) getConfiguration();
    int32_t calcThrottle1, calcThrottle2;

    calcThrottle1 = normalizeInput(rawSignal->input1, config->minimumLevel1, config->maximumLevel1);
    if (config->numberPotMeters == 1 && config->throttleSubType == 2) { // inverted
        calcThrottle1 = 1000 - calcThrottle1;
    }


    if (calcThrottle1 > (1000 + CFG_THROTTLE_TOLERANCE))
    {
        if (status == OK)
            Logger::debug(POTACCELPEDAL, "ERR_HIGH_T1: throttle 1 value out of range: %l", calcThrottle1);
        status = ERR_HIGH_T1;
        return false;
    }

    if (calcThrottle1 < (0 - CFG_THROTTLE_TOLERANCE)) {
        if (status == OK)
            Logger::debug(POTACCELPEDAL, "ERR_LOW_T1: throttle 1 value out of range: %l ", calcThrottle1);
        status = ERR_LOW_T1;
        return false;
    }

    if (config->numberPotMeters > 1) {
        calcThrottle2 = normalizeInput(rawSignal->input2, config->minimumLevel2, config->maximumLevel2);

        if (calcThrottle2 > (1000 + CFG_THROTTLE_TOLERANCE)) {
            if (status == OK)
                Logger::debug(POTACCELPEDAL, "ERR_HIGH_T2: throttle 2 value out of range: %l", calcThrottle2);
            status = ERR_HIGH_T2;
            return false;
        }

        if (calcThrottle2 < (0 - CFG_THROTTLE_TOLERANCE)) {
            if (status == OK)
                Logger::debug(POTACCELPEDAL, "ERR_LOW_T2: throttle 2 value out of range: %l", calcThrottle2);
            status = ERR_LOW_T2;
            return false;
        }

        if (config->throttleSubType == 2) {
            // inverted throttle 2 means the sum of the two throttles should be 1000
            if ( abs(1000 - calcThrottle1 - calcThrottle2) > ThrottleMaxErrValue) {
                if (status == OK)
                    Logger::debug(POTACCELPEDAL, "Sum of throttle 1 (%l) and throttle 2 (%l) exceeds max variance from 1000 (%l)",
                                  calcThrottle1, calcThrottle2, ThrottleMaxErrValue);
                status = ERR_MISMATCH;
                return false;
            }
        } else {
            if ((calcThrottle1 - ThrottleMaxErrValue) > calcThrottle2) { //then throttle1 is too large compared to 2
                if (status == OK)
                    Logger::debug(POTACCELPEDAL, "throttle 1 too high (%l) compared to 2 (%l)", calcThrottle1, calcThrottle2);
                status = ERR_MISMATCH;
                return false;
            }
            else if ((calcThrottle2 - ThrottleMaxErrValue) > calcThrottle1) { //then throttle2 is too large compared to 1
                if (status == OK)
                    Logger::debug(POTACCELPEDAL, "throttle 2 too high (%l) compared to 1 (%l)", calcThrottle2, calcThrottle1);
                status = ERR_MISMATCH;
                return false;
            }
        }
    }

    // all checks passed -> throttle is ok
    if (status != OK)
        if (status != ERR_MISC) Logger::info(POTACCELPEDAL, (char *)Constants::normalOperation);
    status = OK;
    return true;
}

/*
 * Convert the raw ADC values to a range from 0 to 1000 (per mille) according
 * to the specified range and the type of potentiometer.
 */
int16_t PotThrottle::calculatePedalPosition(RawSignalData *rawSignal) {
    PotThrottleConfiguration *config = (PotThrottleConfiguration *) getConfiguration();
    uint16_t calcThrottle1, calcThrottle2;

    calcThrottle1 = normalizeInput(rawSignal->input1, config->minimumLevel1, config->maximumLevel1);

    if (config->numberPotMeters > 1) {
        calcThrottle2 = normalizeInput(rawSignal->input2, config->minimumLevel2, config->maximumLevel2);
        if (config->throttleSubType == 2) // inverted
            calcThrottle2 = 1000 - calcThrottle2;
        calcThrottle1 = (calcThrottle1 + calcThrottle2) / 2; // now the average of the two
    }
    return calcThrottle1;
}

/*
 * Return the device ID
 */
DeviceId PotThrottle::getId() {
    return (POTACCELPEDAL);
}

/*
 * Load the device configuration.
 * If possible values are read from EEPROM. If not, reasonable default values
 * are chosen and the configuration is overwritten in the EEPROM.
 */
void PotThrottle::loadConfiguration() {
    PotThrottleConfiguration *config = (PotThrottleConfiguration *) getConfiguration();

    if (!config) { // as lowest sub-class make sure we have a config object
        config = new PotThrottleConfiguration();
        setConfiguration(config);
    }

    Throttle::loadConfiguration(); // call parent

        config->minimumLevel1 = Throttle1MinValue;
        config->maximumLevel1 = Throttle1MaxValue;
        config->minimumLevel2 = Throttle2MinValue;
        config->maximumLevel2 = Throttle2MaxValue;
        config->numberPotMeters = ThrottleNumPots;
        config->throttleSubType = ThrottleSubtype;
        config->AdcPin1 = ThrottleADC1;
        config->AdcPin2 = ThrottleADC2;
    
    Logger::debug(POTACCELPEDAL, "# of pots: %d       subtype: %d", config->numberPotMeters, config->throttleSubType);
    Logger::debug(POTACCELPEDAL, "T1 MIN: %l MAX: %l      T2 MIN: %l MAX: %l", config->minimumLevel1, config->maximumLevel1, config->minimumLevel2,
                  config->maximumLevel2);
}


/**
 * @brief Initialize motor pins
 */
void motorInit() {
    pinMode(STEP_POS_PIN, OUTPUT);
}


/**
 * @brief Checks ALARM_PLUS pin's analog voltage value, which changes when the motor alarms.
 *        The motor's alarm terminals change impedance from low to high when alarm is triggered.
 */
bool isMotorAlarming() {
    if (analogRead(ALARM_PLUS) > ALARM_THRESHOLD_VALUE) {
        Serial.print("Motor Alarm Read: ");
        Serial.println(analogRead(ALARM_PLUS));
        Serial.print("State: ");
        Serial.println(state);
        return true;
    }
    return false;
}

/**
 * @brief Checks the motor, sets alarm and turns motor off if it is
 * 
 * @return True if motor is alarming, false if all is well (to keep consistent with checkEstop())      
 */
bool checkMotor() {
    if (isMotorAlarming()) {
        turnMotorOff();
        setAlarmFault(MOTOR);
        return true;
    }

    return false;
}


/**
 * @brief Set the motor direction
 * @param dir direction of motor to set
 */
void setMotorDir(MotorDir dir) {
    if (dir == CW) {
        digitalWrite(DIR_POS_PIN, HIGH);
        global_motor_state = CW;
    } else {
        digitalWrite(DIR_POS_PIN, LOW);
        global_motor_state = CCW;
    }
}


/**
 * @brief converts given speed to PWM frequency
 * @param cm_per_sec given speed to be converted in cm/sec
 * @return uint32_t converted motor frequency
 */
inline uint32_t speedToFreq(float cm_per_sec) {
  return cm_per_sec * ((PULSES_PER_REV * GEARBOX_RATIO) / (2.0f * PI * REEL_RADIUS_CM));
}


/**
 * @brief Set the motor speed
 * @param cm_per_sec speed to set the motor to
 */
void setMotorSpeed(float cm_per_sec) {
    if (cm_per_sec < 0) {
        setMotorDir(CCW); //Down
    } else if (cm_per_sec > 0) {
        setMotorDir(CW);  //Up
    }
    setMotorFreq(speedToFreq(abs(cm_per_sec)));
}


/**
 * @brief Reset power to the motor
 */
void resetMotor(void) {
    P1.writeDiscrete(1, RELAY_SLOT, MOTOR_POWER);
    delay(1000);
    P1.writeDiscrete(0, RELAY_SLOT, MOTOR_POWER);
}


/**
 * @brief set motor speed to 0
 * 
 */

void turnMotorOff() {
    setMotorSpeed(0);
    global_motor_state = OFF;
}


/**
 * @brief Set the motor step pin output's frequency
 * @param frequency frequency to set the motor PWM to
 */
void setMotorFreq(uint32_t frequency) {
    //Disables timer clock, disabling output

    if (frequency == 0) {
        TC5->COUNT16.CTRLA.bit.ENABLE = 0;
        return;
    }

    // Enable clock for TC5
    GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID_TC4_TC5 |   // Timer TC5
                        GCLK_CLKCTRL_GEN_GCLK0 |   // Use GCLK0 (48MHz)
                        GCLK_CLKCTRL_CLKEN;
    while (GCLK->STATUS.bit.SYNCBUSY);

    // Reset TC5 before configuration
    TC5->COUNT16.CTRLA.reg = TC_CTRLA_SWRST;
    while (TC5->COUNT16.STATUS.bit.SYNCBUSY);

    // Configure TC5 with Waveform Generation Mode (e.g., Match Frequency)
    TC5->COUNT16.CTRLA.reg = TC_CTRLA_MODE_COUNT16 |  
                              TC_CTRLA_PRESCALER_DIV8| TC_CTRLA_ENABLE |
                            TC_CTRLA_WAVEGEN_MFRQ; // Match Frequency PWM 
    
    while (TC5->COUNT16.STATUS.bit.SYNCBUSY); 

    // Set compare value
    uint32_t compareVal = countValFromFreq(frequency);

    TC5->COUNT16.CC[0].reg = (uint16_t) compareVal;
    while (TC5->COUNT16.STATUS.bit.SYNCBUSY);

    // Enable interrupt on match
    enableTimerInterrupt();

    // Enable TC5
    TC5->COUNT16.CTRLA.bit.ENABLE = 1;
    while (TC5->COUNT16.STATUS.bit.SYNCBUSY);

    return;
}


/**
 * @brief Calculates value for timer count register based on given frequency
 * 
 * @param frequency requested output frequency
 * @return uint32_t corresponding timer count value for given frequency
 */
uint32_t countValFromFreq(uint32_t frequency) {
  return round(((SYSTEM_CLOCK_FREQ/(PRESCALER_VAL*2)) / frequency) - 1);
}


/**
 * @brief Enables TC5 interrupt
 */
void enableTimerInterrupt() {
    NVIC_EnableIRQ(TC5_IRQn);
    TC5->COUNT16.INTENSET.reg = TC_INTENSET_MC(1);  // Enable Match/Compare 0 interrupt
}


/**
 * @brief ISR handler for TC5 (Timer/Counter 5)
 */
void TC5_Handler() {
    if (TC5->COUNT16.INTFLAG.bit.MC0) {  // Match/Compare 0 interrupt
        TC5->COUNT16.INTFLAG.reg = TC_INTFLAG_MC(1);  // Clear interrupt flag

        if (!estop_pressed || state == MOTOR_CONTROL) {
            toggle = !toggle;
            digitalWrite(STEP_POS_PIN, toggle);

            if (toggle && global_motor_state == CW) { //Raising
              motor_pulses--;
            } else if (toggle && global_motor_state == CCW) {//Lowering
              motor_pulses++;
            }
        }
    }
}


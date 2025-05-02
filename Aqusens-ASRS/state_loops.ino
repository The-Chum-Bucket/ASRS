/* State Machine Functions ******************************************************/

/**
 * @brief CALIBRATE
 * 
 * No selection options
 */
void calibrateLoop() {
  resetMotor();
  resetLCD();

  updateSolenoid(CLOSED, SOLENOID_ONE);
  updateSolenoid(CLOSED, SOLENOID_TWO);

  lcd.setCursor(0, 0);
  lcd.print("CALIBRATING...");

  if (checkEstop()) {
    setAlarmFault(ESTOP);
    return;
  }  
   

  homeTube();
  state = STANDBY;
}

/**
 * @brief STANDBY
 * 
 * Two selection options:
 *    - Settings: proceeds to first page of settings page
 *    - Run Sample: proceeds to "Are you sure?" screen for manually running sample
 * Checks for E-Stop press
 */
void standbyLoop()
{ 
  static char key;
  static uint8_t key_pressed;
  lcd.clear();
  cursor_y = 2;

  while (state == STANDBY) 
  {
    checkEstop();

    standbyLCD();

    key_pressed = cursorSelect(2, 3);

    if (key_pressed == 'S') {
      if (cursor_y == 2) {
        settings_page = 1;
        state = SETTINGS;
      }

      else if (cursor_y == 3) {
        state = ENSURE_SAMPLE_START;
      }
    }
  }
}

/**
 * @brief ENSURE_SAMPLE_START
 * 
 * Two selection options:
 *    - Exit: returns to standby mode
 *    - Run Sample: proceeds to release mode
 */
void ensureSampleStartLoop() {
  char key_pressed;
  resetLCD();
  cursor_y = 3;

  while (state == ENSURE_SAMPLE_START) {
    ensureLCD("RUN SAMPLE");

    key_pressed = cursorSelect(2, 3);

    if (key_pressed == 'S') {
      if (cursor_y == 2) {
        state = RELEASE;
      }
      else if (cursor_y == 3) {
        state = STANDBY;
      }
    }
  }
}

/**
 * @brief RELEASE
 * 
 * No selection options
 * Checks for E-Stop press
 */
void releaseLoop() {

  // resetLCD();
  // static char pos[6];
  

  drop_distance_cm = getDropDistance();

  // drop_distance_cm = 30; // FIXME:MANUALLY SET TO 10 FOR DEBUG PURPOSES


  // actually drop the tube
  while (state == RELEASE){
    // snprintf(pos, sizeof(pos), "%.2fm", PULSES_TO_DISTANCE(motor_pulses));
    // releaseLCD(pos);

    if (isMotorAlarming()) setAlarmFault(MOTOR);

    if (dropTube(drop_distance_cm)) {
      state = SOAK;
    }
  }
}

/**
 * @brief SOAK
 * 
 * No selection options
 * Checks for E-Stop press
 */
void soakLoop() {
  char sec_time[3]; // "00"
  char min_time[3]; // "00"

  resetLCD();

  uint32_t curr_time = millis();
  uint32_t end_time = curr_time + (60 * soak_time.Minute * 1000) + (soak_time.Second * 1000);

  int seconds_remaining, minutes_remaining;

  while (state == SOAK && millis() < end_time) {
    checkEstop();

    // Calculate remaining time, accounting for millis() overflow
    uint32_t millis_remaining;
    if (end_time > millis()) {
      millis_remaining = end_time - millis();
    } else {
      // Handle overflow case
      millis_remaining = (UINT32_MAX - millis()) + end_time;
    }

    seconds_remaining = millis_remaining / 1000;
    minutes_remaining = seconds_remaining / 60;

    // Format seconds with leading zero if necessary
    if (seconds_remaining % 60 > 9) {
      snprintf(sec_time, sizeof(sec_time), "%i", seconds_remaining % 60);
    } else {
      snprintf(sec_time, sizeof(sec_time), "0%i", seconds_remaining % 60);
    }

    // Format minutes with leading zero if necessary
    if (minutes_remaining > 9) {
      snprintf(min_time, sizeof(min_time), "%i", minutes_remaining);
    } else {
      snprintf(min_time, sizeof(min_time), "0%i", minutes_remaining);
    }

    // Update LCD with remaining time
    soakLCD(min_time, sec_time, seconds_remaining % 4);
  }

  state = RECOVER;
}

/**
 * @brief RECOVER
 * 
 * No selection options
 * Checks for E-Stop press
 */
void recoverLoop() {
  char pos[6];

  resetLCD();

  while (state == RECOVER) {
    // checkEstop();
    if (isMotorAlarming()) setAlarmFault(MOTOR);
    

    if (retrieveTube(0)) { //Return to 0 distance, or the "home" state
      state = SAMPLE;
    }

    // snprintf(pos, sizeof(pos), "%.2fm", tube_position_f / 100.0f);
    // recoverLCD(pos);
  }
}

/**
 * @brief SAMPLE
 * 
 * No selection options
 */
void sampleLoop() {
  resetLCD();

  

  unsigned long start_time = millis();
  unsigned long curr_time = start_time;

  // TODO: wrap this in a meaningful function name (inline?)
  sendToPython("S");
  // COMMENTED OUT FOR TESTING PURPOSES
  // sendToPython("S");

  while (state == SAMPLE) 
  {
    sampleLCD();


    if (curr_time >= start_time + TOPSIDE_COMP_COMMS_TIMEOUT_MS) // Err: timeout due to lack of reply from topside computer
    {
      setAlarmFault(TOPSIDE_COMP_COMMS);
      continue;
    }

     if (Serial.available()) {
       String data = Serial.readStringUntil('\n'); // Read full line

       // check if wanting temperature read
       if (data == "T") {
         sendToPython(String((int)(readRTD(TEMP_SENSOR_ONE))));
       }

       // only transition to flushing after Aqusens sample done
       else {
         if (data == "D") { 
           state = FLUSH_TUBE;
         }
       }

       // Flush any remaining characters
       while (Serial.available()) {
           Serial.read();  // Discard extra data
       }
     }

    curr_time = millis();

    //  if (Serial.available()) {
    //    String data = Serial.readStringUntil('\n'); // Read full line

    //    // check if wanting temperature read
    //    if (data == "T") {
    //      sendToPython(String((int)(readRTD(TEMP_SENSOR_ONE))));
    //    }

    //    // only transition to flushing after Aqusens sample done
    //    else {
    //      if (data == "D") {  
    //        state = FLUSH_TUBE;
    //      }
    //    }

    //    // Flush any remaining characters
    //    while (Serial.available()) {
    //        Serial.read();  // Discard extra data
    //    }
    //  }
    state = FLUSH_TUBE;
    // // TODO: if pc_signal
    // if (Serial.available()) {
    //   state = FLUSH_TUBE;
    // }
  }
}

/**
 * @brief FLUSH
 * 
 * No selection options
 */
void tubeFlushLoop() {
  resetLCD();
  bool temp_flag = true;
  char sec_time[3]; // "00"
  char min_time[3]; // "00"

  uint32_t curr_time = millis();
  // uint32_t end_time = curr_time + (60 * tube_flush_time.Minute * 1000) + (tube_flush_time.Second * 1000);
  uint32_t end_time = curr_time + (60 * 0 * 1000) + (15 * 1000);

  int seconds_remaining, minutes_remaining;
  uint32_t last_toggle_time = curr_time; // Track the last time temp_flag was toggled

  while (state == FLUSH_TUBE) {
    checkEstop();

    // COMMENTED OUT FOR TESTING PURPOSES
    // if done flushing, exit loop
    // if (flushTube()) {
    //   state = DRY;
    //   break;
    // }

    flushSystem();
    state = DRY;

  //   // Calculate remaining time, accounting for millis() overflow
  //   uint32_t millis_remaining;
  //   if (end_time > millis()) {
  //     millis_remaining = end_time - millis();
  //   } else {
  //     // Handle overflow case
  //     millis_remaining = (UINT32_MAX - millis()) + end_time;
  //   }

  //   seconds_remaining = millis_remaining / 1000;
  //   minutes_remaining = seconds_remaining / 60;

  //   // Format seconds with leading zero if necessary
  //   if (seconds_remaining % 60 > 9) {
  //     snprintf(sec_time, sizeof(sec_time), "%i", seconds_remaining % 60);
  //   } else {
  //     snprintf(sec_time, sizeof(sec_time), "0%i", seconds_remaining % 60);
  //   }

  //   // Format minutes with leading zero if necessary
  //   if (minutes_remaining > 9) {
  //     snprintf(min_time, sizeof(min_time), "%i", minutes_remaining);
  //   } else {
  //     snprintf(min_time, sizeof(min_time), "0%i", minutes_remaining);
  //   }

  //   // Toggle temp_flag every second
  //   if (millis() - last_toggle_time >= 1000) {
  //     temp_flag = true; // Toggle temp_flag
  //     last_toggle_time = millis(); // Update the last toggle time
  //   }

  //   // Update LCD with remaining time
  //   flushLCD(min_time, sec_time, seconds_remaining % 4, temp_flag);

  //   if (temp_flag)
  //     temp_flag = false;
   }

  // TODO: send all the temperature buffer to pc to log

}

/**
 * @brief DRY
 * 
 * No selection options
 */
void dryLoop() {
  // turn off Aqusens pump before transitioning to dry state
  unsigned long start_time = millis();
  unsigned long curr_time = start_time;
  char sec_time[3]; // "00"
  char min_time[3]; // "00"
  uint32_t millis_remaining;
  int seconds_remaining, minutes_remaining;

  resetLCD();
  liftupTube();
  uint32_t end_time = curr_time + (60 * dry_time.Minute * 1000) + (dry_time.Second * 1000);

  while (state == DRY && curr_time < millis()) {
    checkEstop();

    if (start_time + TOPSIDE_COMP_COMMS_TIMEOUT_MS > millis()) { //If timeout ocurred...
      if (debug_ignore_timeouts)
        state = STANDBY;
      else 
        setAlarmFault(TOPSIDE_COMP_COMMS);
      
      continue;
    }

    // Calculate remaining time, accounting for millis() overflow
    if (end_time > millis()) {
      millis_remaining = end_time - millis();
    } 
    else {
      // Handle overflow case
      millis_remaining = (UINT32_MAX - millis()) + end_time;
    }

    seconds_remaining = millis_remaining / 1000;
    minutes_remaining = seconds_remaining / 60;

    // Format seconds with leading zero if necessary
    if (seconds_remaining % 60 > 9) {
      snprintf(sec_time, sizeof(sec_time), "%i", seconds_remaining % 60);
    } 
    else {
      snprintf(sec_time, sizeof(sec_time), "0%i", seconds_remaining % 60);
    }

    // Format minutes with leading zero if necessary
    if (minutes_remaining > 9) {
      snprintf(min_time, sizeof(min_time), "%i", minutes_remaining);
    } 
    else {
      snprintf(min_time, sizeof(min_time), "0%i", minutes_remaining);
    }

    // Update LCD with remaining time
    dryLCD(min_time, sec_time, seconds_remaining % 4); 
  }

  // bring tube home
  unliftTube();

  state = STANDBY;

  }


  /**
 * @brief ALARM
 * 
 * Two selection options:
 *    - Exit: returns to standby mode if alarm is resolved
 *            otherwise, flashes warning
 *    - Manual Mode: proceeds to manual mode
 */
  void alarmLoop() {
    turnMotorOff();
    char key;
    uint8_t key_pressed;
    lcd.clear();
    cursor_y = 2;
    while (state == ALARM) 
    {
      alarmLCD();
      key_pressed = cursorSelect(2, 3);

      if (key_pressed == 'S') {
        if (cursor_y == 3 && !checkEstop()) {
          state = CALIBRATE;
        }

        else if (cursor_y == 3 && checkEstop()) {
          lcd.clear();
          releaseEstopLCD();
          delay(1500);
          lcd.clear();
        }

        else if (cursor_y == 2) {
          state = MANUAL;
        }
      }
    }
  }

  /**
 * @brief MANUAL
 * 
 * Three selection options:
 *    - Motor: proceeds to MOTOR_CONTROL state
 *    - Solenoids: proceeds to SOLENOID_CONTROL state
 *    - Exit: returns to previous alarm state
 */
  void manualLoop() {
    char key;
    uint8_t key_pressed;
    lcd.clear();
    cursor_y = 1;
    while (state == MANUAL) {
      manualLCD();
      key_pressed = cursorSelect(1, 3);

      if (key_pressed == 'S') {
        if (cursor_y == 1) {
          state = MOTOR_CONTROL;
        } 

        else if (cursor_y == 2) {
          state = SOLENOID_CONTROL;
        }

        else if (cursor_y == 3) {
          setAlarmFault(ESTOP);
        }
      }
    }
  }

  /**
 * @brief MOTOR_CONTROL
 * 
 * Four selection options:
 *    - Reset: cycles motor power to reset any alarm
 *    - Raise: slowly raises motor in increments
 *    - Lower: slowly lowers motor in increments
 *    - Back: returns to manual mode
 */

  void motorControlLoop() {
    motorControlLCD();
    updateMotorCurrPositionDisplay(OFF);
    char key;
    uint8_t key_pressed;
    uint8_t last_key_pressed;

    lcd.clear();
    cursor_y = 1;

    while (state == MOTOR_CONTROL) {
      motorControlLCD();


      key_pressed = getKeyTimeBasedDebounce();

      if (key_pressed == 'L') {
        state = MANUAL;
      }

      else if (key_pressed == 'S') {
        resetMotor();
      }

      else if (key_pressed == 'D') {
        while (pressAndHold('D') == 'D') {
          setMotorSpeed(-10);
          updateMotorCurrPositionDisplay(CCW);
          // Serial.println("D");
        }
        turnMotorOff();
        updateMotorCurrPositionDisplay(OFF);
      }

      else if (key_pressed == 'U') {
        while (!magSensorRead() && pressAndHold('U') == 'U') {
          if (magSensorRead()) 
            turnMotorOff();
          else
            setMotorSpeed(10);
          //Move motor up 1cm
          updateMotorCurrPositionDisplay(CW);
          // while(!retrieveTube(1)); // Raises tube 
          // Serial.println("U");
        }
        turnMotorOff();
        updateMotorCurrPositionDisplay(OFF);
      }
    }
  }

  /**
 * @brief SOLENOID_CONTROL
 * 
 * Three selection options:
 *    - Solenoid 1: switches Solenoid 1 relay
 *    - Solenoid 2: switches Solenoid 2 relay
 *    - Exit: returns to manual mode
 */
  void solenoidControlLoop() {
    lcd.clear();
    char key_pressed;
    cursor_y = 1;

    while (state == SOLENOID_CONTROL) {
      solenoidControlLCD();
      key_pressed = cursorSelect(1, 2);

      if (key_pressed == 'S') {
        if (cursor_y == 1) {
          if (solenoid_one_state == OPEN) {
            updateSolenoid(CLOSED, SOLENOID_ONE);
          } else {
            updateSolenoid(OPEN, SOLENOID_ONE);
          }
        } else if (cursor_y == 2) {
          if (solenoid_two_state == OPEN) {
            updateSolenoid(CLOSED, SOLENOID_TWO);
          } else {
            updateSolenoid(OPEN, SOLENOID_TWO);
          }
        } 
      } else if (key_pressed == 'L') {
        state = MANUAL;
      }
    }
  }

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
    return;
  }  
   

  homeTube(); 
  liftupTube();
  delay(5*1000);
  unliftTube();

  if (state != ALARM) {
    state = STANDBY;
  }
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
  // drop_distance_cm = getDropDistance();

  drop_distance_cm = 10; // FIXME:MANUALLY SET TO 10 FOR DEBUG PURPOSES


  // actually drop the tube
  while (state == RELEASE){
    if (checkMotor() || checkEstop())
      continue; //Continue to next iteration of the loop, where state will be checked again (and will be ALARM)

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
    checkMotor();

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
    // if (seconds_remaining % 60 > 9) {
    //   snprintf(sec_time, sizeof(sec_time), "%i", seconds_remaining % 60);
    // } else {
    //   snprintf(sec_time, sizeof(sec_time), "0%i", seconds_remaining % 60);
    // }

    // // Format minutes with leading zero if necessary
    // if (minutes_remaining > 9) {
    //   snprintf(min_time, sizeof(min_time), "%i", minutes_remaining);
    // } else {
    //   snprintf(min_time, sizeof(min_time), "0%i", minutes_remaining);
    // }
    snprintf(sec_time, sizeof(sec_time), "%02i", seconds_remaining % 60);
    snprintf(min_time, sizeof(min_time), "%02i", minutes_remaining);


    // Update LCD with remaining time
    soakLCD(min_time, sec_time);
    printDots(seconds_remaining);
  }

  if (state != ALARM) 
    state = RECOVER;
}

/**
 * @brief RECOVER
 * 
 * No selection options
 * Checks for E-Stop press
 */
void recoverLoop() {
  resetLCD();

  while (state == RECOVER) {
    // checkEstop();
    if (checkMotor() || checkEstop()) //If motor is alarming or estop is pressed, continue to next iter
      continue;
    

    if (retrieveTube(0)) { //Return to 0 distance, or the "home" state
      state = SAMPLE;
    }
  }
}

/**
 * @brief SAMPLE
 * 
 * No selection options
 */
void sampleLoop() {
  pumpControl(START_PUMP, 0, NULL_STAGE);


  preSampleLCD();
  resetLCD();

  unsigned long start_time = millis();
  unsigned long curr_time = start_time;
  unsigned long last_lcd_update = 0;
  unsigned long end_time = start_time + (1000 * SAMPLE_TIME_SEC);

  
  // TODO: wrap this in a meaningful function name (inline?)
  
  
  if (state == SAMPLE) {
    sendToPython("S");
    delay(500); //Delay for a half second, wait for message to be processed...
    sendToPython(String(SAMPLE_TIME_SEC));
  }

  while (state == SAMPLE) 
  {
    if (last_lcd_update == 0 || curr_time - last_lcd_update > 1000) {
      sampleLCD(end_time);
      last_lcd_update = curr_time;
    }
    

    if (curr_time >= start_time + (1000 * SAMPLE_TIME_SEC) + TOPSIDE_COMP_COMMS_TIMEOUT_MS) // Err: timeout due to lack of reply from topside computer
    {
      Serial.println("sample state err!");
      if (debug_ignore_timeouts)
        state = FLUSH_SYSTEM;
      else 
        setAlarmFault(TOPSIDE_COMP_COMMS);
      continue;
    }

     if (Serial.available()) {
       String data = Serial.readStringUntil('\n'); // Read full line

       // check if wanting temperature read
       if (data == "T") {
         sendToPython(String((int)(readRTD(SAMPLE_TEMP_SENSOR))));
       }

       // only transition to flushing after Aqusens sample done
       else {
         if (data == "D") { 
           sendToPython("F"); 
           state = FLUSH_SYSTEM;
         }
       }

       // Flush any remaining characters
       while (Serial.available()) {
           Serial.read();  // Discard extra data
       }
     }

    curr_time = millis();
  }
}

/**
 * @brief Runs the system flush procedure as a finite state machine with discrete stages.
 * 
 * The flush process includes dumping the sample, air bubble, line flush,
 * freshwater device flush, air flush, and final homing of the tube.
 * Adjust timings and durations in config.h.
 */

void flushSystemLoop() {
  resetLCD();
  int total_flush_time_ms = 1000 * (LIFT_TUBE_TIME_S + 
                                    AIR_BUBBLE_TIME_S + 
                                    FLUSH_LINE_TIME_S +
                                    FRESHWATER_TO_DEVICE_TIME_S + 
                                    FRESHWATER_FLUSH_TIME_S +
                                    FINAL_AIR_FLUSH_TIME_S + 
                                   (FLUSHING_TIME_BUFFER));

  bool stagesStarted[] = {false, false, false, false, false, false};
  // Represents the start of each stage for FSM
  // Represents: Dumping the sample, Air Bubble, Line Flush, Freshwater Device Flush, Final Air Flush, and home tube
  
  FlushStage curr_stage = DUMP_SAMPLE;
  closeAllSolenoids();

  unsigned long curr_time = millis();
  unsigned long last_lcd_update_time = curr_time;
  unsigned long end_time = curr_time + total_flush_time_ms;
  unsigned long curr_stage_end_time = 0;

  updateFlushTimer(end_time, curr_stage);
  

  while (state == FLUSH_SYSTEM) {

    ///////////// ASK ROB AND ALEXIS ABOUT THIS ////////////////////
    
    // if (checkEstop()) {
    //   closeAllSolenoids();
    //   pumpControl(STOP_PUMP, end_time, curr_stage);
    // }

    ////////////////////////////////////////////////////////////////
    
    if (checkMotor()) { //If motor is alarming, continue to break out of while loop (state will be changed by call)
      continue;
    }

    switch(curr_stage) {
      case DUMP_SAMPLE:
        if (stagesStarted[DUMP_SAMPLE] == false) {
          //Serial.println("STARTING DUMP");
          liftupTube();
          stagesStarted[DUMP_SAMPLE] = true;
          curr_stage_end_time = curr_time + (1000 * LIFT_TUBE_TIME_S);
        }
        if (curr_time >= curr_stage_end_time) {
          // Lift tube state is done
          unliftTube();
          curr_stage = AIR_BUBBLE;
        }
        break;

      case AIR_BUBBLE:
      if (stagesStarted[AIR_BUBBLE] == false) {
        //Serial.println("STARTING AIR BUBBLE");
        pumpControl(START_PUMP, end_time, curr_stage);
        stagesStarted[AIR_BUBBLE] = true;
        curr_stage_end_time = curr_time + (1000* AIR_BUBBLE_TIME_S);
      }
      if (curr_time >= curr_stage_end_time) {
        pumpControl(STOP_PUMP, end_time, curr_stage);
        curr_stage = FRESHWATER_LINE_FLUSH;
      }
        break;

      case FRESHWATER_LINE_FLUSH:
        if (stagesStarted[FRESHWATER_LINE_FLUSH] == false) {
          //Serial.println("STARTING FRESHWATER LINE FLUSH");
          dropTube(LINE_FLUSH_DROP_DIST_CM); //Drop down for line flush
          updateSolenoid(OPEN, SOLENOID_ONE); //Flush line
          homeTube(end_time, curr_stage);
          stagesStarted[FRESHWATER_LINE_FLUSH] = true;
          curr_stage_end_time = curr_time + (1000 * FLUSH_LINE_TIME_S);
        }
        if (curr_time >= curr_stage_end_time) {
          updateSolenoid(CLOSED, SOLENOID_ONE);
          curr_stage = FRESHWATER_DEVICE_FLUSH;
        }
        break;

      case FRESHWATER_DEVICE_FLUSH:
        if (stagesStarted[FRESHWATER_DEVICE_FLUSH] == false) {
          //Serial.println("STARTING FRESHWATER DEVICE FLUSH");
          liftupTube();
          end_time += flushDevice(); //Update end time if there was any waiting for flushing water to cool down.
          pumpControl(START_PUMP, end_time, curr_stage);
          stagesStarted[FRESHWATER_DEVICE_FLUSH] = true;
          curr_stage_end_time = curr_time + (1000 * (FRESHWATER_TO_DEVICE_TIME_S + FRESHWATER_FLUSH_TIME_S));
        }
        if (curr_time >= curr_stage_end_time) {
          closeAllSolenoids();
          unliftTube();
          curr_stage = AIR_FLUSH;
        }
        break;

      case AIR_FLUSH:
        if (stagesStarted[AIR_FLUSH] == false) {
          //Serial.println("STARTING AIR FLUSH");
          //pumpControl(START_PUMP, end_time, curr_stage);
          stagesStarted[AIR_FLUSH] = true;
          curr_stage_end_time = curr_time + (1000 * FINAL_AIR_FLUSH_TIME_S);
        }
        if (curr_time >= curr_stage_end_time) {
          pumpControl(STOP_PUMP, end_time, curr_stage);
          curr_stage = HOME_TUBE;
        }
        break;
      case HOME_TUBE:
        if (stagesStarted[HOME_TUBE] == false) {
          //Serial.println("STARTING FINAL HOMING");
          //dropTube(2); //Drop 3 cm, want to overshoot the magnet
          homeTube(end_time, curr_stage);
          stagesStarted[HOME_TUBE] = true;
          state = DRY;
        }
        break;

      default:
        break;
    }

    curr_time = millis();
    checkEstop();
    updateFlushTimer(end_time, curr_stage);
  }
}



/**
 * @brief Executes the drying process after flushing.
 * 
 * Lifts the tube, displays countdown timer for drying duration,
 * checks for E-Stop, then transitions to standby.
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

  while (state == DRY && end_time > millis()) {
    checkEstop();

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

    snprintf(sec_time, sizeof(sec_time), "%02i", seconds_remaining % 60);
    snprintf(min_time, sizeof(min_time), "%02i", minutes_remaining);

    // Update LCD with remaining time
    dryLCD(min_time, sec_time); 
    printDots(seconds_remaining);

  }

  unliftTube();

  if (state != ALARM)
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
    int curr_speed = 0;

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
          // setMotorSpeed(-10);
          rampUpMotor(curr_speed, -MANUAL_CONTROL_MOTOR_SPEED);
          updateMotorCurrPositionDisplay(CCW);
          // Serial.println("D");
        }
        // turnMotorOff();
        rampDownMotor(curr_speed, 0);
        updateMotorCurrPositionDisplay(OFF);
      }

      else if (key_pressed == 'U') {
        while (!magSensorRead() && pressAndHold('U') == 'U') {
          if (magSensorRead()) 
            turnMotorOff();
          else
            rampUpMotor(curr_speed, MANUAL_CONTROL_MOTOR_SPEED);
          updateMotorCurrPositionDisplay(CW);
        }

        rampDownMotor(curr_speed, 0);
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

/**
 * @brief 
 * 
 * @file position.ino
 */


/**
 * @brief returns tube to home position at constant speed
 *        Re-homes the tube during initializing and calibrate state
 * 
 */

void homeTube() {
  setMotorSpeed(SAFE_RISE_SPEED_CM_SEC); // Slowly raise the tube up to home position
  
  while (!magSensorRead()) { //While the calculated position is greater than and the mag sensor is not sensing the magnet...
    //Update LCD?
  }

  motor_pulses = 0;

  turnMotorOff();
}

/**
 * @brief drops tube to given distance
 * 
 * 
 * @param distance_cm distance to drop in cm
 * @return true if the tube has finished dropping to given distance
 * @return false if the tube has not finished dropping
 */

bool dropTube(unsigned int distance_cm) {
  unsigned long start_time = millis();
  unsigned long curr_time = millis();
  unsigned long last_lcd_update = millis();
  char pos[6];

  uint32_t final_step_count = DISTANCE_TO_PULSES(abs(distance_cm));

  resetLCD();

  setMotorSpeed(-DROP_SPEED_CM_SEC/4);
  delay(100);
  setMotorSpeed(-DROP_SPEED_CM_SEC/2);
  delay(100);
  setMotorSpeed(-DROP_SPEED_CM_SEC);

  snprintf(pos, sizeof(pos), "%.2fm", PULSES_TO_DISTANCE(motor_pulses) / 100.0f);
  releaseLCD(pos);

  while (motor_pulses <= final_step_count && curr_time < start_time + TOPSIDE_COMP_COMMS_TIMEOUT_MS) {
    
    if (checkEstop()) { //If E-stop is pressed, set alarm fault and head into alarm loop.
      return false;
    }

    if (curr_time - last_lcd_update > 50) { //Update every 50ms
      snprintf(pos, sizeof(pos), "%.2fm", PULSES_TO_DISTANCE(motor_pulses)/ 100.0f);
      releaseLCD(pos);
      last_lcd_update = millis();
    }

    curr_time = millis();
    
  }

  turnMotorOff();

  if (curr_time >= start_time + TOPSIDE_COMP_COMMS_TIMEOUT_MS) {
    return false; //Timed out, went 30s without actually stopping
  }

  return true;
}


bool retrieveTube(float distance_cm) {
  unsigned long start_time = millis();
  unsigned long curr_time = millis();
  unsigned long last_lcd_update = millis();
  static char pos[6];

  uint32_t final_step_count = DISTANCE_TO_PULSES(abs(distance_cm));

  setMotorSpeed(RAISE_SPEED_CM_SEC/4);
  delay(100);
  setMotorSpeed(RAISE_SPEED_CM_SEC/2);
  delay(100);
  setMotorSpeed(RAISE_SPEED_CM_SEC);

  // Serial.print("FINAL STEP COUNT ");
  // Serial.println(final_step_count);

  while (motor_pulses >= final_step_count && curr_time < start_time + TOPSIDE_COMP_COMMS_TIMEOUT_MS) {
    if (checkEstop()) { //If E-stop is pressed, set alarm fault and head into alarm loop.
      return false;
    }

    if (curr_time - last_lcd_update > 50) { //Update every 50ms
      snprintf(pos, sizeof(pos), "%.2fm", PULSES_TO_DISTANCE(motor_pulses)/ 100.0f);
      recoverLCD(pos);
      last_lcd_update = millis();
    }

    if (magSensorRead()) {
      turnMotorOff();
      break;
    }

    else if (PULSES_TO_DISTANCE(motor_pulses) < ALIGNMENT_TUBE_OPENING_DIST && PULSES_TO_DISTANCE(motor_pulses) > NEARING_HOME_DIST) {
      setMotorSpeed(RAISE_SPEED_CM_SEC / 4); //Slow motor to 1/4 normal speed
    }

    else if (PULSES_TO_DISTANCE(motor_pulses) < NEARING_HOME_DIST) {
      setMotorSpeed(RAISE_SPEED_CM_SEC / 10); //Slow to 10th of normal speed for alignment
    }

    curr_time = millis();
  }

  turnMotorOff();

  if (curr_time >= start_time + TOPSIDE_COMP_COMMS_TIMEOUT_MS) {
    return false; //Timed out, went 30s without actually stopping
  }

  return true;
}


/**
 * @brief Lift the tube to the leaking position
 * @note This function is BLOCKING!!!
 * @note Assumes that tube is in the home position. IDK what happens if it isnt
 */
#define LIFT_SPEED_CM_S     (2.0f)
#define HOME_SPEED_CM_S     (-2.0f)

void liftupTube() {
  // if (tube_state) //If tube already lifted
  //   return;
  
  setMotorSpeed(LIFT_SPEED_CM_S);
  while(magSensorRead()) {}; //Sit in this loop, waiting for the mag sensor to stop sensing the tube magnet
                             // Once the magnetic sensor stop reading the magnet, the tube is pulled up enough
  turnMotorOff();

  return;
}

/**
 * @brief returns tube from lifted (to drain) state back to resting
 *
 * @param tube_state current tube state, true if lifted, false if resting
 */

void unliftTube() { // Resets tube back to home position after being lifted to dump excess sea/flushing water
    //if (!tube_state) return; 

    setMotorSpeed(HOME_SPEED_CM_S);
    
    while(!magSensorRead()){}; //Wait for the magnetic sensor to trip again

    while(magSensorRead()){}; //Then wait for it to "untrip", want to overshoot the magnet and then rehome

    homeTube();
}


void tube_home_funcs(bool lift) {
    static bool is_tube_up = false;

    if (lift) {
        liftupTube();
    } else {
        unliftTube();
    }
}

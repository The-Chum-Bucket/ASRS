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
  
  setMotorSpeed(-SAFE_RISE_SPEED_CM_SEC * 3);
  delay(1000);
  setMotorSpeed(SAFE_RISE_SPEED_CM_SEC); // Slowly raise the tube up to home position
  
  while (!magSensorRead() && state != ALARM){ //While the calculated position is greater than and the mag sensor is not sensing the magnet...
    //Update LCD?
    checkEstop();
    checkMotor();
    
  }

  motor_pulses = 0;

  turnMotorOff();
}

/**
 * @brief returns tube to home position at constant speed
 *        Version for Tube Flush end_time so it can update LCD
 *        Overloaded for code readability's sake
 * @param end_time from tubeFlushLoop, the calculated end time from millis()
 */
void homeTube(unsigned long end_time, FlushStage curr_stage) {
  
  setMotorSpeed(-SAFE_RISE_SPEED_CM_SEC * 3);
  delay(1000);
  setMotorSpeed(SAFE_RISE_SPEED_CM_SEC); // Slowly raise the tube up to home position
  
  while (!magSensorRead()) { //While the calculated position is greater than and the mag sensor is not sensing the magnet...
    checkEstop();
    updateFlushTimer(end_time, curr_stage);
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
  int curr_speed = 0;

  if (checkEstop() || checkMotor()) {
    return false;
  }

  rampUpMotor(curr_speed, -DROP_SPEED_CM_SEC);

  snprintf(pos, sizeof(pos), "%.2fm", PULSES_TO_DISTANCE(motor_pulses) / 100.0f);
  if (state == RELEASE) {
    resetLCD();
    releaseLCD(pos);
  }
    

  while (motor_pulses <= final_step_count && curr_time < start_time + TOPSIDE_COMP_COMMS_TIMEOUT_MS) {
    
    if (checkEstop() || checkMotor()) { //If E-stop is pressed, set alarm fault and head into alarm loop.
      return false;
    }

    if (state == RELEASE && curr_time - last_lcd_update > 50) { //Update every 50ms
      snprintf(pos, sizeof(pos), "%.2fm", PULSES_TO_DISTANCE(motor_pulses)/ 100.0f);
      releaseLCD(pos);
      last_lcd_update = millis();
    }

    curr_time = millis();
    
  }

  rampDownMotor(curr_speed, 0);

  if (curr_time >= start_time + TUBE_TIMEOUT_ERR_TIME_MS) {
    setAlarmFault(TUBE);
    return false; //Timed out, went 30s without actually stopping
  }

  return true;
}


bool retrieveTube(float distance_cm) {
  unsigned long start_time = millis();
  unsigned long curr_time = millis();
  unsigned long last_lcd_update = millis();
  static char pos[6];
  //bool stop_and_wait_flag = false; //Enables the sampler to stop as it nears the alignment tube, gives time to settle

  uint32_t final_step_count = DISTANCE_TO_PULSES(abs(distance_cm));
  bool stop_and_wait_flag = distance_cm < ALIGNMENT_TUBE_OPENING_DIST;

  int curr_speed = 0;

  if (checkEstop() || checkMotor()) { //If E-stop is pressed, set alarm fault and head into alarm loop.
      return false;
  }


  rampUpMotor(curr_speed, RAISE_SPEED_CM_SEC);
  // int curr_speed = RAISE_SPEED_CM_SEC;

  while (motor_pulses >= final_step_count && curr_time < start_time + TOPSIDE_COMP_COMMS_TIMEOUT_MS) {
    if (checkEstop() || checkMotor()) { //If E-stop is pressed, set alarm fault and head into alarm loop.
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

    else if (PULSES_TO_DISTANCE(motor_pulses) < ALIGNMENT_TUBE_OPENING_DIST 
            && PULSES_TO_DISTANCE(motor_pulses) > NEARING_HOME_DIST 
            && !stop_and_wait_flag) {
      
      rampDownMotor(curr_speed, 0);
      // curr_speed = 0;
      delay(5*1000); //Turn motor off and allow for tube to settle as it nears the opening of the alignment tube
      stop_and_wait_flag = true; //Indicate that the stop-and-wait-to-settle step is done
    }

    else if (PULSES_TO_DISTANCE(motor_pulses) < ALIGNMENT_TUBE_OPENING_DIST 
            && PULSES_TO_DISTANCE(motor_pulses) > NEARING_HOME_DIST 
            && stop_and_wait_flag) { //CONDITION NEEDS TO REMOVE TRUE TO WORK WITH PIER
        rampUpMotor(curr_speed, RAISE_SPEED_CM_SEC / 5); //Slow to 1/5 of typical speed as we travel up the tube
        // curr_speed = RAISE_SPEED_CM_SEC / 5; //Update curr_speed
    }

    else if (PULSES_TO_DISTANCE(motor_pulses) < NEARING_HOME_DIST) {
      rampDownMotor(curr_speed, RAISE_SPEED_CM_SEC / 10); //Slow to 10th of normal speed for alignment
      // curr_speed = RAISE_SPEED_CM_SEC / 10;
    }

    curr_time = millis();
  }

  turnMotorOff();

  if (curr_time >= start_time + TUBE_TIMEOUT_ERR_TIME_MS) {
    setAlarmFault(TUBE);
    return false; //Timed out, went 45s without actually stopping
  }

  return true;
}

/**
 * @brief Ramps UP motor speed from curr_speed to target_speed
 * 
 * @param curr_speed: the current speed of the motor
 * @param target_speed: the target final speed of the motor
 */

 void rampUpMotor(int &curr_speed, int target_speed) {
  if (abs(target_speed) <= abs(curr_speed)) {
    // Prevent ramping down from this function
    return;
  }

  const int steps = 10; // Hard-coded number of steps
  int sign = (target_speed >= 0) ? 1 : -1;
  int curr_mag = abs(curr_speed);
  int target_mag = abs(target_speed);

  float step_size = (float)(target_mag - curr_mag) / steps;

  for (int i = 1; i <= steps; ++i) {
    int new_mag = curr_mag + round(step_size * i);
    if (new_mag > target_mag) new_mag = target_mag;
    setMotorSpeed(sign * new_mag);
    delay(25);
  }

  setMotorSpeed(target_speed);
  curr_speed = target_speed;
}

  

/**
 * @brief Ramps DOWN motor speed from curr_speed to target_speed
 * 
 * @param curr_speed: the current speed of the motor, updated at end of function
 * @param target_speed: the target final speed of the motor
 */

 void rampDownMotor(int &curr_speed, int target_speed) {
  if (abs(target_speed) >= abs(curr_speed)) {
    // Prevent ramping up in this function
    return;
  }

  const int steps = 10; // Hard-coded number of steps
  int sign = (curr_speed >= 0) ? 1 : -1;
  int curr_mag = abs(curr_speed);
  int target_mag = abs(target_speed);

  float step_size = (float)(curr_mag - target_mag) / steps;

  for (int i = 1; i <= steps; ++i) {
    int new_mag = curr_mag - round(step_size * i);
    if (new_mag < target_mag) new_mag = target_mag;
    setMotorSpeed(sign * new_mag);
    delay(25);
  }

  setMotorSpeed(target_speed);
  curr_speed = target_speed;
}


/**
 * @brief Lift the tube to the leaking position
 * @warning assumes tube is in home position
 * 
 */

void liftupTube() {
  
  setMotorSpeed(DRAIN_LIFT_SPEED_CM_S);
  while(magSensorRead() && state != ALARM) {
    checkMotor();
    checkEstop();
  }; //Sit in this loop, waiting for the mag sensor to stop sensing the tube magnet
                             // Once the magnetic sensor stop reading the magnet, the tube is pulled up enough
  turnMotorOff();

  return;
}

/**
 * @brief returns tube from lifted (to drain) state back to resting
 * @warning assumes tube is already in lifted position, no way to tell if 
 *          it isn't as the "lifted" state is higher than the magnetic position sensor
 */

void unliftTube() { // Resets tube back to home position after being lifted to dump excess sea/flushing water
    setMotorSpeed(DRAIN_HOME_SPEED_CM_S);
    
    while(!magSensorRead() && state != ALARM){
      checkEstop();
      checkMotor();
    }; //Wait for the magnetic sensor to trip again

    while(magSensorRead() && state != ALARM){
      checkEstop();
      checkMotor();
    }; //Then wait for it to "untrip", want to overshoot the magnet and then rehome

    homeTube();
}
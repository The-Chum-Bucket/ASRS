/**
 * @brief 
 * 
 * @file position.ino
 */

#define REEL_RADIUS 5  //The gearbox reel's radius, currently is 5cm (eventually move into a config.ino)
#define GEARBOX_RATIO  5 // Geatbox ratio is 5:1, this number states how many motor turns equal one gearbox turn
#define TIMEOUT_TIME_MS 30 * 1000 //30 sec
#define PULSES_PER_REV 1600 //Setting (set by dip switches on the stepper motor or in stepper software) of pulses per motor revolution

// All inputs should be in cm, all outputs will then be in cm as well for the macros below
#define PULSES_TO_DISTANCE(num_pulses) ((pulses * PI * 2 * REEL_RADIUS)/(PULSES_PER_REV * GEARBOX_RATIO)) //Converts a number of pulses to a distance in cm
#define DISTANCE_TO_PULSES(distance_cm) ((GEARBOX_RATIO * PULSES_PER_REV * distance_cm) / (PI * 2 * REEL_RADIUS)) // Converts a distance into the corresponding # of pulses from home

#define SAFE_RISE_SPEED_CM_SEC  (3.0f)
#define SAFE_DROP_DIST_CM       (10.0f)
#define NUM_PHASES              (4UL)
#define FREE_FALL_IND           (2)
#define RAISE_DIST_PADDING_CM   (10.0f)

PositionConfig_t pos_cfg = {0};

void setPositionCfg(PositionConfig_t& cfg) {
    pos_cfg = cfg;
    return;
}

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
 * @brief retrieves tube for given distance
 * 
 * 
 * @param distance_cm distance to retrieve in cm
 * @return true if the tube has finished retrieving to given distance
 * @return false if the tube has not finished retrieving
 */

bool dropTube(unsigned int distance_cm) {
  unsigned long start_time = millis();
  unsigned long curr_time = millis();

  uint32_t final_step_count = DISTANCE_TO_PULSES(abs(distance_cm));

  setMotorSpeed(-pos_cfg.drop_speed_cm_sec/4);
  delay(100);
  setMotorSpeed(-pos_cfg.drop_speed_cm_sec/2);
  delay(100);
  setMotorSpeed(-pos_cfg.drop_speed_cm_sec);


  while (motor_pulses <= final_step_count && curr_time < start_time + TIMEOUT_TIME_MS) {
    curr_time = millis();
  }

  turnMotorOff();

  if (curr_time >= start_time + TIMEOUT_TIME_MS) {
    return false; //Timed out, went 30s without actually stopping
  }

  return true;
}

bool retrieveTube(float distance_cm) {
  unsigned long start_time = millis();
  unsigned long curr_time = millis();

  uint32_t final_step_count = DISTANCE_TO_PULSES(abs(distance_cm));

  setMotorSpeed(pos_cfg.drop_speed_cm_sec/4);
  delay(100);
  setMotorSpeed(pos_cfg.drop_speed_cm_sec/2);
  delay(100);
  setMotorSpeed(pos_cfg.drop_speed_cm_sec);


  while (motor_pulses >= final_step_count && curr_time < start_time + TIMEOUT_TIME_MS && !magSensorRead()) {
    curr_time = millis();
  }

  turnMotorOff();

  if (curr_time >= start_time + TIMEOUT_TIME_MS) {
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

void liftup_tube(bool& tube_state) {
  if (tube_state) //If tube already lifted
    return;
  
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

void unlift_tube(bool& tube_state) { // Resets tube back to home position after being lifted to dump excess sea/flushing water
    if (!tube_state) return; 

    setMotorSpeed(HOME_SPEED_CM_S);
    
    while(!magSensorRead()){}; //Wait for the magnetic sensor to trip again

    while(magSensorRead()){}; //Then wait for it to "untrip", want to overshoot the magnet and then rehome

    homeTube();
}


void tube_home_funcs(bool lift) {
    static bool is_tube_up = false;

    if (lift) {
        liftup_tube(is_tube_up);
    } else {
        unlift_tube(is_tube_up);
    }
}

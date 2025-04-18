/**
 * @brief 
 * 
 * @file position.ino
 */

void setPositionCfg(PositionConfig_t& cfg) {
    pos_cfg = cfg;
    return;
}

/**
 * @brief returns tube to home position at constant speed
 *        Re-homes the tube during initializing and calibrate state
 * TODO: refactor to understand why turnMotorOff doesn't fully work
 */
void homeTube() {
    tube_position_f = 0;
    if (!magSensorRead()) {
        while (!dropTube(SAFE_DROP_DIST_CM));
        setMotorSpeed(SAFE_RISE_SPEED_CM_SEC);
        while (!magSensorRead());
        turnMotorOff(true);
        tube_position_f = 0;
    }

    tube_home_funcs(true); 
    tube_home_funcs(false);
}

void homeTube2() {
  setMotorSpeed(SAFE_RISE_SPEED_CM_SEC); // Slowly raise the tube up to home position
//   while (global_tube_position > 0 && !magSensorRead()) { //While the calculated position is greater than and the mag sensor is not sensing the magnet...
//   }
  turnMotorOff();
}


/**
 * @brief drops tube with ramping function for given distance
 * 
 *      cm/s 
 *      ^
 *      |
 *      |         ________________
 *      |      __|                |__
 *      |   __|                      |__
 *      |__|                            |__
 *      |                                  |
 *      --------------------------------------> s
 *      |--|
 *       t is set by timer
 * 
 * @param distance_cm distance to drop in cm
 * @return true if the tube has finished dropping to given distance
 * @return false if the tube has not finished dropping
 */
bool dropTube(unsigned int distance_cm) {
    static bool dropping_flag = false;
    static bool small_drop = false;
    static unsigned long prev_time;
    static float drop_distance_cm;
    static size_t phase_ind;

    static float dists_cm[NUM_PHASES] = {pos_cfg.narrow_tube_cm, pos_cfg.tube_cm + pos_cfg.narrow_tube_cm, 0.0f, 0.0f};

    if (!dropping_flag) {
        dropping_flag = true;
        drop_distance_cm = distance_cm + tube_position_f;

        // small drop
        if (distance_cm <= pos_cfg.min_ramp_dist_cm)  {
            small_drop = true;
            setMotorSpeed(-pos_cfg.drop_speed_cm_sec);
        } 
        else {
            phase_ind = 0;
            dists_cm[FREE_FALL_IND] = drop_distance_cm - pos_cfg.water_level_cm; 
            dists_cm[FREE_FALL_IND + 1] = drop_distance_cm;
            setMotorSpeed(-pos_cfg.drop_speeds[phase_ind]);
        }

        prev_time = millis();
    }

    // running integral
    unsigned long cur_time = millis();
    unsigned long delta_time = cur_time - prev_time;
    prev_time = cur_time;

    if (small_drop) {
        tube_position_f += (delta_time * pos_cfg.drop_speed_cm_sec) / 1000.0f;
    } 
    else {
        tube_position_f += (delta_time * pos_cfg.drop_speeds[phase_ind]) / 1000.0f;

        // ramp to next speed 
        if (tube_position_f >= dists_cm[phase_ind]) {
            phase_ind++;
            setMotorSpeed(-pos_cfg.drop_speeds[phase_ind]);
        }
    }

    // reached drop distance
    if (tube_position_f >= drop_distance_cm) {
        turnMotorOff();
        dropping_flag = false;
        small_drop = false;
        return true;
    }

    return false;
}

bool dropTube2(unsigned int distance_cm) {
  
}



/**
 * @brief retrieves tube with ramping function for given distance
 * 
 *      cm/s 
 *      ^
 *      |
 *      |         ________________
 *      |      __|                |__
 *      |   __|                      |__
 *      |__|                            |__
 *      |                                  |
 *      --------------------------------------> s
 *      |--|
 *       t is set by timer
 * 
 * @param distance_cm distance to retrieve in cm
 * @return true if the tube has finished retrieving to given distance
 * @return false if the tube has not finished retrieving
 */
bool retrieveTube(float distance_cm) {
    static bool raise_flag = false;
    static bool small_raise = false;
    static unsigned long prev_time;
    static float raise_distance_cm;
    static size_t phase_ind;

    static float dists_cm[NUM_PHASES] = {0.0f, pos_cfg.tube_cm + pos_cfg.narrow_tube_cm, pos_cfg.narrow_tube_cm, 0.0f};

    if (!raise_flag) {
        raise_flag = true;
        // raise_distance_cm = tube_position_f - distance_cm - RAISE_DIST_PADDING_CM;
        raise_distance_cm = tube_position_f - distance_cm;

        // small raise
        if (distance_cm <= pos_cfg.min_ramp_dist_cm) {
            small_raise = true;
            setMotorSpeed(pos_cfg.raise_speed_cm_sec);
        }
        else {
            phase_ind = 0;
            dists_cm[0] = tube_position_f - pos_cfg.water_level_cm;
            setMotorSpeed(pos_cfg.raise_speeds[phase_ind]);
        }

        prev_time = millis();
    }

    // running integral
    unsigned long cur_time = millis();
    unsigned long delta_time = cur_time - prev_time;
    prev_time = cur_time;

    if (small_raise) {
        tube_position_f -= (delta_time * pos_cfg.raise_speed_cm_sec) / 1000.0f;
    } 
    else {
        tube_position_f -= (delta_time * pos_cfg.raise_speeds[phase_ind]) / 1000.0f;

        // ramp to next speed
        if (tube_position_f <= dists_cm[phase_ind]) {
            phase_ind++;
            setMotorSpeed(pos_cfg.raise_speeds[phase_ind]);
        }
    }

    // the mag sens GOOD :)
    if (magSensorRead()) {
        turnMotorOff();
        tube_position_f = 0; // ? does this need to be here
        raise_flag = false;

        if (small_raise) small_raise = false;
        return true;
    } 
    // motor is still spinning and tube is not home :(
    else if (tube_position_f <= raise_distance_cm) {
        raise_flag = false;
        turnMotorOff();
        setAlarmFault(TUBE);

        return false;
    }

    return false;
}


/**
 * @brief Lift the tube to the leaking position
 * @note This function is BLOCKING!!!
 * @note Assumes that tube is in the home position. IDK what happens if it isnt
 */
#define LIFT_SPEED_CM_S     (2.0f)
#define HOME_SPEED_CM_S     (-2.0f)
void liftup_tube(bool& tube_state) {
    if (tube_state) return; 

    int count = 0;
    setMotorSpeed(LIFT_SPEED_CM_S);
    while (1) {
        count += !magSensorRead(); 
        if (count > 100) {
            turnMotorOff();
            tube_state = true;
            return;
        }
    }
}


/**
 * @brief Returns the tube from leaking position to closed at home
 * @note This function is BLOCKING!!!
 * @note Assumes that tube is in the lifted up position. IDK what happens if it isnt
 */
void dropdown_tube(bool& tube_state) {
    if (!tube_state) return; 

    int count = 0;
    setMotorSpeed(HOME_SPEED_CM_S);
    while (1) {
        count += magSensorRead();
        if (count > 150) {
            turnMotorOff();
            tube_state = false;
            return;
        }
    }
}


void tube_home_funcs(bool lift) {
    static bool is_tube_up = false;

    if (lift) {
        liftup_tube(is_tube_up);
    } else {
        dropdown_tube(is_tube_up);
    }
}

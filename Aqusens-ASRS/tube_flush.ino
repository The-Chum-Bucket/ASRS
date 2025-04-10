// TODO: make on-the-fly configurable
void setFlushCfg(FlushConfig_t& cfg) {
  flush_cfg = cfg;

  DUMP_WATER_TIME_MS = flush_cfg.flush_time_cfg.dump_water_time_s * 1000;
  RINSE_TUBE_TIME_MS = flush_cfg.flush_time_cfg.rinse_tube_time_s * 1000;

  AIR_GAP_TIME_MS = flush_cfg.aqusens_time_cfg.air_gap_time_s * 1000;
  LAST_AIR_GAP_TIME_MS = flush_cfg.aqusens_time_cfg.last_air_gap_time_s * 1000;
  WATER_RINSE_TIME_MS = flush_cfg.aqusens_time_cfg.water_rinse_time_s * 1000;

  FLUSH_TIME_S = 3 * flush_cfg.flush_time_cfg.lift_tube_time_s +  
                flush_cfg.flush_time_cfg.dump_water_time_s + 
                flush_cfg.flush_time_cfg.rope_drop_time_s + 
                flush_cfg.flush_time_cfg.rinse_rope_time_s + 
                flush_cfg.flush_time_cfg.rinse_tube_time_s;

  AQUSENS_TIME_S = 3 * flush_cfg.aqusens_time_cfg.air_gap_time_s + 
                  3 * flush_cfg.aqusens_time_cfg.water_rinse_time_s + 
                  flush_cfg.aqusens_time_cfg.last_air_gap_time_s;
  TOT_FLUSH_TIME_S = FLUSH_TIME_S + AQUSENS_TIME_S;
  
  return;
}

/**
 * @brief FSM to control flushing process (8 states)
 * @return true once done with the flushing cycle
 * @return false if error occurs during flushing cycle
 */
bool flushTube() {
  static FlushState state;
  static unsigned long start_time;
  unsigned long cur_time;

  switch(state) {
    case INIT:
      state = DUMP_TUBE;
      start_time = millis();
      break;

    case DUMP_TUBE:
      tube_home_funcs(true);

      cur_time = millis();

      if (cur_time - start_time > DUMP_WATER_TIME_MS) {
        state = ROPE_DROP;
        
        tube_home_funcs(false);
      }

      break;      

    case ROPE_DROP:
      if (dropTube(DROP_TUBE_DIST_CM)) {
        updateSolenoid(OPEN, SOLENOID_ONE);
        
        state = RINSE_ROPE_HOME; 
      }
      break;

    case RINSE_ROPE_HOME:
      setMotorSpeed(HOME_TUBE_SPD_CM_S);
      
      if (magSensorRead()) {
        turnMotorOff();
        tube_home_funcs(true);
        start_time = millis();

        state = RINSE_TUBE;
      }
      break;    

    case RINSE_TUBE:
      cur_time = millis();

      if (cur_time - start_time > RINSE_TUBE_TIME_MS) {
        start_time = millis();
        
        // TODO: send pc to start aqusens motor
        state = RINSE_AQUSENS;
      }
      break;

    case RINSE_AQUSENS:

      if (flushAqusens(millis())) {
        updateSolenoid(CLOSED, SOLENOID_ONE);
        updateSolenoid(CLOSED, SOLENOID_TWO);
        
        tube_home_funcs(false);
        // TODO: tell pc to stop aqusens motor
        state = INIT;

        return true;
      }
      break;


    default:
      // Serial.println("[FLUSH_TUBE] bad state. Shouldn't be here");      
      while(1);
  }

  return false;
}

typedef enum AqusensState {
  RINSE_INIT,
  AIR_1,
  WATER_1,
  AIR_2,
  WATER_2,
  AIR_3,
  WATER_3,
  LAST_AIR,
} AqusensState;

/**
 * @brief TODO: Danny - update this function comment
 * 
 * @param cur_time 
 * @return true 
 * @return false 
 */
bool flushAqusens(unsigned long cur_time) {
  static AqusensState state = RINSE_INIT;
  static unsigned long prev_time;

  switch (state) {
    case RINSE_INIT:
      updateSolenoid(CLOSED, SOLENOID_TWO);
      
      prev_time = millis();

      // Serial.println("going to air 1");
      state = AIR_1;
      break;

    case AIR_1:

      if (cur_time - prev_time >= AIR_GAP_TIME_MS) {
        prev_time = millis();

        updateSolenoid(OPEN, SOLENOID_TWO);

        // Serial.println("going to water 1");
        state = WATER_1;
      }
      break;

    case WATER_1:
      if (cur_time - prev_time >= WATER_RINSE_TIME_MS) {
        prev_time = millis();

        updateSolenoid(CLOSED, SOLENOID_TWO);

        state = AIR_2;
        // Serial.println("going to air 2");

      }
      break;
    
    case AIR_2:
      if (cur_time - prev_time >= AIR_GAP_TIME_MS) {
        prev_time = millis();

        updateSolenoid(OPEN, SOLENOID_TWO);

        state = WATER_2;
        // Serial.println("going to water 2");

      }
      break;

    case WATER_2:
      if (cur_time - prev_time >= WATER_RINSE_TIME_MS) {
        prev_time = millis();

        updateSolenoid(CLOSED, SOLENOID_TWO);

        state = AIR_3;
        // Serial.println("going to air 3");

      }
      break;
    
    case AIR_3:
      if (cur_time - prev_time >= AIR_GAP_TIME_MS) {
        prev_time = millis();

        updateSolenoid(OPEN, SOLENOID_TWO);

        state = WATER_3;
        // Serial.println("going to water 3");

      }
      break;

    case WATER_3:
      if (cur_time - prev_time >= WATER_RINSE_TIME_MS) {
        prev_time = millis();

        updateSolenoid(CLOSED, SOLENOID_TWO);

        // Serial.println("going to last air ");

        state = LAST_AIR;
      }
      break;
    case LAST_AIR:
      if (cur_time - prev_time >= LAST_AIR_GAP_TIME_MS) {

        state = RINSE_INIT;
        // Serial.println("[FLUSH] Finished aqusens ");

        return true;
      }
  }

  return false;
}

void flushSampler(int sec) {
  updateSolenoid(OPEN, SOLENOID_ONE);
  updateSolenoid(OPEN, SOLENOID_TWO);

  delay(sec * 1000);
}

void flushSystem() {
  static unsigned long start_time;
  unsigned long cur_time;
  
  // DUMP TUBE -- TODO: refactor
  start_time = millis();
  tube_home_funcs(true);
  while (cur_time - start_time < DUMP_WATER_TIME_MS) {
    cur_time = millis();
  }
  tube_home_funcs(false);

  // DROP TUBE -- TODO: refactor
  while (!dropTube(DROP_TUBE_DIST_CM));
  updateSolenoid(OPEN, SOLENOID_ONE);

  // RAISE TUBE -- TODO: refactor
  setMotorSpeed(HOME_TUBE_SPD_CM_S);

  while (!magSensorRead());
  
  turnMotorOff();
  tube_home_funcs(true);

  flushSampler(15);
  updateSolenoid(CLOSED, SOLENOID_ONE);
  updateSolenoid(CLOSED, SOLENOID_TWO);
}

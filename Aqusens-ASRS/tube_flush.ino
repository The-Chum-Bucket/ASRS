/**
 * @brief flushes sampler device, ensuring that water is cool enough
 * 
 * @return milliseconds elapsed over the function, helps update LCD if any significant delay was added by cooling the water
 */

unsigned long flushDevice() {
  unsigned long start_time = millis();
  updateSolenoid(OPEN, SOLENOID_ONE);
  while (readRTD(FLUSHWATER_TEMP_SENSOR) > DEVICE_FLUSH_WATER_TEMP_MAX_C) {
    // do nothing, wait for water to cool down
  }
  
  updateSolenoid(OPEN, SOLENOID_TWO);
  return millis() - start_time;
}

/**
 * @brief closes all solenoids, stops water movement in system
 * 
 */
void closeAllSolenoids() {
  updateSolenoid(CLOSED, SOLENOID_ONE);
  updateSolenoid(CLOSED, SOLENOID_TWO);
}


/**
 * @brief updates the LCD every second with the current time remaining in the flushing stage
 * 
 * 
 * @param end_time the calculated end time of the current flush cycle (passed from main flushing function above)
 * @param curr_stage current stage of the flush procedure, needed to display curr state
 */
void updateFlushTimer(unsigned long end_time, FlushStage curr_stage) {
  static unsigned long last_update_time = 0;
  static bool temp_flag = false;
  static bool reached_zero = false;

  if (millis() - last_update_time >= 1000) {
      last_update_time = millis();
      temp_flag = true;

      uint32_t millis_remaining;
      if (end_time > millis()) {
          millis_remaining = end_time - millis();
      } else {
          // Handle overflow case
          millis_remaining = (UINT32_MAX - millis()) + end_time;
      }

      uint32_t seconds_remaining = millis_remaining / 1000;
      uint32_t minutes_remaining = seconds_remaining / 60;

      char sec_time[3];
      char min_time[3];

      snprintf(sec_time, sizeof(sec_time), "%02lu", seconds_remaining % 60);
      snprintf(min_time, sizeof(min_time), "%02lu", minutes_remaining);

      if (!reached_zero && minutes_remaining == 0 && seconds_remaining == 0) {
        reached_zero = true; //Prevents LCD from underflowing, just stays at zero
      }
      
      if (!reached_zero)
        flushLCD(min_time, sec_time, temp_flag, curr_stage);
      else {
        flushLCD("00", "00", temp_flag, curr_stage); //If exceeded the estimated flush time, then just print zeroes
      }
  }
}
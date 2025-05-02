/**
 * @brief flushes sampler device, ensuring that water is cool enough
 * 
 * @return milliseconds elapsed over the function, helps update LCD if any significant delay was added by cooling the water
 */

unsigned long flushDevice() {
  unsigned long start_time = millis();
  updateSolenoid(OPEN, SOLENOID_ONE);
  while (readRTD(TEMP_SENSOR_ONE) > DEVICE_FLUSH_WATER_TEMP_MAX_C) {
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
 * @brief runs the system flush procedure, coded as a finite state machine with discrete steps, adjust timings in config.h
 * 
 */
void flushSystem() {
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

  updateFlushTimer(end_time);

  while (state == FLUSH_TUBE) {
    switch(curr_stage) {
      DUMP_SAMPLE:
        if (stagesStarted[DUMP_SAMPLE] == false) {
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

      AIR_BUBBLE:
      if (stagesStarted[AIR_BUBBLE] == false) {
        pumpControl(START_PUMP);
        stagesStarted[AIR_BUBBLE] = true;
        curr_stage_end_time = curr_time + (1000* AIR_BUBBLE_TIME_S);
      }
      if (curr_time >= curr_stage_end_time) {
        pumpControl(STOP_PUMP);
        curr_stage = FRESHWATER_LINE_FLUSH;
      }
        break;

      FRESHWATER_LINE_FLUSH:
        if (stagesStarted[FRESHWATER_LINE_FLUSH] == false) {
          dropTube(30); //Drop down 30cm
          updateSolenoid(OPEN, SOLENOID_ONE); //Flush line
          homeTube();
          stagesStarted[FRESHWATER_LINE_FLUSH] = true;
          curr_stage_end_time = curr_time + (1000 * FLUSH_LINE_TIME_S);
        }
        if (curr_time >= curr_stage_end_time) {
          updateSolenoid(CLOSED, SOLENOID_ONE);
          curr_stage = FRESHWATER_DEVICE_FLUSH;
        }
        break;

      FRESHWATER_DEVICE_FLUSH:
        if (stagesStarted[FRESHWATER_DEVICE_FLUSH] == false) {
          end_time += flushDevice(); //Update end time if there was any waiting for flushing water to cool down.
          stagesStarted[FRESHWATER_DEVICE_FLUSH] = true;
          curr_stage_end_time = curr_time + (1000 * (FRESHWATER_TO_DEVICE_TIME_S + FRESHWATER_FLUSH_TIME_S));
        }
        if (curr_time >= curr_stage_end_time) {
          closeAllSolenoids();
          curr_stage = AIR_FLUSH;
        }
        break;

      AIR_FLUSH:
        if (stagesStarted[AIR_FLUSH] == false) {
          pumpControl(START_PUMP);
          stagesStarted[AIR_FLUSH] = true;
          curr_stage_end_time = curr_time + (1000 * FINAL_AIR_FLUSH_TIME_S);
        }
        if (curr_time >= curr_stage_end_time) {
          pumpControl(STOP_PUMP);
        }
        break;
      HOME_TUBE:
        if (stagesStarted[HOME_TUBE] == false) {
          dropTube(3);
          homeTube();
          stagesStarted[HOME_TUBE] = true;
          state = DRY;
        }
        break;

      default:
        break;
    }

    curr_time = millis();
    updateFlushTimer(end_time);
  }


  // // DUMP TUBE
  // liftupTube(); //Dump remainder of sample
  // //drySampleTube(); //5s delay for draining tube
  // unliftTube(); //Return tube to home position
  
  // // AIR BUBBLE
  // //pumpControl(START_PUMP);
  // //delay((90 + 10)*1000) //Delay for 90sec, plus 10s buffer REPLACE WITH NONBLOCKING
  // pumpControl(STOP_PUMP);

  // // FLUSH LINE (and not Aqusens)
  // dropTube(30); //Drop down 30cm
  // updateSolenoid(OPEN, SOLENOID_ONE); //Flush line
  // homeTube(); //Start to bring the tube home
  // updateSolenoid(CLOSED, SOLENOID_ONE); // Stop flushing line

  // // FLUSH AQUSENS
  // flushDevice(10); // TODO: Set to 180s (3:00) (30s for water to reach Aqusens, 2:30 of flushing),
  //                  // start Aqusens pump
  
  // // AIR "BUBBLE" - drain entire system (all Aqusens lines dry)
  // pumpControl(START_PUMP);
  // //delay for 45s to drain system
  // pumpControl(STOP_PUMP);

  
  // // HOME TUBE
  // dropTube(3);
  // homeTube();
}

/**
 * @brief updates the LCD every second with the current time remaining in the flushing stage
 * 
 * 
 * @param end_time the calculated end time of the current flush cycle (passed from main flushing function above)
 */
void updateFlushTimer(unsigned long end_time) {
  static unsigned long last_update_time = 0;
  static bool temp_flag = false;

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

      flushLCD(min_time, sec_time, seconds_remaining % 4, temp_flag);
  }
}
/**
 * @brief SETTINGS
 * 
 * Two page options:
 *    - SET_INTERVAL/SET_START_TIME/SET_CLOCK
 *    - SET_SOAK_TIME/SET_DRY_TIME/FILTER_STATUS
 */
void settingsLoop() {
  uint8_t last_key_press = rtc.getMinutes();
  char key_pressed;
  resetLCD();
  uint32_t last_key_press_time = millis();
  
  while (state == SETTINGS) {
    settingsLCD(settings_page); //Launch into first page of settings

    if (settings_page != last_setting_page)
      key_pressed = cursorSelect(0, 2);
    else
      key_pressed = cursorSelect(0, 2);

    if (key_pressed > 0) {
      last_key_press = rtc.getMinutes();
    }

    if (key_pressed == 'L') { // left
      if (settings_page == 1) { // exits to STANDBY mode on first page left select
        resetLCD();
        state = STANDBY;
        cursor_y = 2;
      }
      else { // else, returns to previous settings page
        resetLCD();
        settings_page--;
      }
    }

    else if (key_pressed == 'R' && settings_page != last_setting_page) { //right
      resetLCD();
      settings_page++;
    }

    else if (key_pressed == 'S') {
      if (settings_page == 1) {
        switch(cursor_y) {
          case 0:
            state = SET_INTERVAL;
            break;
          
          case 1:
            state = SET_START_TIME;
            break;
          
          case 2:
            state = SET_CLOCK;
            break;
          
          default:
            break;
        }
      }

      else if (settings_page == 2) {
        switch(cursor_y) { //add more states
          case 0:
            state = SET_SOAK_TIME;
            break;
          
          case 1:
            state = SET_DRY_TIME;
            break;
          
          case 2: // TODO: this state needs to be implemented
            state = FILTER_STATUS; // - displays status of water filter - when does it need to be replaced (and SD card data?)
            break;
          
          default:
            break;
        }
      }
    }

    if (rtc.getMinutes() == ((last_key_press + 5) % 60)) {
      state = STANDBY;
    }
  }
}

/**
 * @brief SET_START_TIME
 * TODO: do we still want this?
 */
void setStartTimeLoop() {
  tmElements_t adjusted_start_time;
  adjusted_start_time.Month = rtc.getMonth();
  adjusted_start_time.Day = rtc.getDay();
  adjusted_start_time.Year = rtc.getYear(); 
  adjusted_start_time.Hour = rtc.getHours();
  adjusted_start_time.Minute = rtc.getMinutes();
  char key;
  uint8_t cursor_pos = 0; // Holds time-setting position of cursor for "00-00-00 00:00"
                         // Does not include spacing/colons/hyphens, ranges 0 to 9.

  resetLCD();
  initSetClockLCD();
  updateSetClockLCD(cursor_pos, adjusted_start_time);
  lcd.blink();

  while (state == SET_START_TIME) {
    key = getKeyDebounce();
    
    if (key != NULL) {
      if (key == 'S') {
        updateAlarm(adjusted_start_time);
        lcd.noBlink();
        state = SETTINGS;
      }

      else if (key == 'L' && cursor_pos > 0) {
        cursor_pos--;
      }

      else if (key == 'R' && cursor_pos < 9) {
      cursor_pos++;
      }

      else if (key == 'U' || key == 'D') {
        adjustSetTimeDigit(key, &adjusted_start_time, &cursor_pos);
      }
      if (state == SET_START_TIME) {
        updateSetClockLCD(cursor_pos, adjusted_start_time);
      }
    } 
  }

}

/**
 * @brief Handles user interface for setting the system clock manually.
 * 
 * Allows user to adjust the date and time. Saves to RTC and updates alarm
 * when confirmed. This can be replaced or supplemented by syncing with
 * a host computer via a Python script to fetch epoch time.
 */

void setClockLoop() {

  tmElements_t adjusted_time;
  adjusted_time.Month = rtc.getMonth();
  adjusted_time.Day = rtc.getDay();
  adjusted_time.Year = rtc.getYear(); 
  adjusted_time.Hour = rtc.getHours();
  adjusted_time.Minute = rtc.getMinutes();
  char key;
  uint8_t cursor_pos = 0; // Holds time-setting position of cursor for "00-00-00 00:00"
                         // Does not include spacing/colons/hyphens, ranges 0 to 9.

  resetLCD();
  initSetClockLCD();
  updateSetClockLCD(cursor_pos, adjusted_time);
  lcd.blink();

  while (state == SET_CLOCK) {
    key = getKeyDebounce();
    
    if (key != NULL) {
      if (key == 'S') {
        breakTime(makeTime(adjusted_time), adjusted_time);
        rtc.setMonth(adjusted_time.Month);
        rtc.setDay(adjusted_time.Day);
        rtc.setYear(adjusted_time.Year);
        rtc.setHours(adjusted_time.Hour);
        rtc.setMinutes(adjusted_time.Minute);
        updateAlarm();
        lcd.noBlink();
        state = SETTINGS;
      }

      else if (key == 'L' && cursor_pos > 0) {
        cursor_pos--;
      }

      else if (key == 'R' && cursor_pos < 9) {
        cursor_pos++;
      }

      else if (key == 'U' || key == 'D') {
        adjustSetTimeDigit(key, &adjusted_time, &cursor_pos);
      }
      if (state == SET_CLOCK) {
        updateSetClockLCD(cursor_pos, adjusted_time);
      }
    } 
  }
}

/**
 * @brief SET_INTERVAL
 * 
 */
void setIntervalLoop() {
  tmElements_t new_interval;
  new_interval.Day = sample_interval.Day;
  new_interval.Hour = sample_interval.Hour;
  new_interval.Minute = sample_interval.Minute;
  char key;
  uint8_t cursor_pos = 0; // Holds time-setting position of cursor for "00 00 00" (Day Hour Min)
                         // Does not include spacing/colons/hyphens, ranges 0 to 5.

  resetLCD();
  initSetIntervalLCD();
  updateSetIntervalLCD(cursor_pos, new_interval);
  lcd.blink();

  while (state == SET_INTERVAL) {
    key = getKeyDebounce();
    
    if (key != NULL) {
      if (key == 'S') {
        sample_interval.Day = new_interval.Day;
        sample_interval.Hour = new_interval.Hour;
        sample_interval.Minute = new_interval.Minute;
        updateAlarm();
        lcd.noBlink();
        state = SETTINGS;
      }

      else if (key == 'L' && cursor_pos > 0) {
        cursor_pos--;
      }

      else if (key == 'R' && cursor_pos < 5) {
      cursor_pos++;
      }

      else if (key == 'U' || key == 'D') {
        adjustSetIntervalDigit(key, &new_interval, &cursor_pos);
      }
      if (state == SET_INTERVAL) {
        updateSetIntervalLCD(cursor_pos, new_interval);
      }
    } 
  }
}

/**
 * @brief SET_SOAK_TIME
 * 
 */
void setSoakTimeLoop() {
  tmElements_t new_soak_time;
  new_soak_time.Minute = soak_time.Minute;
  new_soak_time.Second = soak_time.Second;
  char key;
  uint8_t cursor_pos = 0; // Holds time-setting position of cursor for "00 00" (Hour Min)
                         // Does not include spacing/colons/hyphens, ranges 0 to 3.

  resetLCD();
  initSetSoakOrDryOrFlushLCD();
  updateSetSoakOrDryOrFlushLCD(cursor_pos, new_soak_time);
  lcd.blink();

  while (state == SET_SOAK_TIME) {
    key = getKeyDebounce();
    
    if (key != NULL) {
      if (key == 'S') {
        //breakTime(makeTime(new_interval), new_interval);
        soak_time.Second = new_soak_time.Second;
        soak_time.Minute = new_soak_time.Minute;

        lcd.noBlink();
        state = SETTINGS;
      }

      else if (key == 'L' && cursor_pos > 0) {
        cursor_pos--;
      }

      else if (key == 'R' && cursor_pos < 3) {
      cursor_pos++;
      }

      else if (key == 'U' || key == 'D') {
        adjustSetSoakOrDryDigit(key, &new_soak_time, &cursor_pos);
      }
      if (state == SET_SOAK_TIME) {
        updateSetSoakOrDryOrFlushLCD(cursor_pos, new_soak_time);
        }
    } 
  }
}

/**
 * @brief SET_DRY_TIME
 * 
 */
void setDryTimeLoop() {
  tmElements_t new_dry_time;
  new_dry_time.Minute = dry_time.Minute;
  new_dry_time.Second = dry_time.Second;
  char key;
  uint8_t cursor_pos = 0; // Holds time-setting position of cursor for "00 00" (Hour Min)
                         // Does not include spacing/colons/hyphens, ranges 0 to 3.

  resetLCD();
  initSetSoakOrDryOrFlushLCD();
  updateSetSoakOrDryOrFlushLCD(cursor_pos, new_dry_time);
  lcd.blink();

  while (state == SET_DRY_TIME) {
    key = getKeyDebounce();
    
    if (key != NULL) {

      if (key == 'S') {
        //breakTime(makeTime(new_interval), new_interval);
        dry_time.Second = new_dry_time.Second;
        dry_time.Minute = new_dry_time.Minute;


        lcd.noBlink();
        state = SETTINGS;
      }

      else if (key == 'L' && cursor_pos > 0) {
        cursor_pos--;
      }

      else if (key == 'R' && cursor_pos < 3) {
      cursor_pos++;
      }

      else if (key == 'U' || key == 'D') {
        adjustSetSoakOrDryDigit(key, &new_dry_time, &cursor_pos);
      }

      if (state == SET_DRY_TIME) {
      updateSetSoakOrDryOrFlushLCD(cursor_pos, new_dry_time);
      }
    } 
  }
}
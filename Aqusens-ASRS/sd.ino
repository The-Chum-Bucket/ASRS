/**
 * @brief Initializes the SD card communication, waits for SD.begin to return.
 * 
 * @return float offset distance for tide in cm
 */

void initSD() {
    if (!SD.begin(SD_CS)){
        Serial.println("[SD] Failed to Initialize SD card");
    }
}

/**
 * @brief interpolates tide data from SD card
 * 
 * @return float offset distance for tide in cm
 */
float getTideData(){
    char first_char;
    String line;
    int curr_year, curr_month, curr_day, curr_hour;
    int itr_year, itr_month, itr_day, itr_hour, pred_index;
    float prev_itr_pred, itr_pred, prev_itr_hour;

    String data_print;

    prev_itr_pred = -1;
    prev_itr_hour = -1;

    File file = SD.open(TIDE_FILE_NAME);
    // TODO: do this check multiple times
    if (!file){
        Serial.println("Failed to Open Tide File");
        return NULL;
    }

    /* Skip over File Header */
    while (file.available()){
        line = file.readStringUntil('\n');
        first_char = line[0];
        if (isDigit(first_char)){
            break;  /* At the first line of data */
        }
    }

    /* Get Current Time */
    curr_year = rtc.getYear();  /* In #### format */
    curr_month = rtc.getMonth();
    curr_day = rtc.getDay();
    curr_hour = (rtc.getHours() + GMT_TO_PST) % 24; /* Convert GMT tide data to PST */

    /* Start reading tide data */
    while (file.available()){
        // line = file.readStringUntil('\n');  /* Get line of data */

        // Serial.println(line);

        if (line.length() > 0){
            line.trim();  /* Removes whitespaces from end and beginning */

            itr_year = line.substring(2,4).toInt();
            itr_month = line.substring(5,7).toInt();
            itr_day = line.substring(8,10).toInt();
            itr_hour = line.substring(15,17).toInt(); 
            pred_index = line.indexOf('\t', 21) + 2;  /* Offset to move past 2 tabs */
            data_print = line.substring(pred_index);
            itr_pred = line.substring(pred_index, line.indexOf('\t', pred_index)).toInt();  // Needs to be updated for military times

            if (curr_year != itr_year){
                Serial.println("Need to update SD Card\n");
                file.close();
                return -1;  /* Need to Update Tide data file */
            }

            if (curr_month == itr_month){
                if (curr_day == itr_day){
                    if (curr_hour < itr_hour){
                        break;  /* Closest tide data found */
                    }
                }
            }

            prev_itr_pred = itr_pred;
            prev_itr_hour = itr_hour;
            line = file.readStringUntil('\n');
        } else {
            file.close();
            return -1; /* Reached end of file */
        }
    }

    /* Interpolate to get an estimate of current tide level */
    float scaled_curr_hour, hour_diff;
    float proportion, pred_diff, interpolated_pred;

    if (prev_itr_pred == -1 and prev_itr_hour == -1){
        file.close();
        return itr_pred;  /* First in list is closest match so just return it since there is no prev */
    }

    if (prev_itr_hour > itr_hour){
        /* Need to scale time as interpolation is inbetween two days */
        scaled_curr_hour = (curr_hour + 24) - prev_itr_hour; 
        hour_diff = (itr_hour + 24) - prev_itr_hour;
    } else {
        scaled_curr_hour = curr_hour - prev_itr_hour; 
        hour_diff = itr_hour - prev_itr_hour;
    }

    proportion = scaled_curr_hour / hour_diff;
    pred_diff = itr_pred - prev_itr_pred;
    interpolated_pred = (pred_diff * proportion) + prev_itr_pred;

    file.close();
    return interpolated_pred;
}

/**
 * @brief Retrieves the distance to drop based on tide data.
 * 
 * Attempts to get the drop distance from the topside computer first.
 * If communication fails or times out, falls back to interpolated SD card tide data.
 * Applies default pier offset and returns adjusted drop distance in cm.
 * 
 * @return float Drop distance in centimeters.
 */

float getDropDistance(){
    float drop_distance_cm = -1000; // Err value, will be updated to a non-negative (valid) distance if receive reply from topside computer

    sendToPython("T");
    //drop_distance_cm = getTideData();

    // get the distance to drop from online or sd card
    unsigned long start_time = millis();
    unsigned long curr_time = start_time;
    while (start_time + TOPSIDE_COMP_COMMS_TIMEOUT_MS > curr_time)
    {
        if (Serial.available()) {
            String data = Serial.readStringUntil('\n'); // Read full line
            drop_distance_cm = data.toFloat();  // Convert to float
            break;
        }
        
        checkEstop();
        curr_time = millis(); //Update curr_time

    }


    if (start_time + TOPSIDE_COMP_COMMS_TIMEOUT_MS <= curr_time) { //If timeout ocurred...
      setAlarmFault(TOPSIDE_COMP_COMMS);
    }

    // if drop distance is -1000 then get SD card info
    if (drop_distance_cm == -1000) {
        
        drop_distance_cm = getTideData();
    }
    // otherwise convert from meters to cm
    else {
        drop_distance_cm = drop_distance_cm * 100;
    }

    // Flush any remaining characters
    while (Serial.available()) {
        Serial.read();  // Discard extra data
    }

    return PIER_DEFAULT_DIST_CM - drop_distance_cm + 60.0f;
}

/**
 * @brief Recursively lists all files and directories on the SD card.
 * 
 * Prints the name and size of each file found under the given directory.
 * Indents output based on directory depth.
 * 
 * @param dir The starting directory to list.
 * @param numTabs Number of tab characters to indent for nested directories.
 */


void listFiles(File dir, int numTabs) {
    File entry = dir.openNextFile();
    while (entry) {
        
        for (int i = 0; i < numTabs; i++) {
            Serial.print("\t");
        }
        Serial.print(entry.name());
        if (entry.isDirectory()) {
            Serial.println("/");
            listFiles(entry, numTabs + 1); // Recursive call for subdirectories
        } else {
            Serial.print("\t");
            Serial.println(entry.size(), DEC); // Print file size
        }
        entry.close();
        
        entry = dir.openNextFile();
    }
}

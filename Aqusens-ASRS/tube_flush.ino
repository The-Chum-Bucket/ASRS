void flushDevice(int sec) {
  updateSolenoid(OPEN, SOLENOID_ONE);
  updateSolenoid(OPEN, SOLENOID_TWO);

  delay(sec * 1000);
}

void drySampleTube(int sec) {
  updateSolenoid(CLOSED, SOLENOID_ONE);
  updateSolenoid(CLOSED, SOLENOID_TWO);

  delay(sec * 1000);
}

// void homeTube3() {
//   while (!dropTube(5.0f));
//   setMotorSpeed(HOME_TUBE_SPD_CM_S);
//   while (!magSensorRead());
//   turnMotorOff();
// }

// void dumpTube() {
//   if (!magSensorRead()) {
//     homeTube3();
//   }

//   setMotorSpeed(HOME_TUBE_SPD_CM_S);
//   while (magSensorRead());
//   turnMotorOff();
// }

// TODO: will timing need to be redone for a different tubing length? Will tubing length be changed? (small tubing connected to Aqusens)
void flushSystem() {

  /*
  *   TODO: Replace all of pseudo-function calls to aqusens comms with real ones
  */

  //hard coded at the moment but 
  //int secondsRemaining = 5 + 
  
  // DUMP TUBE
  liftupTube(); //Dump remainder of sample

  drySampleTube(5); //5s delay for draining tube
  unliftTube(); //Return tube to home position
  
  // AIR BUBBLE
  /*sendToAqusens(ENABLE_PUMP)*/
  //delay((90 + 10)*1000) //Delay for 90sec, plus 10s buffer
  /*sendToAqusens(DISABLE_PUMP)*/

  // FLUSH LINE (and not Aqusens)
  dropTube(30); //Drop down 30cm
  updateSolenoid(OPEN, SOLENOID_ONE); //Flush line
  homeTube(); //Start to bring the tube home
  updateSolenoid(CLOSED, SOLENOID_ONE); // Stop flushing line

  // FLUSH AQUSENS
  flushDevice(10); // TODO: Set to 180s (3:00) (30s for water to reach Aqusens, 2:30 of flushing),
                   // start Aqusens pump
  
  // AIR "BUBBLE" - drain entire system (all Aqusens lines dry)
  /*sendToAqusens(ENABLE_PUMP)*/
  //delay for 45s to drain system
  /*sendToAqusens(DISABLE_PUMP)*/

  
  // HOME TUBE
  dropTube(3);
  homeTube();
}

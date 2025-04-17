void flushDevice(int sec) {
  updateSolenoid(OPEN, SOLENOID_ONE);
  updateSolenoid(OPEN, SOLENOID_TWO);

  delay(sec * 1000);
}

void drySampler(int sec) {
  updateSolenoid(CLOSED, SOLENOID_ONE);
  updateSolenoid(CLOSED, SOLENOID_TWO);

  delay(sec * 1000);
}

void homeTube3() {
  while (!dropTube(5.0f));
  setMotorSpeed(HOME_TUBE_SPD_CM_S);
  while (!magSensorRead());
  turnMotorOff();
}

void dumpTube() {
  if (!magSensorRead()) {
    homeTube3();
  }

  setMotorSpeed(HOME_TUBE_SPD_CM_S);
  while (magSensorRead());
  turnMotorOff();
}

// TODO: will timing need to be redone for a different tubing length? Will tubing length be changed? (small tubing connected to Aqusens)
void flushSystem() {
  // TODO: turn on Aqusens pump

  // DUMP TUBE
  dumpTube();

  // AIR BUBBLE
  drySampler(10); // TODO: Set to 90s (1:30 for air to reach Aqusens)

  // TODO: turn off Aqusens pump

  // DROP TUBE
  while (!dropTube(10.0f)); // TODO: Set to a real distance
  
  // FLUSH LINE (and not Aqusens)
  updateSolenoid(OPEN, SOLENOID_ONE);

  // RAISE TUBE to dump position
  dumpTube();

  // TODO: turn on Aqusens pump

  // FLUSH AQUSENS
  flushDevice(10); // TODO: Set to 180s (3:00) (30s for water to reach Aqusens, 2:30 of flushing)
  
  // AIR "BUBBLE" - drain entire system (all Aqusens lines dry)
  drySampler(10);// TODO: Set to 150s (2:30) (2:30 for Aqusens to be drained/dry)
  
  // HOME TUBE
  homeTube3();
}

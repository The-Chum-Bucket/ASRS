//  -Aqusens - NORA
//  -Aqusens - Nearshore Ocean Retrieval Apparatus v2.0
//  -Date of Last Revision: 2/26/25
//  -Califonia Polytechnic State University
//  -Bailey College of Science and Mathamatics Biology Department
//  -Primary Owner: Alexis Pasulka
//  -Design Engineers: Doug Brewster and Rob Brewster
//  -Contributors: Jack Anderson, Emma Lucke, Deeba Khosravi, Jorge Ramirez, Danny Gutierrez
//  -Microcontroller: P1AM-100 by ProOpen 
//  -Arduino IDE version:2.3.6
//  -See User Manual For Project Description
//  -non-stock libraries needed: (add library reference here)

//* libraries

#include <P1AM.h>
#include <SD.h>
#include <LiquidCrystal_I2C.h>
#include <RTCZero.h>
#include <SimpleKeypad.h>
#include <TimeLib.h>
#include <math.h>
#include "SAMD_PWM.h"
#include <ArduinoJson.h>
#include "config.h" // Config file, change global defines here (reel radius, flush timing, etc.)

//volatile StateEnum state = SAMPLE; //Global vars for tracking device state...
volatile StateEnum state = CALIBRATE; //Global vars for tracking device state...
volatile AlarmFault fault = NONE;   // and for tracking Alarm reason

volatile uint32_t motor_pulses = 0; // Global position tracker, increments each pulse when lowering, decrements each pulse when raising 
                                    // (down is the "positive direction")


int8_t cursor_y = 2; // keeps track of current cursor position

// lcd.ino
LiquidCrystal_I2C lcd(0x27, 20, 4);

volatile bool toggle = false; //Used for square wave generation in Timer Interrupt
MotorDir global_motor_state = OFF;

float drop_distance_cm;

tmElements_t next_sample_time, sample_interval, soak_time, dry_time;

uint8_t last_setting_page = 2; // amount of settings pages
uint8_t settings_page = 1; // current settings page

volatile bool estop_pressed = false; // Flag to keep track of E-stop pressed/released

// RTC
RTCZero rtc;

// Bool for keeping track of second attempts at retrieving a sample
bool is_second_retrieval_attempt = false;

// Solenoids
typedef enum SolenoidState {
  OPEN,
  CLOSED
} SolenoidState;

SolenoidState solenoid_one_state = CLOSED;
SolenoidState solenoid_two_state = CLOSED;

bool debug_ignore_timeouts = true;

/* Setup and Loop **************************************************************/
void setup() {
  Serial.begin(115200);
  while (!P1.init()) {} // Initialize controller

  lcd.init(); // Initialize the LCD
  lcd.backlight(); // Turn on the backlight
  lcd.setCursor(0, 0); // Set cursor to column 0, row 0

  rtcInit();
  rtdInit();
  gpioInit();
  estopInit();
  motorInit();

  // state = CALIBRATE;
  
  // Serial.println("[SETUP] done with init");
}

void loop() {
  switch (state) {
    case CALIBRATE: // Entered after Alarm mode to recalibrate sample device and flush as needed
      calibrateLoop();
      break;
    case STANDBY: // Always starts in STANDBY
      standbyLoop();
      break;
    case ENSURE_SAMPLE_START: // "Are you sure?" screen for manually starting sample run
      ensureSampleStartLoop();
      break;
    case RELEASE: // Releasing the sample device to the ocean
      releaseLoop();
      break;
    case SOAK: // Device on the surface of the ocean, collecting sample
      soakLoop();
      break;
    case RECOVER: // Recovering the sample device from the ocean surface to the home position
      recoverLoop();
      break;
    case SAMPLE: // Sample is sent through the Aqusens device
      sampleLoop();
      break;
    case FLUSH_SYSTEM: // Aqusens and Sample device are flushed with filtered freshwater/air
      flushSystemLoop();
      break;
    case DRY: // Sample device is dried for predetermined amount of time
      dryLoop();
      break;
    case ALARM: // Alarm mode is tripped due to E-stop press
      alarmLoop();
      break;
    case MANUAL: // Manual control of motor/solenoids, only entered from alarm mode
      manualLoop();
      break;
    case MOTOR_CONTROL:
      motorControlLoop();
      break;
    case SOLENOID_CONTROL:
      solenoidControlLoop();
    case SETTINGS: // Pages of parameters that can be modified or checked
      settingsLoop();
      break;
    case SET_CLOCK: // Settings option to set current time
      setClockLoop();
      break;
    case SET_INTERVAL: // Settings option to set sampling interval
      setIntervalLoop();
      break;
    case SET_START_TIME: // Settings option to set start time
      setStartTimeLoop();
      break;
    case SET_SOAK_TIME: // Settings option to set soak time
      setSoakTimeLoop();
      break;
    case SET_DRY_TIME: // Settings option to set dry time
      setDryTimeLoop();
      break;
    default:
      break;
  }
}

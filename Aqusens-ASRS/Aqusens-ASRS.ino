//  -Aqusens-ASRS
//  -Aqusens - Automated Sample Retrevial System V1.0
//  -Date of Last Revision: 2/26/25
//  -Califonia Polytechnic State University
//  -Bailey College of Science and Mathamatics Biology Department
//  -Primary Owner: Alexis Pasulka
//  -Design Engineers: Doug Brewster and Rob Brewster
//  -Contributors: Jack Anderson, Emma Lucke, Deeba Khosravi, Jorge Ramirez, Danny Gutierrez
//  -Microcontroller: P1AM-100 ProOpen 
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

/* Pin Mapping ********************************************************************/

// Slots - Position of modules
#define HV_GPIO_SLOT 1                  // High voltage GPIO (P1-15CDD1)
#define RELAY_SLOT   2                  // Relay module (P1-04TRS)
#define RTD_SLOT     3                  // RTD Temp Sensor Module (P1-04RTD)

// Keypad (GPIO)
#define KEY_S 0
#define KEY_D 1
#define KEY_U 2
#define KEY_L 3
#define KEY_R 4

// SD card - chip select pin TODO: what is this?
#define SD_CS 28

// Motor (GPIO)
#define STEP_POS_PIN  6
#define DIR_POS_PIN   13
#define ALARM_PLUS A2  // Used as interrupt, should be high normally, low in case of motor alarm
#define ALARM_MINUS A5 // Keep High

// HV GPIO (P1-15CDD1)
#define MAG_SENSOR_IO_SLOT 1

// Relays - Motor and Solenoid Power (P1-04TRS)
#define MOTOR_POWER 1
#define SOLENOID_ONE 3
#define SOLENOID_TWO 4

// Temperature Sensors (P1-04RTD)
typedef enum TempSensor {
  TEMP_SENSOR_ONE = 1, // TODO: Eventually change to more descriptive names, depending on what each sensor actually is measuring
  TEMP_SENSOR_TWO = 2
} TempSensor;


/* Variable Declarations *********************************************************/

// Machine States
enum StateEnum {
  CALIBRATE,
  STANDBY,
  RELEASE,
  SOAK,
  RECOVER,
  SAMPLE,
  FLUSH_TUBE,
  DRY,
  ALARM,
  MANUAL,
  MOTOR_CONTROL,
  SOLENOID_CONTROL,
  SETTINGS,
  SET_INTERVAL,
  ENSURE_SAMPLE_START,
  SET_START_TIME,
  SET_TUBE_FLUSH_TIME,
  SET_AQUSENS_FLUSH_TIME,
  ADD_EVENT,
  VIEW_EVENTS,
  SET_CLOCK,
  SET_DRY_TIME,
  SET_SOAK_TIME,
  FILTER_STATUS,
  SET_BRIGHTNESS,
  SET_CONTRAST      
};

typedef enum AlarmFault { // TODO: add SERIAL alarm state
  NONE,
  MOTOR,
  TUBE,
  ESTOP,
} AlarmFault;

volatile StateEnum state = STANDBY; 
volatile AlarmFault fault = NONE;

volatile uint32_t motor_pulses = 0; // Global position tracker, increments each pulse when lowering, decrements each pulse when raising 
                                    // (down is the "positive direction")


int8_t cursor_y = 2; // keeps track of current cursor position

// config.ino - TODO: REMOVE ALL THIS

#define CONFIG_FILENAME         ("CONFIG~1.JSO")

// motor.ino
#define REEL_RAD_CM             (5.0f)
#define PULSE_PER_REV           (1600)
#define GEAR_RATIO              (5.0f)

// position.ino
#define NARROW_TUBE_CM          (15.0f) 
#define TUBE_CM                 (85.0f) 
#define WATER_LEVEL_CM          (15.0f) 
#define MIN_RAMP_DIST_CM        (NARROW_TUBE_CM + TUBE_CM + WATER_LEVEL_CM + 5.0f)
#define DROP_SPEED_CM_SEC       (15.0f)
#define RAISE_SPEED_CM_SEC      (5.0f)

// tube_flush.ino
// timings
#define LIFT_TUBE_TIME_S        (0.5f)
#define DUMP_WATER_TIME_S       (5UL)
#define ROPE_DROP_TIME_S        (40.0f / 15.0f)
#define RINSE_ROPE_TIME_S       (20.0f)
#define RINSE_TUBE_TIME_S       (5UL)

// aqusens timings 
#define AIR_GAP_TIME_S          (5)
#define WATER_RINSE_TIME_S      (15)
#define LAST_AIR_GAP_TIME_S     (10)

// sd.ino
#define PIER_DEFAULT_DIST_CM  (762.0f)
#define TIDE_FILE "tides.txt"

// ========================================================================
// Struct Definitions
// ========================================================================

typedef struct MotorConfig_t {
    float reel_radius_cm;
    float gear_ratio;
    unsigned int pulse_per_rev;
} MotorConfig_t;

typedef struct PositionConfig_t {
    float narrow_tube_cm;
    float tube_cm;
    float water_level_cm;
    float min_ramp_dist_cm;
    float drop_speed_cm_sec;
    float raise_speed_cm_sec;
    float drop_speeds[4];
    float raise_speeds[4];
} PositionConfig_t;

typedef struct FlushTimeConfig_t {
    float lift_tube_time_s;
    unsigned long dump_water_time_s;
    float rope_drop_time_s;
    float rinse_rope_time_s;
    unsigned long rinse_tube_time_s;
} FlushTimeConfig_t;

typedef struct AqusensTimeConfig_t {
    unsigned long air_gap_time_s;
    unsigned long water_rinse_time_s;
    unsigned long last_air_gap_time_s;
} AqusensTimeConfig_t;

// TODO: ?get rid of pointers?
typedef struct FlushConfig_t {
    FlushTimeConfig_t flush_time_cfg;
    AqusensTimeConfig_t aqusens_time_cfg;
} FlushConfig_t;

typedef struct SDConfig_t {
    char* tide_data_name;
    float pier_dist_cm;
} SDConfig_t;

typedef struct TimeUnit_t {
    uint8_t day;    
    uint8_t hour;   
    uint8_t min; 
    uint8_t sec; 
} TimeUnit_t;

typedef struct TimesConfig_t {
    TimeUnit_t sample_interval;
    TimeUnit_t soak_time;
    TimeUnit_t dry_time;
} TimesConfig_t;

typedef struct GlobalConfig_t {
    MotorConfig_t& motor_cfg;
    PositionConfig_t& position_cfg;
    SDConfig_t& sd_cfg;
    TimesConfig_t& times_cfg;
} GlobalConfig_t;


// ========================================================================
// Default Values
// ========================================================================

MotorConfig_t DEF_MOTOR_CFG = {REEL_RAD_CM, GEAR_RATIO, PULSE_PER_REV};

PositionConfig_t DEF_POSITION_CFG = {
    NARROW_TUBE_CM,
    TUBE_CM,
    WATER_LEVEL_CM,
    MIN_RAMP_DIST_CM,
    DROP_SPEED_CM_SEC,
    RAISE_SPEED_CM_SEC,
    {15.0f, 30.0f, 75.0f, 30.0f},
    {25.0f, 50.0f, 10.0f, 1.5f}
};

SDConfig_t DEF_SD_CFG = {"tides.txt", PIER_DEFAULT_DIST_CM};

TimesConfig_t DEF_TIME_CFG  = {
    {0, 0, 15, 0}, 
    {0, 0, 0, 5},
    {0, 0, 0, 8}
};

GlobalConfig_t gbl_cfg = {DEF_MOTOR_CFG, DEF_POSITION_CFG, DEF_SD_CFG, DEF_TIME_CFG};


// lcd.ino
LiquidCrystal_I2C lcd(0x27, 20, 4);

// motor.ino
#define SYSTEM_CLOCK_FREQ       (48000000)
#define PRESCALER_VAL           (8)

#define ALARM_THRESHOLD_VALUE   (988) // Analog value on the Alarm Plus pin that seperates alarm state from non-alarm state.

// TODO: these two enums are redundant @Jack
typedef enum MotorDir {
  CCW, //Lowering
  CW,  //Raising
  OFF
} MotorDir;

typedef enum MotorStatus {
  RAISING,
  LOWERING,
  MOTOR_OFF
} MotorStatus; // For lowering and raising the motor manually

volatile bool toggle = false;
MotorDir global_motor_state = OFF;
float MOTORSPEED_FACTOR;


// position.ino
#define REEL_RADIUS 5  //The gearbox reel's radius, currently is 5cm (eventually move into a config.ino)
#define GEARBOX_RATIO  5 // Geatbox ratio is 5:1, this number states how many motor turns equal one gearbox turn

#define PULSES_TO_DISTANCE(num_pulses) ((pulses * PI * 2 * REEL_RADIUS)/(PULSES_PER_REV * GEARBOX_RATIO)) //Converts a number of pulses to a distance in cm
#define DISTANCE_TO_PULSES(distance_cm) ((GEARBOX_RATIO * PULSES_PER_REV * distance_cm) / (PI * 2 * REEL_RADIUS)) // Converts a distance into the corresponding # of pulses from home

#define SAFE_RISE_SPEED_CM_SEC  (3.0f)
#define SAFE_DROP_DIST_CM       (10.0f)
#define NUM_PHASES              (4UL)
#define FREE_FALL_IND           (2)
#define RAISE_DIST_PADDING_CM   (10.0f)

PositionConfig_t pos_cfg = {0};

float drop_distance_cm;
float tube_position_f; // Stores the current position of the sampler tube relative to the top of the tube in the home position

// sd.ino
#define GMT_TO_PST  (8)
#define JSON_SIZE   (4096)
SDConfig_t sd_cfg = {0};

// settings.ino
tmElements_t next_sample_time, sample_interval, soak_time, dry_time, tube_flush_time, aqusens_flush_time;

uint8_t last_setting_page = 4; // amount of settings pages
uint8_t settings_page = 1; // current settings page

// tube_flush.ino
#define DROP_TUBE_DIST_CM       (40.0f)
#define LIFT_SPEED_CM_S         (0.5f)
#define HOME_TUBE_SPD_CM_S      (2.0f)

#define FRESHWATER_TIME_TO_AQU (30)

unsigned long FLUSH_TIME_S, AQUSENS_TIME_S, TOT_FLUSH_TIME_S;
unsigned long AIR_GAP_TIME_MS, LAST_AIR_GAP_TIME_MS, WATER_RINSE_TIME_MS;
unsigned long DUMP_WATER_TIME_MS, RINSE_TUBE_TIME_MS;

typedef enum FlushState {
  INIT,
  DUMP_TUBE,
  ROPE_DROP,
  RINSE_ROPE_HOME,
  RINSE_TUBE,
  RINSE_AQUSENS,
} FlushState;

// utils.ino
#define TIME_BASED_DEBOUNCE_WAIT_TIME_MS 35
#define PRESS_AND_HOLD_INTERVAL_MS 25

volatile bool estop_pressed = false; // Flag to keep track of E-stop pressed/released

// RTC
RTCZero rtc;

// Solenoids
typedef enum SolenoidState {
  OPEN,
  CLOSED
} SolenoidState;

SolenoidState solenoid_one_state = CLOSED;
SolenoidState solenoid_two_state = CLOSED;

/* Setup and Loop **************************************************************/
void setup() {
  Serial.begin(115200);
  while (!P1.init()) {} // Initialize controller

  init_cfg();

  rtcInit(); //TODO: add screen to input actual time/date to init rtc with
  rtdInit();
  gpioInit();
  estopInit();
  motorInit();

  lcd.init(); // Initialize the LCD
  lcd.backlight(); // Turn on the backlight
  lcd.setCursor(0, 0); // Set cursor to column 0, row 0

  state = CALIBRATE;
  
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
    case FLUSH_TUBE: // Aqusens and Sample device are flushed with filtered freshwater/air
      tubeFlushLoop();
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
    case SET_TUBE_FLUSH_TIME: // Settings option to set dry time
      setTubeFlushTimeLoop();
      break;
    case SET_AQUSENS_FLUSH_TIME: // Settings option to set dry time
      setAqusensFlushTimeLoop();
      break;
    default:
      break;
  }
}

#ifndef CONFIG_H
#define CONFIG_H

/************************* Python Script Comms *************************/
#define STOP_PUMP         "F"
#define BEGIN_SAMPLE      "S"
#define REQUEST_TIME      "C"
#define REQUEST_TIDE_DATA "T"
#define START_PUMP        "P"

/************************* Default Timings *************************/
#define DEFAULT_SAMPLE_INTERVAL_HOUR 4
#define DEFAULT_SAMPLE_INTERVAL_MIN  0
#define DEFAULT_SAMPLE_INTERVAL_SEC  0

#define DEFAULT_SOAK_TIME_MIN  0
#define DEFAULT_SOAK_TIME_SEC  15

#define DEFAULT_DRY_TIME_MIN  0
#define DEFAULT_DRY_TIME_SEC  15

/************************* File & Time Config *************************/
#define TIDE_FILE_NAME    "tides.txt"
#define TIDE_FILE         "tides.txt"
#define CONFIG_FILENAME   ("CONFIG~1.JSO")
#define GMT_TO_PST        (8)

/************************* Motor Configuration (motor.ino) *************************/
#define REEL_RAD_CM       (5.0f)
#define PULSE_PER_REV     (1600)
#define GEAR_RATIO        (5.0f)
#define REEL_RADIUS       5
#define GEARBOX_RATIO     5

#define PULSES_TO_DISTANCE(num_pulses) ((num_pulses * PI * 2 * REEL_RADIUS)/(PULSES_PER_REV * GEARBOX_RATIO))
#define DISTANCE_TO_PULSES(distance_cm) ((GEARBOX_RATIO * PULSES_PER_REV * distance_cm) / (PI * 2 * REEL_RADIUS))

#define SYSTEM_CLOCK_FREQ       (48000000)
#define PRESCALER_VAL           (8)

#define ALARM_THRESHOLD_VALUE   (988) // Analog value on the Alarm Plus pin that seperates alarm state from non-alarm state.

/************************* Positioning Parameters (position.ino) *************************/
#define NARROW_TUBE_CM        (15.0f) 
#define TUBE_CM               (85.0f) 
#define WATER_LEVEL_CM        (15.0f) 
#define MIN_RAMP_DIST_CM      (NARROW_TUBE_CM + TUBE_CM + WATER_LEVEL_CM + 5.0f)
#define DROP_SPEED_CM_SEC     (75.0f)
#define RAISE_SPEED_CM_SEC    (50.0f)
#define SAFE_RISE_SPEED_CM_SEC  (3.0f)
#define SAFE_DROP_DIST_CM       (10.0f)
#define NUM_PHASES              (4UL)
#define FREE_FALL_IND           (2)
#define RAISE_DIST_PADDING_CM   (10.0f)

/************************* Tube Flush Timings & Thresholds (tube_flush.ino) *************************/
// General timings
#define LIFT_TUBE_TIME_S              (5.0f)
#define AIR_BUBBLE_TIME_S             (90.0f)
#define SAMPLE_TO_DEVICE_TIME_S       (90.0f)
#define FLUSH_LINE_TIME_S               (5.0f) //Get this experimentally based on how far we send the line down to flush
#define FRESHWATER_TO_DEVICE_TIME_S   (35.0f)
#define FRESHWATER_FLUSH_TIME_S       (150.0f)
#define FINAL_AIR_FLUSH_TIME_S        (45.0f)
#define FLUSHING_TIME_BUFFER          (10.0f)  //Gives some extra time to each of the steps in flush, ensures proper flushing
#define DEVICE_FLUSH_WATER_TEMP_MAX_C (100.0f) // Change this to the actual value, I assume the max somewhere south of boiling
// #define DUMP_WATER_TIME_S       (5UL)
// #define ROPE_DROP_TIME_S        (40.0f / 15.0f)
// #define RINSE_ROPE_TIME_S       (20.0f)
// #define RINSE_TUBE_TIME_S       (5UL)

// Aqusens timings
#define AIR_GAP_TIME_S          (5)
#define WATER_RINSE_TIME_S      (15)
#define LAST_AIR_GAP_TIME_S     (10)

/************************* SD Storage (sd.ino) *************************/
#define PIER_DEFAULT_DIST_CM    (762.0f)

/************************* Pin Mapping *************************/
// Slots - Position of modules
#define HV_GPIO_SLOT 1
#define RELAY_SLOT   2
#define RTD_SLOT     3

// Keypad (GPIO)
#define KEY_S 0
#define KEY_D 1
#define KEY_U 2
#define KEY_L 3
#define KEY_R 4

// SD Card
#define SD_CS 28

// Motor (GPIO)
#define STEP_POS_PIN   6
#define DIR_POS_PIN    13
#define ALARM_PLUS     A2
#define ALARM_MINUS    A5

// HV GPIO
#define MAG_SENSOR_IO_SLOT 1

// Relays
#define MOTOR_POWER   1
#define SOLENOID_ONE  3
#define SOLENOID_TWO  4

// Misc
#define TOPSIDE_COMP_COMMS_TIMEOUT_MS 10 * 1000

/************************* Temperature Sensors *************************/
typedef enum TempSensor {
  TEMP_SENSOR_ONE = 1,
  TEMP_SENSOR_TWO = 2
} TempSensor;

/************************* State and Fault Enums *************************/
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

// Faults
typedef enum AlarmFault { 
  NONE,
  MOTOR,
  TUBE,
  ESTOP,
  TOPSIDE_COMP_COMMS
} AlarmFault;

//Stages for the flushing procedure
typedef enum FlushStage { 
  DUMP_SAMPLE               = 0,
  AIR_BUBBLE                = 1,
  FRESHWATER_LINE_FLUSH     = 2,
  FRESHWATER_DEVICE_FLUSH   = 3,
  AIR_FLUSH                 = 4,
  HOME_TUBE                 = 5
} FlushStage;

typedef enum MotorDir {
  CCW, //Lowering
  CW,  //Raising
  OFF
} MotorDir;


#endif

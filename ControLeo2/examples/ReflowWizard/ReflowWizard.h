// Include file for the Reflow Wizard
#ifndef REFLOW_WIZARD_H
#define REFLOW_WIZARD_H

// Main menu modes
#define MODE_TESTING                         0
#define MODE_CONFIG                          1
#define MODE_REFLOW                          2
#define NO_OF_MODES                          3

#define NEXT_MODE                            false

// Output type
#define TYPE_UNUSED                          0
#define TYPE_TOP_ELEMENT                     1
#define TYPE_BOTTOM_ELEMENT                  2
#define TYPE_BOOST_ELEMENT                   3
#define TYPE_CONVECTION_FAN                  4
#define NO_OF_TYPES                          5

const char *outputDescription[NO_OF_TYPES] = {"Unused", "Top", "Bottom", "Boost", "Fan"};

// Phases of reflow
#define PHASE_INIT                           0    // Variable initialization
#define PHASE_PRESOAK                        1    // Pre-soak rapidly gets the oven to around 150C
#define PHASE_SOAK                           2    // Soak brings the PCB and components to the same (high) temperature
#define PHASE_REFLOW                         3    // Reflow melts the solder
#define PHASE_WAITING                        4    // After reaching max temperature, wait a bit for heat to permeate and start cooling
#define PHASE_COOLING_BOARDS_IN              5    // The oven door should be open and the boards remain in until components definitely won't move
#define PHASE_COOLING_BOARDS_OUT             6    // Boards can be removed. Remain in this state until another reflow can be started at 50C
#define PHASE_ABORT_REFLOW                   7    // The reflow was aborted or completed.

const char *phaseDescription[] = {"", "Presoak", "Soak", "Reflow", "Waiting", "Cooling", "Cool - open door", "Abort"};

// Tunes used to indication various actions or status
#define TUNE_STARTUP                         0
#define TUNE_TOP_BUTTON_PRESS                1
#define TUNE_BOTTOM_BUTTON_PRESS             2
#define TUNE_REFLOW_DONE                     3
#define TUNE_REMOVE_BOARDS                   4
#define MAX_TUNES                            5

// EEPROM settings
// Remember that EEPROM initializes to 0xFF after flashing the bootloader
#define SETTING_EEPROM_NEEDS_INIT             0    // EEPROM will be initialized to 0 at first run
#define SETTING_D4_TYPE                       1    // Element type controlled by D4 (or fan, unused)
#define SETTING_D5_TYPE                       2    // Element type controlled by D5 (or fan, unused)
#define SETTING_D6_TYPE                       3    // Element type controlled by D6 (or fan, unused)
#define SETTING_D7_TYPE                       4    // Element type controlled by D7 (or fan, unused)
#define SETTING_MAX_TEMPERATURE               5    // Maximum oven temperature.  Relow curve will be based on this (stored temp is offset by 150 degrees)
#define SETTING_SETTINGS_CHANGED              6    // Settings have changed.  Relearn duty cycles

// Learned settings
#define SETTING_LEARNING_MODE                 10   // ControLeo is learning oven response and will make adjustments
#define SETTING_PRESOAK_D4_DUTY_CYCLE         11   // Duty cycle (0-100) that D4 must be used during presoak
#define SETTING_PRESOAK_D5_DUTY_CYCLE         12   // Duty cycle (0-100) that D5 must be used during presoak
#define SETTING_PRESOAK_D6_DUTY_CYCLE         13   // Duty cycle (0-100) that D6 must be used during presoak
#define SETTING_PRESOAK_D7_DUTY_CYCLE         14   // Duty cycle (0-100) that D7 must be used during presoak
#define SETTING_SOAK_D4_DUTY_CYCLE            15   // Duty cycle (0-100) that D4 must be used during soak
#define SETTING_SOAK_D5_DUTY_CYCLE            16   // Duty cycle (0-100) that D5 must be used during soak
#define SETTING_SOAK_D6_DUTY_CYCLE            17   // Duty cycle (0-100) that D6 must be used during soak
#define SETTING_SOAK_D7_DUTY_CYCLE            18   // Duty cycle (0-100) that D7 must be used during soak
#define SETTING_REFLOW_D4_DUTY_CYCLE          19   // Duty cycle (0-100) that D4 must be used during reflow
#define SETTING_REFLOW_D5_DUTY_CYCLE          20   // Duty cycle (0-100) that D4 must be used during reflow
#define SETTING_REFLOW_D6_DUTY_CYCLE          21   // Duty cycle (0-100) that D4 must be used during reflow
#define SETTING_REFLOW_D7_DUTY_CYCLE          22   // Duty cycle (0-100) that D4 must be used during reflow
#define SETTING_SERVO_OPEN_DEGREES            23   // The position the servo should be in when the door is open
#define SETTING_SERVO_CLOSED_DEGREES          24   // The position the servo should be in when the door is closed


#define SETTING_TEMPERATURE_OFFSET            150  // To allow temperature to be saved in 8-bits (0-255)


// Thermocouple
#define THERMOCOUPLE_FAULT(x)                 (x == FAULT_OPEN || x == FAULT_SHORT_GND || x == FAULT_SHORT_VCC)

// Strings stored in Flash
#define STR_CONTROLEO2_REFLOW_OVEN_CONTROLLER_VX_X                                0
#define STR_REFLOW_ABORTED_BECAUSE_OF_THERMOCOUPLE_ERROR                          1
#define STR_BUTTON_PRESSED_ABORTING_REFLOW                                        2
#define STR_OVEN_TOO_HOT_TO_START_REFLOW_PLEASE_WAIT                              3
#define STR_SETTINGS_CHANGED_BY_USER_REINITIALIZING_DUTY_CYCLES                   4
#define STR_LEARNING_MODE_IS_ENABLED_DUTY_CYCLES_MAY_BE_ADJUSTED_IF_NECCESSARY    5
#define STR_ADJUSTMENTS_HAVE_BEEN_MADE_TO_DUTY_CYCLES_FOR_THIS_PHASE_CONTINUING   6
#define STR_ADJUSTMENTS_HAVE_BEEN_MADE_TO_DUTY_CYCLES_FOR_THIS_PHASE_ABORTING     7
#define STR_DUTY_CYCLES_LOWERED_SLIGHTLY_FOR_FUTURE_RUNS                          8
#define STR_DUTY_CYCLES_INCREASED_SLIGHTLY_FOR_FUTURE_RUNS                        9
#define STR_ABORTING_REFLOW_OVEN_CANNOT_REACH_REQUIRED_TEMPERATURE                10
#define STR_PHASE_WAITING                                                         11
#define STR_TURNING_ALL_HEATING_ELEMENTS_OFF                                      12
#define STR_PHASE_COOLING                                                         13
#define STR_OPEN_THE_DOOR                                                         14
#define STR_REFLOW_IS_DONE                                                        15
#define FLASH_LAST_STRING                                                         15

#endif // REFLOW_WIZARD_H

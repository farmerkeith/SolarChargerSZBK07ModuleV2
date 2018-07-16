// MPPT software for a solar charger using a SZBK07 DC-DC converter
// and a MOSFET diode
// The MPPT functions are performed by a class defined in MPPT_SZBK07.h
// Software written by farmerkeith
// last update 14 July 2018

//-----------------------------------------------------------------------------------------------------------------
//////// Arduino pins Connections//////////////////////////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------------------------------------------

// A0 - Solar voltage measurement
// A1 - Solar current measurement
// A2 - Battery voltage measurement

// D9 - DC converter control

#include "MPPT_SZBK07.h"
#include "repeatEvery.h"

// global constants
const unsigned int start_margin = 5000; // mV margin of solar above battery
const unsigned int bat_float = 14600;   // mV float voltage
//const unsigned int bat_float = 12600;   // mV float voltage
const unsigned int bat_current_limit = 10000; // mA

// global variables
unsigned int cv_target = 10000; // for Constant Voltage operation
unsigned int cc_target = 10000; // for Constant Current operation

Type_mode mode = off_mode;
// Type_mode mode = mppt_mode;
// Type_mode mode = cv_mode;
// Type_mode mode = cvcc_mode;

void setup() {
  Serial.begin (115200);
  Serial.println("\n\nMPPT_SZBK07 initial version ");
  // Serial.println(mode);
  // Serial.print("\n line 29 solar_current_scale=");
  // Serial.print(mppt.solar_current_scale);
  mppt.battery_volts (2,164,9); // calibration of battery resistive divider
  mppt.solar_volts (0,403,9); // calibration of solar resistive divider
  mppt.solar_current (1, 100, 1.38, 0.004); // calibration of solar current amplifier
}

void loop() {
  mppt.run_off(mode);
  mppt.run_MPPT(mode);
  mppt.run_CV(mode, cv_target);
  mppt.run_CVCC(mode, cv_target, cc_target);
  repeatEvery(1000, manage_charger);
}

Type_charger_data charger_data; // global variable

// function to manage the charger mode according to solar and battery voltages
// does not include temperature compensation or boost charge with time limit
void manage_charger (bool flag, unsigned long & Time) {
  static unsigned long lastTime;
  if (flag) {
    lastTime = Time;
    // ---------------------------------------------
    charger_data = mppt.get_values();
    Serial.print("\nCharger data: sol_V=");
    Serial.print(charger_data.sol_volt);
    Serial.print(" sol_A=");
    Serial.print(charger_data.sol_amps);
    Serial.print(" bat_V=");
    Serial.print(charger_data.bat_volt);
    Serial.print(" pwm=");
    Serial.print(charger_data.pwm_value);
    Serial.print(" mode=");
    Serial.print(mode);
    long bat_amps = (long)charger_data.sol_amps * charger_data.sol_volt;
    bat_amps /= charger_data.bat_volt;
    if (charger_data.sol_volt < charger_data.bat_volt + start_margin) {
      // not enough solar voltage, charger is off
      mode = off_mode; // set mode
    } else if (bat_amps > bat_current_limit) {
      // too much current, start current limiting
      mode = cvcc_mode; 
      cv_target = bat_float;
      cc_target = bat_current_limit;
    } else if (charger_data.bat_volt > bat_float) {
      // battery voltage over float value, go to CV mode
      mode = cv_mode;  // set mode
      cv_target = bat_float; // set CV target
    } else {
    // track max power point
    mode = mppt_mode;
    }
  }
  // ---------------------------------------------
Time = lastTime;              // required by repeatEvery
}
// end of file


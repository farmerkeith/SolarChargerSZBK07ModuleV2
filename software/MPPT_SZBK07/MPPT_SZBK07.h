// file MPPT_SZBK07.h
// class definition for MPPT
// Software written by farmerkeith
// Last update 14 July 2018

enum Type_mode { // charger mode
  off_mode, mppt_mode, cv_mode, cvcc_mode
} ;

struct Type_charger_data { // 3 unsigned integers
  unsigned int sol_volt;
  unsigned int sol_amps;
  unsigned int bat_volt;
  byte pwm_value;
} ;


class MPPT {
    // public:
  private:
    // private variables
    byte solar_volts_pin = 0;   // pin A0
    byte solar_current_pin = 1; // pin A1
    byte battery_volts_pin = 2; // pin A2
    byte MPPT_pin = 9;          // pin D9 for PWM control of DC converter
    int solar_volts_divider = (long)100000 * (390 + 10) / 10 / 1023;
    unsigned int solar_current_scale = 3.3 * 100000 / 100 / 0.004 / 1023;  // default setting Rlo=3.3K, Rhi=100K, Rs=4 milli ohms
    int battery_volts_divider = (long)100000 * (150 + 10) / 10 / 1023; // default setting 150K, 10K; 16V for 1.0V reading
    int currentOffset = 80;    // adc code for zero current

    int8_t pwm_delta = 1;
    int pwm_value = 128;

    unsigned int sol_volt = 0; // mV 0 to 40,000
    unsigned int sol_amps = 0; // mA 0 to 8,000
    unsigned int bat_volt = 0; // mV 0 to 16,000

    // private functions
    void measure();
    unsigned int measure_solar_volts();
    unsigned int measure_battery_volts();
    unsigned int measure_solar_current();
    void do_mppt(unsigned int volt, unsigned int amps);
    void do_CV (unsigned int target); // fix battery voltage at target
    void do_CVCC (unsigned int Vtarget, unsigned int Ctarget); // fix battery current at target

  public:
    // public functions
    MPPT ();  // constructor with default settings
    // configuration control
    void solar_volts(byte pin, unsigned int Rhi, unsigned int Rlo);   // configure solar voltage divider
    void battery_volts(byte pin, unsigned int Rhi, unsigned int Rlo); // configure battery voltage divider
    void solar_current (byte pin, float Rhi, float Rlo, float Rshunt); // configure solar current sense
    void control_pin(byte pin);  // configure pin for PWM control of the DC converter
    // doing things
    void run_off(Type_mode mode);    // charging off
    void run_MPPT(Type_mode mode);   // maximum power point tracking mode
    void run_CV(Type_mode mode, unsigned int target); // constant voltage mode
    void run_CVCC(Type_mode mode, unsigned int Vtarget, unsigned int Ctarget); // constant current mode
    Type_charger_data get_values (); // read measurements
} mppt;

MPPT::MPPT() { // constructor for MPPT object
  analogReference(INTERNAL); // set analog measurement to 1 V reference

}

void MPPT::solar_volts(byte pin, unsigned int Rhi, unsigned int Rlo) {  // configure solar voltage divider
  solar_volts_pin = pin;
  solar_volts_divider = (long)100000 * (Rhi + Rlo) / Rlo / 1023;

}

void MPPT::solar_current(byte pin, float Rhi, float Rlo, float Rshunt) {  // configure solar voltage divider
  solar_current_pin = pin;
  solar_current_scale = Rlo * 100000 / Rhi / Rshunt / 1023;
}

void MPPT::battery_volts(byte pin, unsigned int Rhi, unsigned int Rlo) {  // configure solar voltage divider
  battery_volts_pin = pin;
  battery_volts_divider = 100000 * (Rhi + Rlo) / Rlo / 1023;
}

void MPPT::control_pin(byte pin) {  // configure pin for PWM control of the DC converter
  MPPT_pin = pin;
}

void MPPT::run_off(Type_mode mode) { // charging off
  if (mode != off_mode) return;
  static unsigned long task_time = millis();
  if ((long)(millis() - task_time) >= 500) {
    do task_time += 500; while (millis() > task_time);
    // reduced frequency of measurement when off
    // ---------------------------------------------------
    // Serial.print("\n line 95 off run at ");
    // Serial.print((float) millis() / 1000, 3);
    measure();
    // static int old_sol_amps = sol_amps;
    // if (old_sol_amps != 0) {
    //   currentOffset += (long)old_sol_amps * 100  / solar_current_scale + 1;
    // }
    // Serial.print(" sol_amps=");
    // Serial.print(sol_amps);
    // Serial.print(" currentOffset=");
    // Serial.print(currentOffset);
    analogWrite (MPPT_pin, 255); // minimum output voltage from SZBK07
    // ---------------------------------------------------
  }
}

void MPPT::run_MPPT(Type_mode mode) { // maximum power point tracking mode
  // cycle every 100 ms
  if (mode != mppt_mode) return;
  static unsigned long task_time = millis();
  if ((long)(millis() - task_time) >= 100) {
    do task_time += 100; while (millis() > task_time);
    // ---------------------------------------------------
    // Serial.print("\n line 107 mppt run at ");
    // Serial.print((float) millis() / 1000, 3);
    measure();
    do_mppt(sol_volt, sol_amps); // maximise power
    // ---------------------------------------------------
  }
}

void MPPT::run_CV(Type_mode mode, unsigned int target) { // constant voltage mode
  if (mode != cv_mode) return;
  static unsigned long task_time = millis();
  if ((long)(millis() - task_time) >= 100) {
    do task_time += 100; while (millis() > task_time);
    // ---------------------------------------------------
    // Serial.print("\n line 128 CV run at ");
    // Serial.print((float) millis() / 1000, 3);
    measure();
    // Serial.print(" currentOffset=");
    // Serial.print(currentOffset);
    do_CV (target); // fix battery voltage at target
    // ---------------------------------------------------
  }
}

void MPPT::run_CVCC(Type_mode mode, unsigned int Vtarget, unsigned int Ctarget) { // constant current mode
  if (mode != cvcc_mode) return;
  static unsigned long task_time = millis();
  if ((long)(millis() - task_time) >= 100) {
    do task_time += 100; while (millis() > task_time);
    // ---------------------------------------------------
    // Serial.print("\n line 136 CVCC run at ");
    // Serial.print((float) millis() / 1000, 3);
    measure();
    do_CVCC (Vtarget, Ctarget); // fix battery current at target
    // ---------------------------------------------------
  }
}

void MPPT::measure() {
  sol_volt = measure_solar_volts();   // mV 0 to 40,000
  // Serial.print("\nline145 sol_volt=");
  // Serial.print(sol_volt);
  sol_amps = measure_solar_current(); // mA 0 to 8,000
  bat_volt = measure_battery_volts(); // mV 0 to 16,000
}

unsigned int MPPT::measure_solar_volts() {
  // static int counter = 0;
  // counter+=100;
  unsigned int mV = analogRead(solar_volts_pin);
  // if (counter >1023) counter = 1023;
  // mV -= counter;
  // Serial.print("\nline158 solar volts mV code=");
  // Serial.print(mV);
  mV = (long)mV * solar_volts_divider / 100;
  // return 500;
  return mV;
}

unsigned int MPPT::measure_battery_volts() {
  // static int counter = 0;
  // counter+=100;
  unsigned int mV = analogRead(battery_volts_pin);
  // if (counter >1023) counter = 1023;
  // mV -= counter;
  // Serial.print("\nline169 battery volts mV code=");
  // Serial.print(mV);
  mV = (long)mV * battery_volts_divider / 100;
  // return 12000;
  return mV;
}

unsigned int MPPT::measure_solar_current() {
  // static int counter = 0;
  // counter+=100;
  unsigned int mA = analogRead(solar_current_pin);
  // Serial.print("\nline194 solar current mA code=");
  // Serial.print(mA);
  if (mA > currentOffset) mA -= currentOffset;
  else mA = 0;
  // if (counter >1023) counter = 1023;
  // mA -= counter;
  mA = (long)mA * solar_current_scale / 100;
  //  return 0;
  return mA;
}

void MPPT::do_mppt(unsigned int volt, unsigned int amps) { // does mppt
  // changes pwm_value to maximise power
  static unsigned long old_watts = 0;
  unsigned long watts = (long)volt * amps / 1000;   // calculate new power, milliwatts
  // Serial.print("\n line 185 do_mppt at ");
  // Serial.print((float) millis() / 1000, 3);
  // Serial.print(" power=");
  // Serial.print(watts);
  if (watts < old_watts) { // power has gone down - change direction
    pwm_delta = - pwm_delta;
  }       // "else" power is the same or increased  - keep direction
  old_watts = watts;
  pwm_value += pwm_delta;
  pwm_value = constrain (pwm_value, 0, 255);
  analogWrite (MPPT_pin, pwm_value);
}

void MPPT::do_CV (unsigned int target) { // fix battery voltage at target
  // Serial.print("\n line 207 do_CV at ");
  // Serial.print((float) millis() / 1000, 3);
  if (bat_volt > target) { // increase pwm value
    pwm_delta = abs(pwm_delta);
  } else {                // decrease pwm value
    pwm_delta = - abs(pwm_delta);
  }
  pwm_value += pwm_delta;
  pwm_value = constrain (pwm_value, 0, 255);
  analogWrite (MPPT_pin, pwm_value);
}

void MPPT::do_CVCC (unsigned int Vtarget, unsigned int Ctarget) {
  // stay at or below voltage and current targets
  // Serial.print("\n line 222 do_CVCC at ");
  // Serial.print((float) millis() / 1000, 3);
  unsigned int bat_amps = (long)sol_amps * sol_volt / bat_volt;
  if (bat_amps > Ctarget || bat_volt > Vtarget) { // increase pwm value to decrease charging
    pwm_delta = abs(pwm_delta);
    pwm_value += pwm_delta;
    pwm_value = constrain (pwm_value, 0, 255);
    analogWrite (MPPT_pin, pwm_value);
  } else {                // increase charging subject to capacity
    do_mppt(sol_volt, sol_amps);
  }
}

Type_charger_data MPPT::get_values () { // read measurements
  Type_charger_data charger_data;      // working variable
  // Serial.print("\nline220 sol_volt=");
  // Serial.print(sol_volt);
  charger_data.sol_volt = sol_volt;
  charger_data.sol_amps = sol_amps;
  charger_data.bat_volt = bat_volt;
  charger_data.pwm_value = pwm_value;
  return charger_data;
}


// end of file


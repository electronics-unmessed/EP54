//
// ********************************** Arduino Nano Digital Filter and Stepper Gauge **************************************************
// Stepper Motor X27 directly driven by Arduino Nano
// 0.5° per step  -> 90° -> 180 steps
// Rotation Angle: Max 315°
// 0.5°/full step
// Coil resistance: 280 +/- 20Ω: 5V -> 18mA
// White LEDs power consumption 2* 2.5V/390 Ohms -> 0 .. 13mA with full brightness
// Arduino Nano 15mA
// Microphone power consumption (?) my best guess 5mA
// Total consumption: 18mA + (0..13mA) + 15mA + 5mA = 38..51mA

// Features:

//       Start-up procedure shows 0% -> 100% and relative settings of potentiometers
//       SW definition for gain, zero adjust, filter damping, fiter resonance frequency, LED brightness
//       LED goes to full brightness on Start-Up and if audio level is over 70%
//       Automatic sleep after time and wake-up on audio level
//       Show filter parameter adjustments on clapping noise

// Results from Tests:
// Find corresponding by searching for "??_1, ??_2, .. "

// ??_0:  Any input signal is possible, analalog 0..5 Volt, digital from any source like sensors, interfaces or operations
// ??_1:  Stepper speed, can be 1 to 30, I chose 10 in order not to loose steps with extremely volatile audio signals
// ??_2:  Delay corresponds to the stepper speed, I chose 8333 micro-seconds
// ??_3:  Value defines movement for stepper. Depends on volatility of your signal.
//        e.g. 0 -> pointer goes to exact position.
//            10 -> pointer remains until difference between "to be" and "actual" position is greater 10.
// ??_4: threshold audio level for "wake-up"
// ??_5: runtime, if audio is below threshold
// ??_6: starts void Start_Up again when strong audio signal appears

#include <Stepper.h>
#define STEPS 720  // steps per revolution (limited to 315°) 720
#define COIL1 8
#define COIL2 9
#define COIL3 10
#define COIL4 11

// create an instance of the stepper class:
Stepper stepper(STEPS, COIL1, COIL2, COIL3, COIL4);

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// +++++ Software Definitions, which you can adjust +++++++++++++++++++++++++++

float Gain = 0.8;        // 0 .. 10 Gain of Gauge to calibrate  (typical 0.5 .. 5)
float Damping = 1.0;    // 0 .. 1  Filter Tuning               (typical 0.2 .. 2)
float Resonance = 0.02;  // 0 .. 1   Filter Tuning             (typical 0.001 .. 0.5)

int zero_adjust = 0;       // adjust pointer to zero

float LoWBrightness = 25;   // Brightness for normal operation LEDs PWM 0..255
float MaxBrightness = 255;  // Brightness for Clipping and and Start-up LEDs PWM 0..255

// +++++ END Software Definitions, which you can adjust +++++++++++++++++++++++
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// Definitions for the Dial Angles

int Full_Deflection = 180; // No of Steps for full deflection 90 degrees in my case
int zero_offset = 20;  // Offset from left block to Zero on Scale
int Zero_Under = 5;    // Clipping at Zero Underschoot
int Max_Over = 215;    // Clipping at Maximum Overshoot

// Definitions for Sleep Time and Wake-up

long run_time = 300000;   // ??_5: Runtime in case audio < threshold
float threshold = 2.0;    // ??_4: threshold for "wake-up"
long run_count = 300000;  // counter for runtime count down


float Brightness = 100;  // Brightness for LEDs PWM 0..255

int pos = 0;  //Position in steps(0-630)= (0°-315°)
int val = 0;  // to be Position in Steps 0..180 = 90°

int speed = 25;    // ??_1: Rounds per minute (max 30)
int Delay = 8;     // ??_2: 10-30 RPM 8ms OK with microphone
float diff = 1.0;  // ??_3: 0..10 difference val-pos for stepper to get active

float no_average = 5;  // averaging for audio reference level

int LED_Lights = 6;  // defines PIN for LED

// +++++++++++++++++

const int analogInPin0 = A0;  // Analog in Voltage
const int analogInPin1 = A1;  // Analog in Resonance
const int analogInPin2 = A2;  // Analog in Damping
const int analogInPin3 = A3;  // Analog in Adjust Zero
const int analogInPin4 = A4;  // Analog in Adjust Gain
const int analogInPin5 = A5;  // Analog in Brightness

float counter = 0;
float increment = 1.0;
int check_count = 0;
long count = 0;
int max_count = 0;
int count_rectangle = 0;

float inputA0 = 0;  // Variable for Analog Input
float inputA1 = 0;  // Variable for Analog Input
float inputA2 = 0;  // Variable for Analog Input
float inputA3 = 0;  // Variable for Analog Input
float inputA4 = 0;  // Variable for Analog Input
float inputA5 = 0;  // Variable for Analog Input

float voltA0 = 0;
float voltA0_OLD = 0;
float voltA0_average = 0;
float voltA0_average_OLD = 0;
float voltA0_max = 0;
float voltA0_max_OLD = 0;

float audio = 0;
float audio_max = 0;
float audio_max_log = 0;
float audio_OLD = 0;

float val_A1 = 0;
float val_A1_OLD = 0;
float val_A2 = 0;
float val_A2_OLD = 0;
float val_A3 = 0;
float val_A3_OLD = 0;
float val_A4 = 0;
float val_A4_OLD = 0;
float val_A5 = 0;
float val_A5_OLD = 0;

float Testfunktion = 0;
float Rectangle = 0;
int No_high = 600;  //  EXCEL: 438
int No_low = 1200;  // EXCEL: 2500

// Filter Input Output

float FilterInput_0 = 0;
float FilterInput_1 = 0;
float FilterInput_2 = 0;

float FilterOutput_0 = 0;
float FilterOutput_1 = 0;
float FilterOutput_2 = 0;

// Butterworth Coefficients

float a0 = 1.980000;
float a1 = -0.98096528;
float b0 = 0.00024132;
float b1 = 0.00048264;
float b2 = 0.00024132;

// Filter Coefficients for Tuning

float a0_t = 1.8445;
float a1_t = -0.94102800;
float b0_t = 0.00024132;
float b1_t = 0.00048264;
float b2_t = 0.00024132;

// ******************* End Definitions ***********************************************************************************

// ******************** Set-up *******************************************************************************************

void setup() {
  Serial.flush();
  Serial.begin(38400);
  Serial.setTimeout(200);  // Standard is 1000m
  Serial.flush();

  pinMode(LED_Lights, OUTPUT);

  stepper.setSpeed(speed);  // set the motor speed to 30 RPM (360 PPS aprox.).
  stepper.step(630);        //Reset Position(630 steps counter-clockwise).
}

// ***************************** Main Program ***************************************************************************
// **********************************************************************************************************************

void loop() {

  A0_Read_Voltage();

  // Serial.println(run_count) ;

  if (audio > threshold) { run_count = run_time; };
  if (run_count < 4) {
    run_count = 2;
    Brightness = 0;
  };
  Lights();

  // Serial.println(run_count);

  if (run_count > 2) {

    if (audio > 0.7 * threshold) { run_count = run_time; };

    run_count--;

    // Values remain FIX as defined above!

    // A1_Read_Damping();
    // A2_Read_Resonance();
    // A3_Read_Zero_Adjust();
    // A4_Read_Gain();
    // A5_Read_Brightness();

    Brightness = LoWBrightness;

    //  Lights();
    //  RectangleFct();  // Test function
    //  FilterInput_0 = 25 + 140 * Rectangle;

    FilterInput_0 = 2 * Gain * audio_max_log;  //

    FilterTuning();
    FilterFct();
    // Variation();

    val = int(FilterOutput_0);

    if (audio > 70) {  // ??_6: starts void Start_Up again when strong audio signal appears
      count = 0;
    };

    Start_Up();  // ??_6: moves pointer -> 0 -> 100% -> % gain -> % damping -> % resonance frequency -> audio signal

    val = val + zero_offset + zero_adjust;

    // Check_Up();  // moves pointer to 50% and remains some seconds, check whether steps slipped

    Clipping();

    Drive_Stepper();

    // ******** Sequence for Testing
    /*
  counter = counter + 1;
  if (counter > 100) {
    SerialPrint();
    // SerialPlot();
    counter = 0;
  };
   */

    // delayMicroseconds(Delay);
    // delay(Delay);
  };
}

// ************ End Main Loop ********************************************************************************************
// ***********************************************************************************************************************

// ********* Variation of Resonance and Damping **************************************************************************

void SerialPrint() {

  Serial.print("VoltA0: ");
  Serial.print(voltA0_average);
  Serial.print(", ");
  Serial.print("Audio: ");
  Serial.print(audio);
  Serial.print(", ");
  Serial.print("logAudio: ");
  Serial.print(audio_max_log);
  Serial.print(", ");
  Serial.print("Damp: ");
  Serial.print(Damping);
  Serial.print(", ");
  Serial.print("Res: ");
  Serial.print(Resonance);
  Serial.print(", ");
  Serial.print("Zero: ");
  Serial.print(zero_adjust);
  Serial.print(", ");
  Serial.print("Gain: ");
  Serial.print(Gain);
  Serial.print(", ");
  Serial.print("Bright: ");
  Serial.print(Brightness);
  Serial.print(", ");
  Serial.print("Fi-In: ");
  Serial.print(FilterInput_0);
  Serial.print(", ");
  Serial.print("Fi-Out: ");
  Serial.print(FilterOutput_0);
  Serial.print(", ");
  Serial.print("Val: ");
  Serial.print(val);
  Serial.print(", ");
  Serial.print("Pos: ");
  Serial.print(pos);
  // Serial.print(", ");
  Serial.println("");
}

void SerialPlot() {

  Serial.print(220.0);
  Serial.print(", ");
  Serial.print(0.0);
  Serial.print(", ");

  Serial.print(val);
  Serial.print(", ");
  // Serial.print("Pos: ");
  Serial.println(pos);
}

// ********* Drive Stepper Motor **************************************************************************


void Drive_Stepper() {

  if (abs(val - pos) > diff) {  // ??_3:  moves stepper, if diference is greater than diff steps.
    if ((val - pos) > 0) {
      stepper.step(-1);  // move one step to the left.
      // delay(Delay);
      pos++;
    }
    if ((val - pos) < 0) {
      stepper.step(1);  // move one step to the right.
      // delay(Delay);
      pos--;
    }
    if ((val - pos) == 0) {
      // delay(Delay);
    }
    delay(Delay);
  }
}


// ********* Variation of Resonance and Damping **************************************************************************

void RectangleFct() {

  // ******** Rectangle Test Function ******************************************************************************************

  count_rectangle = count_rectangle + 1;

  if (count_rectangle < No_high) {
    Rectangle = 1.0;
  } else {
    Rectangle = 0.0;
  };
  if (count_rectangle > No_low) { count_rectangle = 0; };
}

// ********* Variation of Resonance and Damping **************************************************************************

void FilterFct() {

  // ******** Filter Function *************************************************************************************************

  a0 = 1.94 + 0.005 * ((1 - Damping * Damping * Damping * Damping) / (0.01 * Damping * Damping * Damping * Damping + 0.0849) - 0.192929 * (Resonance - 1));

  a1 = 1 - a0 - b0 - b1 - b2;

  FilterOutput_0 = b0 * FilterInput_0 + b1 * FilterInput_1 + b2 * FilterInput_2 + a0 * FilterOutput_1 + a1 * FilterOutput_2;

  FilterOutput_2 = FilterOutput_1;
  FilterOutput_1 = FilterOutput_0;
  FilterInput_2 = FilterInput_1;
  FilterInput_1 = FilterInput_0;
}

void FilterTuning() {
  // ************** Filter Tuning **********************************************************************************************

  b0 = Resonance * b0_t;
  b1 = Resonance * b1_t;
  b2 = Resonance * b2_t;
}

void Variation() {
  // ************** Filter Variation *********************************************************************************************

  if (counter > No_low) {
    Resonance = Resonance * 4.0;
    counter = 0;
  };
  if (Resonance > 1000) { Resonance = 0.1; };
}

void A0_Read_Voltage() {

  inputA0 = analogRead(analogInPin0);
  voltA0 = map(inputA0, 0, 1023, 0, 1023);  // ??_0: any input signal can be chosen instead

  voltA0_OLD = voltA0;
  voltA0_average_OLD = voltA0_average;
  voltA0_max_OLD = voltA0_max;
  audio_OLD = audio;

  voltA0_average = (no_average - 1) / no_average * voltA0_average_OLD + 1 / no_average * voltA0;

  audio = abs(voltA0 - voltA0_average);
  audio = 0.2 * audio + 0.8 * audio_OLD;
  if (audio < 0) { audio = 0; };

  //audio_max_log = 4000 * log10(0.07 * (audio-2.5) + 1.0);
  audio_max_log = 500 * (log10(2.0 * (audio) + 0.1) - 0.00);


  if (audio_max_log < 0) { audio_max_log = 0; };
}

void A1_Read_Damping() {

  inputA1 = analogRead(analogInPin1);
  val_A1_OLD = val_A1;
  val_A1 = map(inputA1, 0, 1023, 1023, 0);
  val_A1 = 0.9 * val_A1_OLD + 0.1 * val_A1;
  Damping = val_A1 / 300;
}

void A2_Read_Resonance() {

  inputA2 = analogRead(analogInPin2);
  val_A2_OLD = val_A2;
  val_A2 = map(inputA2, 0, 1023, 1023, 0);
  val_A2 = 0.9 * val_A2_OLD + 0.1 * val_A2;
  Resonance = val_A2 / 600;
}

void A3_Read_Zero_Adjust() {

  inputA3 = analogRead(analogInPin3);
  val_A3_OLD = val_A3;
  val_A3 = map(inputA3, 0, 1023, 1023, 0);
  val_A3 = 0.9 * val_A3_OLD + 0.1 * val_A3;
  zero_adjust = int(80 * val_A3 / 1023 - 40);
}

void A4_Read_Gain() {

  inputA4 = analogRead(analogInPin4);
  val_A4_OLD = val_A4;
  val_A4 = map(inputA4, 0, 1023, 1023, 0);
  val_A4 = 0.9 * val_A4_OLD + 0.1 * val_A4;
  Gain = 2.0 * val_A4 / 1023;
}

void A5_Read_Brightness() {

  inputA5 = analogRead(analogInPin5);
  val_A5_OLD = val_A5;
  val_A5 = map(inputA5, 0, 1023, 255, 0);
  val_A5 = 0.9 * val_A5_OLD + 0.1 * val_A5;
  Brightness = val_A5;
}

void Clipping() {

  if (val < Zero_Under) { val = Zero_Under; };
  if (val > Max_Over) { val = Max_Over; };
}

void Start_Up() {
  count = count + 1;
  if (count < 10 * no_average) {
    // val = 76 - diff; // shows -> 42 <- on a 100% Scale (the answer to all questions of ths universe)
    Brightness = MaxBrightness;
  };


  if (count < 5000) { val = Full_Deflection * 10 * Resonance + diff; };  // shows rel Resonance
  if (count < 4000) { val = Full_Deflection * Damping + diff; };    // shows rel Damping
  if (count < 3000) { val = Full_Deflection * Gain + diff; };       // shows rel Gain


  if (count < 2000) { val = Full_Deflection + diff; };
  if (count < 1000) { val = 0 + diff; };
  if (count > 10000) { count = 10000; };
}

void Check_Up() {
  check_count = check_count + 1;
  if (check_count > 12000) { val = 110; };
  if (check_count > 16000) {
    check_count = 0;
    val = 110;
    delay(2000);
  };
}

void Lights() {
  if (val < Full_Deflection + zero_offset + zero_adjust ) {
    analogWrite(LED_Lights, Brightness);
  } else {
    analogWrite(LED_Lights, MaxBrightness);
  };
}

// ******** End **********************************************************************************************************

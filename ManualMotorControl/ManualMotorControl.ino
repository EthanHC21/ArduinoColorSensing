/* 
 *  Step C0 C1 C2 C3
 *    1  1  0  1  0
 *    2  0  1  1  0
 *    3  0  1  0  1
 *    4  1  0  0  1
 */

// Number of steps per output rotation
int stepsPerRevolution = 200;

// array to hold pins
int feederPins[] = {10, 11, 12, 13};
int conveyorPins[] = {6, 7, 8, 9};

// speeds in RPM
const int feederSpeed = 3;
const int conveyorSpeed = 20;

// store direction
boolean feederDir = true;
boolean conveyorDir = false;

// step number for each motor
byte feederStepNum = 1;
byte conveyorStepNum = 1;

// store the previous time value
unsigned long t_prev_feeder = 0;
unsigned long t_prev_conveyor = 0;
// store the current time value
//unsigned long t_now_ = 0;

// calculate step time delay
long feederStepDelay = long(1.0/(float(feederSpeed) * float(stepsPerRevolution) / float(60000000)));
long conveyorStepDelay = long(1.0/(float(conveyorSpeed) * float(stepsPerRevolution) / float(60000000)));

// store whether motor is running
boolean motorOn = true;

void setup()
{
  for (int i = 0; i < 4; i++) {

    pinMode(feederPins[i], OUTPUT);
    pinMode(conveyorPins[i], OUTPUT);
    
  }

  Serial.begin(9600);
  Serial.print("feederStepDelay is ");
  Serial.print(feederStepDelay);
  Serial.print("\n");
  Serial.print("coneyorStepDelay is ");
  Serial.print(conveyorStepDelay);
  Serial.print("\n");

  t_prev_feeder = micros(); // only goes for 71min
  t_prev_conveyor = micros();
  
}

void loop() 
{
  
  // get current time
  unsigned long t_now = micros();
  
  // check if this is bigger than either motor's delay
  if ((t_now - t_prev_feeder) > feederStepDelay) {
    // move the feeder
    moveFeeder();
    // set the previous time to the time just used for calculation
    t_prev_feeder = t_now;
  }

  if ((t_now - t_prev_conveyor) > conveyorStepDelay) {
    // move the conveyor
    moveConveyor();
    // set the previous time to the time jut used for calculation
    t_prev_conveyor = t_now;
  }
  
}

void moveFeeder() {

  switch (feederStepNum) {
    case 1: // 1010
      digitalWrite(feederPins[0], HIGH);
      digitalWrite(feederPins[1], LOW);
      digitalWrite(feederPins[2], HIGH);
      digitalWrite(feederPins[3], LOW);
    break;
    case 2: // 0110
      digitalWrite(feederPins[0], LOW);
      digitalWrite(feederPins[1], HIGH);
      digitalWrite(feederPins[2], HIGH);
      digitalWrite(feederPins[3], LOW);
    break;
    case 3: // 0101
      digitalWrite(feederPins[0], LOW);
      digitalWrite(feederPins[1], HIGH);
      digitalWrite(feederPins[2], LOW);
      digitalWrite(feederPins[3], HIGH);
    break;
    case 4: // 1001
      digitalWrite(feederPins[0], HIGH);
      digitalWrite(feederPins[1], LOW);
      digitalWrite(feederPins[2], LOW);
      digitalWrite(feederPins[3], HIGH);
    break;
  }
  
  if (!feederDir) { // increment step number
    feederStepNum++;
    if (feederStepNum == 5) feederStepNum = 1;
  } else { // decrement step number
    feederStepNum--;
    if (feederStepNum == 0) feederStepNum = 4;
  }
  
}

void moveConveyor() {

  switch (conveyorStepNum) {
    case 1: // 1010
      digitalWrite(conveyorPins[0], HIGH);
      digitalWrite(conveyorPins[1], LOW);
      digitalWrite(conveyorPins[2], HIGH);
      digitalWrite(conveyorPins[3], LOW);
    break;
    case 2: // 0110
      digitalWrite(conveyorPins[0], LOW);
      digitalWrite(conveyorPins[1], HIGH);
      digitalWrite(conveyorPins[2], HIGH);
      digitalWrite(conveyorPins[3], LOW);
    break;
    case 3: // 0101
      digitalWrite(conveyorPins[0], LOW);
      digitalWrite(conveyorPins[1], HIGH);
      digitalWrite(conveyorPins[2], LOW);
      digitalWrite(conveyorPins[3], HIGH);
    break;
    case 4: // 1001
      digitalWrite(conveyorPins[0], HIGH);
      digitalWrite(conveyorPins[1], LOW);
      digitalWrite(conveyorPins[2], LOW);
      digitalWrite(conveyorPins[3], HIGH);
    break;
  }
  
  if (!conveyorDir) { // increment step number
    conveyorStepNum++;
    if (conveyorStepNum == 5) conveyorStepNum = 1;
  } else { // decrement step number
    conveyorStepNum--;
    if (conveyorStepNum == 0) conveyorStepNum = 4;
  }
  
}

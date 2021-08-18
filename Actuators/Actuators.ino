#include "DEV_Config.h"
#include "TCS34725.h"
#include "util.h"
#include "LinkedListLib.h"
RGB rgb,RGB888;
UWORD   RGB565=0;

// define pins
const byte ORANGEPIN = 51;
const byte GREENPIN = 52;
const byte BLUEPIN = 53;
const byte YELLOWPIN = 50;
const byte KEYPIN = 12;
const byte SPEAKERPIN = 10;

//for comparison
const byte NONE = 0;
const byte YELLOW = 1;
const byte ORANGE = 2;
const byte GREEN = 3;
const byte BLUE = 4;


const unsigned long YELLOWPERIOD = 2700;  //the value is a number of milliseconds
const unsigned long ORANGEPERIOD = 5200;  //the value is a number of milliseconds
const unsigned long GREENPERIOD = 7700;  //the value is a number of milliseconds
const unsigned long BLUEPERIOD = 9750;  //the value is a number of milliseconds

// Hues
int H_Y = 51;
int H_O = 5;
int H_G = 129;
int H_B = 210;


unsigned long currentMillis;
unsigned long activeValves[4] = {0}; //[0 YELLOW] [1 ORANGE] [2 GREEN] [3 BLUE] per above (const - 1);
const unsigned long VALVEDELAY = 500; //in milliseconds


//current detected item struct
typedef struct
 {
    unsigned long detectedMillis; //32 bits
    byte color; //8 bits
 }  DetectedItem;

const byte NUMITEMS = 5;
DetectedItem detectedItems[NUMITEMS] = {0};
byte curItems = 0;

// Array to hold background values
const int NUMCOLSBG = 20;
RGB bgVals[NUMCOLSBG];

// byte to hold oldest value in bg array
byte bgOldest = 0;

// floats to hold average color values
float avgR, avgG, avgB;

// floats for standard deviation for color values
float sigmaR, sigmaG, sigmaB;

// store the critical P value
//float P = 1.72913; // 19DOF, 95%
float P = 10.0;


// LinkedList to hold values of sensed object
LinkedList<RGB> objVals;
// minimum number of statistically different measurments to signify a detection
// this is based on the conveyor speed. It's half the number of measurements we
// should get on a single M&M of 15mm (Cameron, Emma-Leigh P.)

const float V_BELT = .009; // velocity of conveyor belt in m/s

const float INTEG = .05; // integration time of sensor in s

// number of object detections to signify an actual object
const int OBJ_THRESH = 12; //(int) (.015 / V_BELT / INTEG / 2); // anything less than
// 16 measurements is considered noise

// number of consecutive bg measurements to signify detection is over
const int OBJ_END = 1; // should be one, we're not using dark colors
// but we can change it if it doesn't work

// number of consecutive measurements to signify we have detected an object
const int OBJ_BEG = 5; // ORIGINALLY 3

// number of consecutive object measurements detected
int nObj = 0;
// number of consecutive bg measurements detected
int nBg = 0;

// boolean to store whether we're in object recording mode or not
boolean detecting = false;

// floats to hold averages from the detected objects array
float avgObjR, avgObjG, avgObjB;

// I added an abort button because I wanted
boolean abortMode = false;

void setup() {
  Config_Init();
  if(TCS34725_Init() != 0){
      Serial.print("TCS34725 initialization error!!\r\n");
      return 0; 
  } 
  Serial.print("TCS34725 initialization success!!\r\n");

  delay(500);

  // define pin modes
  pinMode(ORANGEPIN, OUTPUT);
  pinMode(GREENPIN, OUTPUT);
  pinMode(BLUEPIN, OUTPUT);
  pinMode(YELLOWPIN, OUTPUT);
  pinMode(KEYPIN, INPUT);
  pinMode(SPEAKERPIN, OUTPUT);

  Serial.print("Sensor Setup complete");
  Serial.print("\r\n");

  /*
  Serial.print("Size of bgVals array is ");
  Serial.print(NUMCOLSBG);
  Serial.print("\r\n");
  */

  // populate array
  for (byte i = 0; i < NUMCOLSBG; i++) {
    /*
    Serial.print("Populating value ");
    Serial.print(i);
    Serial.print("\r\n");
    */
    bgVals[i] = TCS34725_GetRGB888(TCS34725_Get_RGBData());

    // debug print statements
    /*
    Serial.print("R is ");
    Serial.print(bgVals[i].R);
    Serial.print("  G is ");
    Serial.print(bgVals[i].G);
    Serial.print("  B is ");
    Serial.print(bgVals[i].B);
    Serial.print("\r\n");
    */
    
    delay(50);
  }

  Serial.print("Initial BG sensing done.\r\n");

  updateStats();

  /*
  int arr[] = {1,2,3,4,5};
  arrayRemove(arr, 5, 3);
  for(int i =0; i<5; i++) {
    Serial.print(arr[i]);
    Serial.print("\r\n");
  }
  */
  
}

void removeObject(DetectedItem arr[NUMITEMS], int index)
{
  for(int i = index; i<NUMITEMS - 1; i++)
  {
    arr[i].color = arr[i+1].color;
    arr[i].detectedMillis = arr[i+1].detectedMillis;
  }
  arr[NUMITEMS - 1].color = 0;
  arr[NUMITEMS - 1].detectedMillis = 0;
  return;
}

void loop() {

  //check for active valves
  currentMillis = millis();
  for (int i = 0; i<4; i++)
  {
    if (activeValves[i] > 0 && currentMillis - activeValves[i] >= VALVEDELAY)
    {
      if (i + 1 == YELLOW)
      {
        digitalWrite(YELLOWPIN, LOW);
        activeValves[i] = 0;
      }
     else if(i + 1 == ORANGE)
     {
        digitalWrite(ORANGEPIN, LOW);
        activeValves[i] = 0;
     }
     else if(i + 1 == GREEN)
     {
        digitalWrite(GREENPIN, LOW);
        activeValves[i] = 0;
     }
     else if(i + 1 == BLUE)
     {
        digitalWrite(BLUEPIN, LOW);
        activeValves[i] = 0;
     }
    }
  }
  
  /*
    double arr[5] = {0};
    arrayRemove(arr, 5, 3);
    for (int i = 0; i<5; i++)
    {
      printf("%f\n", arr[i]);
    }
    
    arrayAdd(arr, 5, 523.33);
    
    for (int i = 0; i<5; i++)
    {
      printf("%f\n", arr[i]);
    }
  */

  byte obj = NONE;
  
  // get RGB value from sensor
  rgb=TCS34725_Get_RGBData();
  RGB888=TCS34725_GetRGB888(rgb);

  // first check whether we're detecting the color of an object
  if (detecting) {

    //Serial.print("Mode = Detecting\r\n");
    
    // check if we detected an object
    if (detectObject(RGB888)) {

      //Serial.print("Object detected...\r\n");
      
      // if so, we add the detection to the array of object color values
      objVals.InsertTail(RGB888);

      // and we set the number of consecutive bg measurements to 0
      nBg = 0;
      
    } else {

      // if not, we increment the number of consecutive bg measurments
      nBg++;

      Serial.print("Object not detected - this is ");
      Serial.print(nBg);
      Serial.print(" consecutive background detections.\r\n");
      
      // check how many consecutive bg detections we've made
      if (nBg > OBJ_END) {

        Serial.print("DETECTING MODE DISENGAGED...\r\n");
        
        // this means we are no longer detecting the object
        detecting = false;

        //Serial.print("There were ");
        //Serial.print(objVals.GetSize());
        //Serial.print(" successful object detections.\r\n");
        
        // check the number of measurements
        if (objVals.GetSize() > OBJ_THRESH) {

          // get object color and compute valve number and timing
          //Serial.print("Object measuremnt threshold exceeded with, color would be reported here.\r\n");
          obj = getColor();

          /*if (obj == YELLOW){

            digitalWrite(YELLOWPIN, HIGH);
            delay(500);
            digitalWrite(YELLOWPIN, LOW);
            
          } else if (obj == ORANGE) {

            digitalWrite(ORANGEPIN, HIGH);
            delay(500);
            digitalWrite(ORANGEPIN, LOW);
            
          } else if (obj == GREEN) {

            digitalWrite(GREENPIN, HIGH);
            delay(500);
            digitalWrite(GREENPIN, LOW);
            
          } else {

            digitalWrite(BLUEPIN, HIGH);
            delay(500);
            digitalWrite(BLUEPIN, LOW);
          }*/
          
        }

        // reset the linked list
        objVals.Clear();
        
        // reset the number of consecutive background measurements because we don't need it anymore
        nBg = 0;
        
      }
      
    }
    
  } else {

    //Serial.print("Mode = Sampling\r\n");

    // check if we detected an object
    if (detectObject(RGB888)) {
      
      // if we detected an object we need to update the counter of detected measurements
      nObj++;

      /*
      Serial.print("Object detected - this is ");
      Serial.print(nObj);
      Serial.print(" consecutive object detections.\r\n");
      */
      
      // now we check how many consecutive object detections we've made
      if (nObj > OBJ_BEG) {

        //Serial.print("DETECTING MODE ENGAGED...\r\n");

        // if we've made more than the number required to begin color recognition
        // we enable color recognition
        detecting = true;

        // we add this most recent detection to the LinkedList of object measurements
        objVals.InsertTail(RGB888);

        // we reset the number of consecutive objects detected to 0 since we don't need it anymore
        nObj = 0;
        
      }
    } else {

      //Serial.print("Object not detected...");
      
      // if not, we set the number of consecutive object detections to 0
      nObj = 0;
      
    }
  }

  if (obj != 0)
  {
    //add new object to array
    detectedItems[curItems].color = obj;
    detectedItems[curItems].detectedMillis = millis();
    curItems++;
  }

  /*
  Serial.print("curItems is ");
  Serial.print(curItems);
  Serial.print("\r\n");
  */
  //check current array
  currentMillis = millis();
  for (int i = 0; i<curItems; i++)
  {
    DetectedItem curItem = detectedItems[i];
    switch (curItem.color) {
      case YELLOW:
        if (currentMillis - curItem.detectedMillis >= YELLOWPERIOD)
        {
          removeObject(detectedItems, i);
          curItems--;
          activeValves[YELLOW-1] = millis();
          digitalWrite(YELLOWPIN, HIGH);
          i--;
        }
        break;
      case ORANGE:
          if (currentMillis - curItem.detectedMillis >= ORANGEPERIOD)
          {
            removeObject(detectedItems, i);
            curItems--;
            activeValves[ORANGE-1] = millis();
            digitalWrite(ORANGEPIN, HIGH);
            i--;
          }
        break;
      case GREEN:
          if (currentMillis - curItem.detectedMillis >= GREENPERIOD)
          {
            removeObject(detectedItems, i);
            curItems--;
            activeValves[GREEN-1] = millis();
            digitalWrite(GREENPIN, HIGH);
            i--;
          }
        break;
      case BLUE:
          if (currentMillis - curItem.detectedMillis >= BLUEPERIOD)
          {
            removeObject(detectedItems, i);
            curItems--;
            activeValves[BLUE-1] = millis();
            digitalWrite(BLUEPIN, HIGH);
            i--;
          }
        break;
      default:
        break;
    }
  }
  

  /*
  // write the colors to the RGB LED
  analogWrite(REDPIN, RGB888.R);
  analogWrite(GREENPIN, RGB888.G/4);
  analogWrite(BLUEPIN, RGB888.B/2);
  */

  // turn all LEDs on for demonstration purposes
  /*
  digitalWrite(REDPIN, HIGH);
  analogWrite(BLUEPIN, 127);
  digitalWrite(GREENPIN, HIGH);
  digitalWrite(YELLOWPIN, HIGH);
  */

  if (digitalRead(KEYPIN) == HIGH) {
    abortMode = true;
    soundAlarm();
  }
  
  DEV_Delay_ms(50); // note that this is added to the time delay of the color sensor
  
}

void updateStats() {

  // reset averages
  avgR = 0;
  avgG = 0;
  avgB = 0;
  
  //  loop through and get the sum of all values
  for (byte i = 0; i < NUMCOLSBG; i++) {
    
    // get RGB values
    rgb = bgVals[i];
    
    avgR += rgb.R;
    avgG += rgb.G;
    avgB += rgb.B;
  }
  // calculate average and save it
  avgR /= float(NUMCOLSBG);
  avgG /= float(NUMCOLSBG);
  avgB /= float(NUMCOLSBG);

  // debug print statement
  /*
  Serial.print("Average R is ");
  Serial.print(avgR);
  Serial.print("  Average G is ");
  Serial.print(avgG);
  Serial.print("  Average B is ");
  Serial.print(avgB);
  Serial.print("\r\n");
  */

  // reset standard deviations
  sigmaR = 0;
  sigmaG = 0;
  sigmaB = 0;
  
  // loop through for standard deviation calculation
  for (byte i = 0; i < NUMCOLSBG; i++) {

    // get RGB values
    rgb = TCS34725_GetRGB888(bgVals[i]);
    
    sigmaR += pow(rgb.R - avgR, 2);
    sigmaG += pow(rgb.G - avgG, 2);
    sigmaB += pow(rgb.B - avgB, 2);
  }
  // calculate and store standard deviations
  sigmaR = sqrt(sigmaR/float(NUMCOLSBG));
  sigmaG = sqrt(sigmaG/float(NUMCOLSBG));
  sigmaB = sqrt(sigmaB/float(NUMCOLSBG));

  // debug print statements
  /*
  Serial.print("StDev R is ");
  Serial.print(sigmaR);
  Serial.print("  StDev G is ");
  Serial.print(sigmaG);
  Serial.print("  StDev B is ");
  Serial.print(sigmaB);
  Serial.print("\r\n");
  */
  
}

boolean detectObject(RGB sensorVal) {

  // get z-vals of each color
  float zR = (sensorVal.R - avgR)/sigmaR;
  float zG = (sensorVal.G - avgG)/sigmaG;
  float zB = (sensorVal.B - avgB)/sigmaB;

  // debug print statements
  /*
  Serial.print("R is ");
  Serial.print(sensorVal.R);
  Serial.print("  G is ");
  Serial.print(sensorVal.G);
  Serial.print("  B is ");
  Serial.print(sensorVal.B);
  Serial.print("\r\n");

  Serial.print("zR is ");
  Serial.print(zR);
  Serial.print("  zG is ");
  Serial.print(zG);
  Serial.print("  zB is ");
  Serial.print(zB);
  Serial.print("\r\n");
  */

  // check if these have a greater magnitude than the P-values
  // note that P-values are calculated manually and input to the program
  if (abs(zR) > P || abs(zG) > P || abs(zB) > P) { // if any z is above P, we've detected an object

    /*
    Serial.print("Object detected");
    Serial.print("\r\n");
    */

    return true;
    
  } else { // if no z is above the P, then we take the current data point as a sample
    // if we're not detecting color (this is done to avoid unnecessary error, like for
    // example if half of measurement is on the M&M)

    if (!detecting) {
      // set this new RGB value as the bgOldest one in the array
      bgVals[bgOldest] = sensorVal;
  
      // increment 
      bgOldest++;
      // if it equals the length of the array, wrap it around
      if (bgOldest == NUMCOLSBG) bgOldest = 0;

      /*
      Serial.print("Insignificant change detected, updating background array");
      Serial.print("\r\n");
      */
    }

    return false;
  }
}

byte getColor() {

  // reset averages
  avgObjR = 0;
  avgObjG = 0;
  avgObjB = 0;
  
  // get average of R, G, and B vals
  //  loop through and get the sum of all values
  for (byte i = 0; i < objVals.GetSize(); i++) {
    
    // get RGB values
    rgb = objVals.GetAt(i);
    
    avgObjR += rgb.R;
    avgObjG += rgb.G;
    avgObjB += rgb.B;
    
  }
  
  // calculate average and save it
  avgObjR /= float(objVals.GetSize());
  avgObjG /= float(objVals.GetSize());
  avgObjB /= float(objVals.GetSize());

  // get hue
  float H;
  
  float Rp = avgObjR/255;
  float Gp = avgObjG/255;
  float Bp = avgObjB/255;

  float Cmax = max(Rp, max(Gp, Bp));
  float Cmin = min(Rp, min(Gp, Bp));

  float triangle = Cmax - Cmin;

  if(Cmax == Rp) {
    H = 60 * (Gp-Bp)/triangle;
  } else if (Cmax == Gp) {
    H = 60 * ((Bp-Rp)/triangle + 2);
  } else {
    H = 60 * ((Rp-Gp)/triangle + 4);
  }

  // check hue is finite
  if(!isnan(H)) {
  
    if (H < 0) {
      H += 360;
    }
  
    
    Serial.print("Measured hue is ");
    Serial.print(H);
    Serial.print("\r\n");
    
  
    // convert reference hues for easy comparison
    // yellow
    int H_Y_obj = H_Y - H;
    while(!(H_Y_obj > -180 && H_Y_obj < 180)) {
      if (H_Y_obj > 0) {
        H_Y_obj -= 360;
      } else {
        H_Y_obj += 360;
      }
    }
  
    // orange
    int H_O_obj = H_O - H;
    while(!(H_O_obj > -180 && H_O_obj < 180)) {
      if (H_O_obj > 0) {
        H_O_obj -= 360;
      } else {
        H_O_obj += 360;
      }
    }
    
    // green
    int H_G_obj = H_G - H;
    while(!(H_G_obj > -180 && H_G_obj < 180)) {
      if (H_G_obj > 0) {
        H_G_obj -= 360;
      } else {
        H_G_obj += 360;
      }
    }
    
    // blue
    int H_B_obj = H_B - H;
    while(!(H_B_obj > -180 && H_B_obj < 180)) {
      if (H_B_obj > 0) {
        H_B_obj -= 360;
      } else {
        H_B_obj += 360;
      }
    }
  
    // get smallest difference from measurement
    int d_Y = abs(H_Y_obj);
    int d_O = abs(H_O_obj);
    int d_G = abs(H_G_obj);
    int d_B = abs(H_B_obj);
  
    int d_min = min(d_Y, min(d_O, min(d_G, d_B)));
  
    
    if (d_min == d_Y) {
      
      Serial.print("Color is yellow");
      Serial.print("\r\n");
      
      return 1;
    } else if (d_min == d_O) {
      
      Serial.print("Color is orange");
      Serial.print("\r\n");
      
      return 2;
    } else if (d_min == d_G) {
      
      Serial.print("Color is green");
      Serial.print("\r\n");
      
      return 3;
    } else {
      
      Serial.print("Color is blue");
      Serial.print("\r\n");
      return 4;
      
    }
  
  }
  
}

void soundAlarm() {

  while (abortMode) {

    digitalWrite(SPEAKERPIN, HIGH);

    delay(1000);

    digitalWrite(SPEAKERPIN, LOW);

    delay(1000);

    if (digitalRead(KEYPIN) == HIGH) {
      abortMode = false;
    }
    
  }

  Serial.print("Sensing will resume in 3...\r\n");
  delay(1000);
  Serial.print("Sensing will resume in 2...\r\n");
  delay(1000);
  Serial.print("Sensing will resume in 1...\r\n");
  delay(1000);
  
}

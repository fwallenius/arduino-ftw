
#define FALSE 0
#define TRUE 1
#define LEFT 0
#define RIGHT 1
#define MIDDLE 2

#define LEFTLED       9
#define LEFTTRIGGER   4
#define LEFTTECHO     5

#define RIGHTLED      8
#define RIGHTTRIGGER  2
#define RIGHTECHO     3

#define SERVOCONTROL  6

#include <Servo.h>
Servo servo;

const int TIME_PER_MOVE = 500;  // How long should it take for Storm Trooper to move from one pos to another

void setup() {
  Serial.begin(9600);
  servo.attach(SERVOCONTROL);
  
  pinMode(LEFTLED, OUTPUT);
  pinMode(RIGHTLED, OUTPUT);

  pinMode(RIGHTTRIGGER, OUTPUT);
  pinMode(RIGHTECHO, INPUT);

  pinMode(LEFTTRIGGER, OUTPUT);
  pinMode(LEFTTECHO, INPUT);

  //calibrate();
  servo.write(90);
}

// Read 20 values with a slight delay between.
// If no anomalies are found - use medium of values as "base"
void calibrate() {
  int samples = 20;
  int leftVals[samples], rightVals[samples];

  readDistance(LEFTTRIGGER, LEFTTECHO);
  readDistance(RIGHTTRIGGER, RIGHTECHO);
  delay(1000);
  digitalWrite(LEFTLED, HIGH);
  digitalWrite(RIGHTLED, HIGH);

  Serial.println("Calibrate:");

  for (int i = 0; i < samples; i++) {
    leftVals[i] = readDistance(LEFTTRIGGER, LEFTTECHO);
    rightVals[i] = readDistance(RIGHTTRIGGER, RIGHTECHO);

    Serial.print(leftVals[i]);
    Serial.print(", ");
    Serial.println(rightVals[i]);

    delay(100);
  }

  sortArray(leftVals, samples);
  sortArray(rightVals, samples);

  Serial.println("\nSorted, without extremes:");
  int sumL = 0, sumR = 0;
  for (int i = 2; i < samples - 2; i++) {
    Serial.print(leftVals[i]);
    Serial.print(", ");
    Serial.println(rightVals[i]);
    sumL += leftVals[i];
    sumR += rightVals[i];
  }
  
  Serial.println("Averages:");
  Serial.print(sumL / (samples - 4));
  Serial.print(", ");
  Serial.println(sumR / (samples - 4));

}




unsigned long lastRead = 0;
unsigned long time;
long currentDistCmRight = 200;
long currentDistCmLeft = 200;

void loop() {
  time = millis(); //millis since program started

  reader();
  mover();
  blinker(LEFT, LEFTLED, currentDistCmLeft);
  blinker(RIGHT, RIGHTLED, currentDistCmRight);

  delay(1);
}




void reader() {
  if (time - lastRead > 100) {
    currentDistCmLeft = readDistance(LEFTTRIGGER, LEFTTECHO);
    currentDistCmRight = readDistance(RIGHTTRIGGER, RIGHTECHO);
    lastRead = time;
  }  
}

long readDistance(int triggerPin, int echoPin) {

  long duration;

  // The PING))) is triggered by a HIGH pulse of 2 or more microseconds.
  // Give a short LOW pulse beforehand to ensure a clean HIGH pulse:
  digitalWrite(triggerPin, LOW);
  delayMicroseconds(2);
  digitalWrite(triggerPin, HIGH);
  delayMicroseconds(5);
  digitalWrite(triggerPin, LOW);

  duration = pulseIn(echoPin, HIGH);

  // convert the time into a distance
  return microsecondsToCentimeters(duration);
}

long microsecondsToCentimeters(long microseconds)
{
  // The speed of sound is 340 m/s or 29 microseconds per centimeter.
  // The ping travels out and back, so to find the distance of the
  // object we take half of the distance travelled.
  return microseconds / 29 / 2;
}




// Called for every main loop
// Can read from 'time', 'currentDistCmRight' and 'currentDistCmLeft' (globals)
unsigned long mLastDecisionTime = 0;
unsigned long mLastStepTime = 0;
int mTimePerDegree;
int mCurrentPosition = MIDDLE;
byte mIsMoving = FALSE;
int mCurrentAngle = 90;
int mStepPerTime;

const int ANGLES[] = {20, 160, 90}; 
void mover() {
  
  if (time - mLastDecisionTime > TIME_PER_MOVE + 500) {  // +500 to allow a small break after moving to new position
    
    mLastDecisionTime = time;
    Serial.println("Time to decide!");
    
    Serial.print(currentDistCmLeft);
    Serial.print(", ");
    Serial.println(currentDistCmRight);
    
    if (abs(currentDistCmRight - currentDistCmLeft) < 30) {
      Serial.println(" Middle!");
      mCurrentPosition = MIDDLE;
      if (mCurrentAngle > ANGLES[MIDDLE]) {
        mStepPerTime = -1;
      } else {
        mStepPerTime = +1;
      }
    } else {
      
      if (currentDistCmRight < currentDistCmLeft) {
        Serial.println(" Right!");
        mCurrentPosition = RIGHT; 
        mStepPerTime = +1;
      } else {
        Serial.println(" LEFT!");
        mCurrentPosition = LEFT;
        mStepPerTime = -1;
      }      
    }
    
    mIsMoving = TRUE;
    // Need to move amount of degrees: mCurrentAngle - ANGLES[mCurrentPosition] = 70
    // Time I should spend doing it: TIME_PER_MOVE (1000?)
    mTimePerDegree = TIME_PER_MOVE / abs(mCurrentAngle - ANGLES[mCurrentPosition]);
    mLastStepTime = time;
    Serial.print("I should move 1 degree / ");
    Serial.print(mTimePerDegree);
    Serial.println(" ms");
  }
  
  if (mIsMoving == TRUE && (time - mLastStepTime > mTimePerDegree)) {
    mCurrentAngle += mStepPerTime;
    servo.write(mCurrentAngle);
    mLastStepTime = time;
  }
  
  if (mCurrentAngle == ANGLES[mCurrentPosition]) {
    mIsMoving = FALSE; // We seem to be where we should
  }
  
}





// Called for every main loop
// Can read from 'time' (global)
int isOn[] = {FALSE, FALSE};
unsigned long lastDecisionTime[] = {0, 0};
unsigned long lastBlinkTime[] = {0, 0};
int blinkDelay[] = {100, 100};

void blinker(int side, int ledPin, long distance) {

  if (time - lastBlinkTime[side] > blinkDelay[side]) { // Turn light on/off
    if (isOn[side] == TRUE) {
      digitalWrite(ledPin, LOW);
      isOn[side] = FALSE;
    } else {
      if (distance < 190) {
        digitalWrite(ledPin, HIGH);
        isOn[side] = TRUE;
      }
    }
    lastBlinkTime[side] = time;
  }

  if (time - lastDecisionTime[side] > 100) {  // Update blink rate
    blinkDelay[side] = distance * 4;
  }

}


void sortArray(int ar[], int len) {
  //Bubblesort ftw!

  for (int i = 0; i < len - 1; i++) {
    for (int j = i + 1; j < len; j++) {
      if (ar[i] > ar[j]) {
        int temp = ar[i];
        ar[i] = ar[j];
        ar[j] = temp;
      }
    }
  }

}








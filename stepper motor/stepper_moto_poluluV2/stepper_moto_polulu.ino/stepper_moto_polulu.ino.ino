// Include the AccelStepper Library
#include <AccelStepper.h>

// Define pin connections
const int dirPin = 4;
const int stepPin = 3;

const int halfPin = 2;

// Define motor interface type
#define motorInterfaceType 1

// Creates an instance
AccelStepper myStepper(motorInterfaceType, stepPin, dirPin);

void setup() {

  digitalWrite(halfPin,HIGH); 
  Serial.begin(9600);
  Serial.println("start  ");
  // set the maximum speed, acceleration factor,
  // initial speed and the target position
  myStepper.setMaxSpeed(1000);
  myStepper.setAcceleration(100);
  myStepper.setSpeed(1000);
  myStepper.moveTo(2000);

  Serial.println("end  ");
}

void loop() {
  // Change direction once the motor reaches target position
  if (myStepper.distanceToGo() == 0) 
  {
    myStepper.moveTo(-myStepper.currentPosition());
    
  }
//myStepper.moveTo(200);
  // Move the motor one step
  myStepper.run();
}

// Include the AccelStepper Library
#include <AccelStepper.h>

// Define pin connections
const int dirPin = 2;
const int stepPin = 3;

// Define motor interface type
#define motorInterfaceType 1

// Creates an instance
AccelStepper myStepper(motorInterfaceType, stepPin, dirPin);

void setup() {
  // set the maximum speed, acceleration factor,
  // initial speed and the target position
  myStepper.setMaxSpeed(1000);
  myStepper.setAcceleration(100);
  myStepper.setSpeed(1000);
  myStepper.moveTo(200);
}

void loop() {
  // Change direction once the motor reaches target position
  //if (myStepper.distanceToGo() == 0) 
    //myStepper.moveTo(-myStepper.currentPosition());
myStepper.moveTo(20000);
  // Move the motor one step
  myStepper.run();
}

#include "Wire.h"
#include <Adafruit_NeoPixel.h>
#include <SoftwareSerial.h>
#include <Adafruit_BMP280.h>

#define IR_INTERNAL A7  //SWAP for toilet
#define IR_EXTERNAL A2
#define MOTOR_POS_PIN  A6
#define MOTOR_PWR_CTRL 13
#define MOTOR_PWM 11
#define MOTOR_RST 10
#define MOTOR_PHASE 12
#define MOTOR_CURRENT_PIN A0
#define MOTOR_CURRENT2_PIN A1


#define MOTOR_OPEN_POS 300
#define MOTOR_CLOSE_POS 600

#define MOTOR_JAM_CURRENT1 18




int motorPosition;
int motorADCCurrent1;//Mosfet Driver
int motorADCCurrent2;//ACS712
int max_current1;
int max_current2;
int processCounter=0;
byte MCP_DATA;
int gap;
byte stuck_status;

//For WIFI Transmission
byte DoorPos[200];

/*void sendWIFI()
{
 
  WIFISerial.write('a');
  WIFISerial.write(locID);
  WIFISerial.write(doorID);
  WIFISerial.write(open_status);
  WIFISerial.write(lock_status);
  WIFISerial.write(temperature);
  WIFISerial.write(maintain_status);
  WIFISerial.write(alert_status);
  WIFISerial.write(alert_status);
  WIFISerial.write(alert_status);
  Serial.println("WIFI SENT");
}
*/

void CloseDoor(byte inData)
{
  float diff;
  int motorSpeed;
  float speed_kp=0;
  int maxSpeed=150;
  int minSpeed=70;
  int maxProcessCounter=100;
    processCounter=0;

     unsigned long start_time = millis();
      unsigned long end_time = 0;
     int index=0;
  while(1)
  {
    processCounter++;
    Serial.println("---------Door Closing---------");
    //position is less than MOTOR_CLOSE_POS
   motorPosition=analogRead(MOTOR_POS_PIN);
   motorADCCurrent1=analogRead(MOTOR_CURRENT_PIN);
   if(max_current1<motorADCCurrent1)
        max_current1=motorADCCurrent1;
   motorADCCurrent2=analogRead(MOTOR_CURRENT2_PIN);
   if(max_current2<motorADCCurrent2)
        max_current2=motorADCCurrent2;

   if(millis() - start_time> 100 && index < 200)
    {
      float percentage = ((  motorPosition - MOTOR_OPEN_POS) * 255.0) / (MOTOR_CLOSE_POS - MOTOR_OPEN_POS);
      
      DoorPos[index] = percentage; 
      Serial.println(DoorPos[index]);
      index++;
      start_time = millis();
    }
   
   Serial.print("Door Position: ");  Serial.println(motorPosition);
   Serial.print("Current 1: ");  Serial.println(motorADCCurrent1);
   Serial.print("Current 2: ");  Serial.println(motorADCCurrent2);    
   
    if(motorPosition<MOTOR_CLOSE_POS)
    {
       diff=MOTOR_CLOSE_POS-motorPosition;
       Serial.print("Door Diff: ");  Serial.println(diff);
       Serial.print("Door gap: ");  Serial.println(gap);
      speed_kp=(diff/gap)*maxSpeed;
      
      Serial.print("Speed: ");  Serial.println(speed_kp);    

     
        if(speed_kp<minSpeed) speed_kp=minSpeed;
        
        if(speed_kp>maxSpeed) speed_kp=maxSpeed;
        Serial.print("Final Speed: ");  Serial.println(speed_kp);    
        motorSpeed=speed_kp;
          
     
    }else{
           /* motorSpeed=120;
            Serial.println(".....Closing Reached... ");
            analogWrite(MOTOR_PWM,motorSpeed);
            */
            
            motorSpeed=150;
            Serial.println(".....Closing Reached... ");
            analogWrite(MOTOR_PWM,motorSpeed);
            delay(1200);            //delay to force close
            motorSpeed=0;
            Serial.println(".....Closing Reached... ");
            analogWrite(MOTOR_PWM,motorSpeed);
            end_time = millis();

            Serial.println(".....Printing Pos Array... ");
            for(int i = 0 ; i < sizeof(DoorPos)  ; i++)
            {
             //  Serial.println(DoorPos[i]);
            }
            
            break;    
    }
    if(processCounter>maxProcessCounter)
      {
        Serial.print("Process Counter: ");  Serial.println(processCounter);
        Serial.print("Possible Stuck Detected!! Alerts");
        motorSpeed=0;
        beep(3);
        analogWrite(MOTOR_PWM,motorSpeed);
        delay(10000);
        processCounter=0;
      }
    if(motorADCCurrent1>=MOTOR_JAM_CURRENT1 && processCounter>5 && stuck_status==0)
   {
      Serial.print("**Cosing Door Jam Detected ");
     // motorSpeed=0;
      //analogWrite(MOTOR_PWM,motorSpeed);
      //digitalWrite(MOTOR_PWR_CTRL,LOW);
      stuck_status=1;
      beep(3);
     // OpenDoor(1);
      stuck_status=0;
      delay(5000);
    
   }
    
     Serial.print("Door Speed: ");  Serial.println(motorSpeed);
     analogWrite(MOTOR_PWM,motorSpeed);
      digitalWrite(MOTOR_PHASE,HIGH);
      digitalWrite(MOTOR_PWR_CTRL,HIGH);
      delay(50);
  }
   resetMotor();
}


void OpenDoor(byte inStatus)
{
  float diff;
  int motorSpeed;
  float speed_kp=0;
  int maxSpeed=190;
  int minSpeed=100;
  int maxProcessCounter=50;
  processCounter=0;
  max_current1=0;
  max_current2=0;
  
  while(1)
  {
    processCounter++;
     Serial.println("---------Door Opening---------");
    //position is less than MOTOR_CLOSE_POS
     motorPosition=analogRead(MOTOR_POS_PIN);
     motorADCCurrent1=analogRead(MOTOR_CURRENT_PIN);
     if(max_current1<motorADCCurrent1)
          max_current1=motorADCCurrent1;
     motorADCCurrent2=analogRead(MOTOR_CURRENT2_PIN);
     if(max_current2<motorADCCurrent2)
          max_current2=motorADCCurrent2;
     
     Serial.print("Door Position: ");  Serial.println(motorPosition);
     Serial.print("Current 1: ");  Serial.println(motorADCCurrent1);
     Serial.print("Current 2: ");  Serial.println(motorADCCurrent2);     
     
  
    if(motorPosition>MOTOR_OPEN_POS)
    {
      diff=motorPosition-MOTOR_OPEN_POS;
      speed_kp=(diff/gap)*maxSpeed;
      
      Serial.print("Door Diff: ");  Serial.println(diff);
      Serial.print("Door gap: ");  Serial.println(gap);
      Serial.print("Speed: ");  Serial.println(speed_kp);    
        
      if(speed_kp<minSpeed) speed_kp=minSpeed;
      if(speed_kp>maxSpeed) speed_kp=maxSpeed;
           Serial.print("Final Speed: ");  Serial.println(speed_kp);    
          motorSpeed=speed_kp;
          
          Serial.println("......Opening ... ");
          analogWrite(MOTOR_PWM,motorSpeed);
          

    }else{
      
        motorSpeed=0;
        Serial.println("Opening Reached... ");
        analogWrite(MOTOR_PWM,motorSpeed);
        
        break;     
    }

     if(processCounter>maxProcessCounter)
      {
        Serial.print("Opening Process Counter: ");  Serial.println(processCounter);
        Serial.print("**Possible Stuck Detected!! Alerts");
        motorSpeed=0;
        beep(2);
        analogWrite(MOTOR_PWM,motorSpeed);
        delay(10000);
        processCounter=0;
      }

     if(motorADCCurrent1>=MOTOR_JAM_CURRENT1 && processCounter>5 && stuck_status==0)
     {
        Serial.print("**Opening Door Jam Detected ");
        stuck_status=1;
        beep(3);
        CloseDoor(1);
        stuck_status=0;
        delay(4000);
     }   


     Serial.print("Door Speed: ");  Serial.println(motorSpeed);
     analogWrite(MOTOR_PWM,motorSpeed);
     digitalWrite(MOTOR_PHASE,LOW);
     digitalWrite(MOTOR_PWR_CTRL,HIGH);
     delay(50);
  }
    
  resetMotor();
}

void setup() {
  // put your setup code here, to run once:
  pinMode(MOTOR_PWR_CTRL,OUTPUT);
  pinMode(MOTOR_RST,OUTPUT);
  pinMode(MOTOR_PWM,OUTPUT);
  pinMode(MOTOR_PHASE,OUTPUT);
  Serial.begin(9600);
  resetMotor();
  setupMCP23008();
  MCP_DATA=0x00;
  setMCP(MCP_DATA);
  
  max_current1=0;
  max_current2=0;
  
}

void loop() {
  // put your main code here, to run repeatedly:
  
  Serial.println("Door Closing...");
  max_current1=0;
  max_current2=0;
  
  CloseDoor(1);
  Serial.print("Max Current 1: ");  Serial.println(max_current1);
  Serial.print("Max Current 2: ");  Serial.println(max_current2);  
  delay(5000);
  Serial.println("Door Opening...");
  
  max_current1=0;
  max_current2=0;
  OpenDoor(1);
   Serial.print("Max Current 1: ");  Serial.println(max_current1);
   Serial.print("Max Current 2: ");  Serial.println(max_current2);  

  delay(5000);
}


void resetMotor()
{
  digitalWrite(MOTOR_PWR_CTRL,LOW);
  delay(50);
  digitalWrite(MOTOR_PWM,LOW);
  digitalWrite(MOTOR_RST,LOW);delay(50);
  digitalWrite(MOTOR_RST,HIGH);delay(50);
  digitalWrite(MOTOR_RST,LOW);delay(50);
  digitalWrite(MOTOR_RST,HIGH);delay(50);

  digitalWrite(MOTOR_RST,HIGH);
  //Serial.println("Motor Reset...");
}


void setupMCP23008()
{
  Wire.begin();
  Wire.beginTransmission(0x20);
  Wire.write(0x00);
  Wire.write(0x00);
  Wire.endTransmission();
}

void setMCP(byte flag)
{
  Wire.beginTransmission(0x20);
  Wire.write(0x09);
  Wire.write(flag);
  Wire.endTransmission();
}


void beep(byte cnt)
{
  byte i;
  for(i=0; i<cnt;i++)
  {
    onSiren(1);
    delay(100);
    onSiren(0);
    delay(100);    
  }
  
}


void onSiren(byte flag)
{
  if(flag==1)
  { bitSet(MCP_DATA, 6);
  }else{    
    bitClear(MCP_DATA, 6);
  }
  setMCP(MCP_DATA);
}

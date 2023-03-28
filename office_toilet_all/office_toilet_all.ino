#include <dummy.h>

#include "Wire.h"
#include <Adafruit_NeoPixel.h>
#include <SoftwareSerial.h>



byte doorID=7;// Toilet 1 (4),  Toilet 2 (5),  Toilet 3 (6)
byte locID=2;//HQ

#define IR_INTERNAL A7  //SWAP for toilet
#define IR_EXTERNAL A2
#define MOTOR_POS_PIN  A6
#define MOTOR_PWR_CTRL 13
#define MOTOR_CURRENT_PIN A0
#define MOTOR_CURRENT2_PIN A1
#define EM_LOCK_PIN 2
#define MAINTENANCE_PIN A3
#define NEO_PIN 3
#define NUMPIXELS      10
#define MP3_PIN   9
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, NEO_PIN, NEO_GRB + NEO_KHZ800);
SoftwareSerial mp3Serial(10, MP3_PIN); // RX, TX
SoftwareSerial WIFISerial(7,6); 


 //set volume
static int8_t set_Max_volume[] = {0x7E , 0x03 , 0x31 , 0x5F , 0xEF}; //
// Select storage device to TF card
static int8_t select_SD_card[] = {0x7e, 0x03, 0X35, 0x01, 0xef}; // 7E 03 35 01 EF
// Play with index: /01/001xxx.mp3
static int8_t play_first_song[] = {0x7e, 0x04, 0x41, 0x00, 0x01, 0xef}; // 7E 04 41 00 01 EF
// Play with index: /01/002xxx.mp3
static int8_t play_second_song[] = {0x7e, 0x04, 0x41, 0x00, 0x02, 0xef}; // 7E 04 41 00 02 EF

static int8_t play_cmd[] = {0x7e, 0x04, 0x42, 0x00, 0x01, 0xef}; 
// Play the song.
static int8_t play[] = {0x7e, 0x02, 0x01, 0xef}; // 7E 02 01 EF
// Pause the song.
static int8_t pause[] = {0x7e, 0x02, 0x02, 0xef}; // 7E 02 02 EF
void send_command(int8_t command[], int len);

#define MP3_DOOR_CLOSING      1
#define MP3_DOOR_OPENING      2
#define MP3_DOOR_LOCKED       3
#define MP3_DOOR_SEE_U_AGAIN  4
#define MP3_MAINTAINENCE      5 
#define MP3_DOOR_CLOSE_TAP_LOCK      6
#define MP3_DOOR_OPEN_TAKE_BELONGS  7 

#define MOTOR_PWM 11
#define MOTOR_RST 10
#define MOTOR_PHASE 12


#define ROOM_FREE  1
#define ROOM_OCCUPIED  2
#define ROOM_MAINTENANCE  3

#define COLOR_RED 1
#define COLOR_ORANGE 2
#define COLOR_GREEN 3
  //doorClose ==370
  //Door Open
#define MOTOR_OPEN_POS 400
#define MOTOR_CLOSE_POS 750
#define MOTOR_SEMIOPEN_POS 650

#define MOTOR_JAM_CURRENT1 48
#define MOTOR_JAM_CURRENT_OPEN 50
#define MOTOR_JAM_CURRENT_CLOSE 50


int stage;
int motorPosition;
int maintenanceStatus;
int maintenance;
int motorADCCurrent;//Mosfet Driver
int motorADCCurrent2;//ACS712
int gap;

/*
 * 
 * Motor IDLE
 Mosfet CS Value ->  0
  AS712 CS Value ->  290-300
 Motor Moving
 Mosfet CS Value ->  15-20
  AS712 CS Value ->  660-670
 */

int Entrance_IR;//IR at external door, 
int Exit_IR;//IR at internal door
int potentiometerPin = A6;
byte MCP_DATA;
int processCounter=0;

void resetMotor();
void open_door();
void close_door();
int motorMoveCounter;

//For timer
unsigned long startTime;

//For WIFI Transmission
byte open_status=1;
byte lock_status=0;
byte temperature=23;
byte maintain_status=0;
byte alert_status;
byte stuck_status;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);   
  mp3Serial.begin(9600); 
  WIFISerial.begin(9600);

  pinMode(IR_INTERNAL, INPUT);
  pinMode(IR_EXTERNAL, INPUT);
  pinMode(MAINTENANCE_PIN, INPUT_PULLUP);
 // pinMode(LED_BLUE_PIN,OUTPUT);
  pinMode(EM_LOCK_PIN,OUTPUT);
  pinMode(MOTOR_PWR_CTRL,OUTPUT);
  pinMode(MOTOR_RST,OUTPUT);
  pinMode(MOTOR_PWM,OUTPUT);
  pinMode(MOTOR_PHASE,OUTPUT);

  Serial.println("EcoPlus Bi-Folding Door Program v 13 Dec 2021  ");
  Serial.println("EcoPlus Bi-folding Door  ");
  delay(300);
  send_command(select_SD_card, 5);
  send_command(set_Max_volume, 5);
  
  resetMotor();
  pixels.begin();
  
  gap=MOTOR_CLOSE_POS-MOTOR_OPEN_POS;
  
  setupMCP23008();
  MCP_DATA=0x00;
  setMCP(MCP_DATA);
  alert_status=0;
  stage=ROOM_FREE;
  onEMLock(0);
  onSiren(0);
  CloseDoor(1);
  showLEDColor(COLOR_GREEN);
       
  
}

void sendWIFI()
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

void playMP3(byte index)
{
  playMP3_command(index,8);
}

void playMP3_command(int songIndex, int folderIndex)
{
  //{0x7e, 0x04, 0x41, 0x00, 0x01, 0xef}; 
  play_cmd[3]=folderIndex;
  play_cmd[4]=songIndex;
  int len = 6;
 // Serial.println("\nMP3 Command => ");
  for(int i=0;i<len;i++){ mp3Serial.write(play_cmd[i]); 
  //Serial.println(play_cmd[i], HEX);
  }
  delay(100);

}
void send_command(int8_t command[], int len)
{
  //Serial.println("\nMP3 Command => ");
  for(int i=0;i<len;i++){ mp3Serial.write(command[i]); 
 // Serial.println(command[i], HEX); 
  }
  delay(100);
}


void showLEDColor(byte color)
{
  for(int i=0;i<NUMPIXELS;i++){

    // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
    if(color==COLOR_RED)
    {
      pixels.setPixelColor(i, pixels.Color(250,0,0)); // Moderately bright Red color.
    }else if (color==COLOR_ORANGE)
    {
      pixels.setPixelColor(i, pixels.Color(230,100,0)); // Moderately bright Orange color.
    }else if(color==COLOR_GREEN)
    {
      pixels.setPixelColor(i, pixels.Color(0,250,0)); // Moderately bright green color.
    }
    pixels.show(); // This sends the updated pixel color to the hardware.
   // delay(delayval); // Delay for a period of time (in milliseconds).

  }
}

void OpenDoor(byte mode) //0 - normal, 1- obstacle detected
{
  resetMotor();
  float diff;
  int motorSpeed;
   float speed_kp=0;
   int maxSpeed=210;
   int minSpeed=150;
   int maxProcessCounter=50;
   motorMoveCounter=0;
   processCounter=0;
     

  showLEDColor(COLOR_ORANGE);
  if( mode==0)
    playMP3(MP3_DOOR_OPENING);
  else if (mode==2)
    playMP3(MP3_DOOR_OPEN_TAKE_BELONGS);
  boolean timer=false;
  while(1)
  {
    motorMoveCounter++;
     Serial.println("---------Door Opening---------");

    motorPosition=analogRead(MOTOR_POS_PIN);

    motorADCCurrent=analogRead(MOTOR_CURRENT_PIN);
    Serial.print("Current: ");  Serial.println(motorADCCurrent);
   
    if(motorPosition>MOTOR_OPEN_POS)
    {
      if(motorPosition<MOTOR_SEMIOPEN_POS && timer == false)
      { 
        unsigned long elapsedTime = millis() - startTime;  // Calculate the elapsed time
        timer = true;
        Serial.println("time to open door :"+elapsedTime);
      }
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

            Serial.println(".....Closing Soft Closing started.. 2. ");
           // ReverseMotor(100,100,HIGH); //reverse from current one
            //delay(200);
            //digitalWrite(MOTOR_PHASE,LOW);
            
            motorSpeed=70;
            analogWrite(MOTOR_PWM,motorSpeed);
            delay(400);
            motorSpeed=0;
            Serial.println("Opening Reached... ");
            //analogWrite(MOTOR_PWM,motorSpeed);
            break;     
    }

    if(processCounter>maxProcessCounter  && stuck_status==0)
      {
        Serial.print("Opening Process Counter: ");  Serial.println(processCounter);
        Serial.print("Possible Stuck Detected!! Alerts");
        motorSpeed=0;
        beep(5);
        stuck_status=1;       
        CloseDoor(1);
		stuck_status=0;
        processCounter=0;
      }
      
     if(motorADCCurrent>=MOTOR_JAM_CURRENT_OPEN  && stuck_status==0)
     {
        Serial.print("Opening Door Jam Detected ");
        stuck_status=1;
        beep(5);
        CloseDoor(1);
        stuck_status=0;
        delay(5000);
     }   

      Serial.print("Motor_Speed: ");  Serial.println(motorSpeed);
      Serial.print("Moving Counter: ");  Serial.println(motorMoveCounter);
      analogWrite(MOTOR_PWM,motorSpeed);
      digitalWrite(MOTOR_PHASE,LOW);
      digitalWrite(MOTOR_PWR_CTRL,HIGH);
      delay(20);
    
  }
  //onEMLock(0);
  //delay(1000);
   Serial.print("Process Counter: ");  Serial.println(processCounter);
  beep(1);
  resetMotor();
}

void CloseDoor(byte mode)
{
  resetMotor();
    float speed_kp=0;
  int maxSpeed=190;
  int minSpeed=170;
  
  send_command(set_Max_volume, 5);
  float diff;
  int motorSpeed;
  float m_Speed;
   motorMoveCounter=0;
 // int gap=MOTOR_OPEN_POS-MOTOR_CLOSE_POS;
  showLEDColor(COLOR_ORANGE);
  if(mode==0)
    playMP3(MP3_DOOR_CLOSING);
  else if (mode==2)
    playMP3(MP3_DOOR_CLOSE_TAP_LOCK);

    int maxProcessCounter=50;
  processCounter=0;
  
  while(1)
  {
     motorMoveCounter++;
     processCounter++;
    Serial.println("---------Door Closing---------");
    
    motorPosition=analogRead(MOTOR_POS_PIN);
    Serial.print("Door Position: ");  Serial.println(motorPosition);
   
    motorADCCurrent=analogRead(MOTOR_CURRENT_PIN);
    Serial.print("Current: ");  Serial.println(motorADCCurrent);

    if(motorPosition<MOTOR_CLOSE_POS)
    {

      diff=MOTOR_CLOSE_POS-motorPosition;
      Serial.print("Door Diff: ");  Serial.println(diff);
      Serial.print("Door gap: ");  Serial.println(gap);
      speed_kp=(diff/gap)*maxSpeed;
      
      Serial.print("Speed: ");  Serial.println(speed_kp);    

      if(speed_kp<minSpeed) speed_kp=minSpeed;
        
      if(speed_kp>maxSpeed) speed_kp=maxSpeed;

      if(diff<15) speed_kp=120;
      Serial.print("Final Speed: ");  Serial.println(speed_kp);    
      motorSpeed=speed_kp;
      analogWrite(MOTOR_PWM,motorSpeed);
     
    }else{
            Serial.println(".....Closing Soft Closing started.. 2. ");
            //ReverseMotor(100,100,LOW); //reverse from current one
           // delay(200);
            //digitalWrite(MOTOR_PHASE,HIGH);
            motorSpeed=100;
            Serial.println(".....Closing Reached... ");
            analogWrite(MOTOR_PWM,motorSpeed);
            delay(200);
            motorSpeed=0;
            Serial.println(".....Closing Reached... ");
            analogWrite(MOTOR_PWM,motorSpeed);
            break;    
    }

      if(processCounter>maxProcessCounter && stuck_status==0)
      {
        Serial.print("Process Counter: ");  Serial.println(processCounter);
        Serial.print("Possible Stuck Detected!! Alerts");
        stuck_status=1;
		motorSpeed=0;
        beep(5);
		processCounter=0;
        OpenDoor(1);
       stuck_status=0;
        delay(5000);
        
      }

    
   if(motorADCCurrent>=MOTOR_JAM_CURRENT_CLOSE && motorMoveCounter>5 && stuck_status==0)
   {
     stuck_status=1;
      Serial.print("Cosing Door Jam Detected ");
     // motorSpeed=0;
      //analogWrite(MOTOR_PWM,motorSpeed);
      //digitalWrite(MOTOR_PWR_CTRL,LOW);
      beep(5);
      OpenDoor(1);
       stuck_status=0;
      delay(5000);
    
   }
      
      Serial.print("Motor_Speed: ");  Serial.println(motorSpeed);
      analogWrite(MOTOR_PWM,motorSpeed);
       digitalWrite(MOTOR_PHASE,HIGH);
      digitalWrite(MOTOR_PWR_CTRL,HIGH);
      delay(20);
    
  }
  Serial.print("Process Counter: ");  Serial.println(processCounter);
  beep(1);
  resetMotor();
}


void ReverseMotor(byte m_speed, int t, bool s) //t=time,  s=status HIGH, LOW
{
  resetMotor();
   analogWrite(MOTOR_PWM, m_speed);
   digitalWrite(MOTOR_PHASE,s);
   delay(t);

  resetMotor();
  
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



void onSiren(byte flag)
{
  if(flag==1)
  { bitSet(MCP_DATA, 7);
  }else{    
    bitClear(MCP_DATA, 7);
  }
  setMCP(MCP_DATA);
}

void onLedRed(byte flag)
{
  if(flag==1)
  { bitSet(MCP_DATA, 6);
  }else{    
    bitClear(MCP_DATA, 6);
  }
  setMCP(MCP_DATA);
}

void onEMLock(byte flag)
{
  if(flag==1)
  { 
    digitalWrite(EM_LOCK_PIN,HIGH);
  }else{    
    digitalWrite(EM_LOCK_PIN,LOW);
  }
 
}

void loop() {
  
  
  Entrance_IR = analogRead(IR_EXTERNAL); // Read the state of the switch
  Exit_IR = analogRead(IR_INTERNAL); // Read the state of the switch
  motorPosition=analogRead(MOTOR_POS_PIN);
  maintenanceStatus = analogRead(MAINTENANCE_PIN);
  Serial.println("--------RAW-------------");
  Serial.print("External IR: ");  Serial.println(Entrance_IR);
  Serial.print("Internal IR: ");  Serial.println(Exit_IR);
  Serial.print("Maintenance: ");  Serial.println(maintenanceStatus);
    
  if(Entrance_IR<1000) Entrance_IR=0; else Entrance_IR=1;
  if(Exit_IR<1000) Exit_IR=0; else Exit_IR=1;
  if(maintenanceStatus<1000) maintenanceStatus=0; else maintenanceStatus=1;
  Serial.println("--------------------------");
  Serial.print("Door Pos: ");  Serial.println(motorPosition);
  Serial.print("EntranceIR: ");  Serial.println(Entrance_IR);
  Serial.print("ExitIR: ");  Serial.println(Exit_IR);
  Serial.print("Maintenance: ");  Serial.println(maintenanceStatus);
   Serial.print(F("Temperature = "));
   temperature = 23;
  Serial.print(temperature);
  Serial.println(" *C");
  if(stage!=ROOM_MAINTENANCE)
  {
  if(Entrance_IR==1 || Exit_IR==1)
   {
    onSiren(1); delay(20);
    onSiren(0);
    startTime = millis();
   }
  }

  Serial.print("Stage: ");  Serial.println(stage);
  if(maintenanceStatus==1 &&  stage!=ROOM_MAINTENANCE)//&&  stage==ROOM_FREE)
  {
    Serial.println("-----Maintenance Mode---Active------");
    maintain_status=1;
    beep(2);
    playMP3(MP3_MAINTAINENCE);
    showLEDColor(COLOR_ORANGE);
    stage=ROOM_MAINTENANCE;
    onEMLock(0);
    //onEMLock(1);  delay(2000);
    OpenDoor(0);
    Serial.println("Door Opened - Maintenance");
    open_status=1;
    sendWIFI();
    resetMotor();
   // onEMLock(0); 
    delay(5000);
    
  }else if(maintenanceStatus==0 &&  stage==ROOM_MAINTENANCE)
  {
    stage=ROOM_FREE;
    Serial.println("-----Maintenance Mode----Disabled-----");
    maintain_status=0;
     beep(2);
   // onEMLock(0);  delay(100);
    
    onEMLock(0);
    CloseDoor(0);
    open_status=0;
    sendWIFI();
    showLEDColor(COLOR_GREEN);
    Serial.println("Door Closed - Maintenance");
    delay(5000);
  }else{
    maintain_status=0;
    process2();  
  }

  delay(50);

}

void beep(byte cnt)
{
  byte i;
  for(i=0; i<cnt;i++)
  {
    onSiren(1);
    delay(40);
    onSiren(0);
    delay(40);    
  }
  
}

void process2()
{
  if(Entrance_IR==HIGH && Exit_IR==LOW &&  stage==ROOM_FREE)
  {
    Serial.println("Free Mode. Disable and Open Door");
    beep(2);
   // onEMLock(1);  delay(100);
    onEMLock(0);
    OpenDoor(0);
    lock_status=0;
    open_status=1;
    sendWIFI();
    Serial.println("Door Opened - Free");
    resetMotor();
    delay(100);
    beep(2);
  //  onEMLock(0);  delay(100);
    CloseDoor(2);//Play Door Closing, Tap to lock door.
    open_status=0;
    sendWIFI();
    showLEDColor(COLOR_GREEN);
    Serial.println("Door Closed - Free");
    delay(100);
  }
  if(Entrance_IR==LOW && Exit_IR==HIGH &&  stage==ROOM_OCCUPIED)
   {
    Serial.println("Occupy Mode. Disable and Open Door");
    beep(2);
   // onEMLock(1);  delay(100);  
    onEMLock(0);
    lock_status=0;
    open_status=1;
    sendWIFI();
    OpenDoor(2);//Play Door Opening, Take your belonging.
   // delay(1000);
    Serial.println("Door Opened - Occupied");
    //playMP3(MP3_DOOR_SEE_U_AGAIN);
    resetMotor();
    delay(100);
    beep(2);
   // onEMLock(0);  delay(100);
    CloseDoor(0);
    lock_status=0;
    open_status=0;
    sendWIFI();
    //delay(1000);
    showLEDColor(COLOR_GREEN);
    stage=ROOM_FREE;
    onLedRed(0);
   }else if(Entrance_IR==LOW && Exit_IR==HIGH &&  stage==ROOM_FREE)
   {

    beep(2);
    Serial.println("Making sure door is closed ");
    
     showLEDColor(COLOR_RED);
    playMP3(MP3_DOOR_LOCKED);
    CloseDoor(1);
    showLEDColor(COLOR_RED);
    stage=ROOM_OCCUPIED;// delay(100);
    
    Serial.println("Enable Occupy Mode");
    onLedRed(1);
    onEMLock(1);
    lock_status=1;
    open_status=0;
    sendWIFI();

    //delay(100);
   }

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

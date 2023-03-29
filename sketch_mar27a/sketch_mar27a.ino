#define IR_EXTERNAL A7

int Entrance_IR;
void setup()
{
  Serial.begin(9600);
 pinMode(IR_EXTERNAL, INPUT);
}
void loop()
{


 Entrance_IR = analogRead(IR_EXTERNAL);
   if(Entrance_IR<900) Entrance_IR=0; else Entrance_IR=1;
  
  if (Entrance_IR == 0) {
      Serial.println("Button is pressed");
  }
  else {
      Serial.println("Button is not pressed");
  }
  delay(100);
}

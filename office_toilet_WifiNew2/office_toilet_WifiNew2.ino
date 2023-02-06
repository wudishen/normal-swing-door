
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

unsigned int raw=0;
float volt=0.0;
const int sleepTimeS = 60;

const char* ssid     = "cnc";
const char* password = "91091230";

const char* host = "ecoplus-mfg.com";

//https://www.ecoplus-mfg.com/test/main.php

const int httpPort = 80;

byte doorID=7;
byte locID=2;
byte open_status=1;
byte lock_status=0;
byte temperature=23;
byte maintenance=0;
byte alerts=0;
byte inByte[10];
int count;
int tryCnt;
unsigned long previousMillis = 0;
unsigned long interval = 30000;  


#define LED 13 //D7

void setup() {

  
  Serial.begin(9600);
 
  blinkLED(5);
  inByte[0]=0;
  inByte[1]=0;
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    tryCnt++;
    Serial.print(".");
    if(tryCnt>20)
    {
      tryCnt=0;
      Serial.println(".");
    }
  }
  //  Connect to WiFi network
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  
  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("RRSI: ");
  Serial.println(WiFi.RSSI());
  count=0;

}



void sendWIFI()
{
   if(WiFi.status() != WL_CONNECTED) {
       reconnectWIFI();
   }
      HTTPClient http;
      String url = "/insert.php?token=6FFA1F&temp=";
      url += temperature;
      url += "&loc=";// loc ID
      url += String(locID);
      url += "&dev=";// door ID
      url += String(doorID);
      url += "&open=";// open
      url += String(open_status);
       url += "&lock=";// lock
      url += String(lock_status);
      url += "\r\n";
      // Serial.print("Requesting URL: ");
       url=host+url;
     // Serial.println(url);

      //String link2="http://ecoplusiot.com.sg/insert.php?token=6FFA1F&temp="+String(temperature)+"&loc=1&dev=2&open=1&lock=0";
      //String link2="http://ecoplusiot.com.sg/insert.php?token=6FFA1F&temp="+String(temperature)+"&loc="+String(locID)+"&dev="+String(doorID)+"&open="+String(open_status)+"&lock="+String(lock_status)+"&alert="+String(alerts)+"&maintain="+String(maintenance);
       String link2="https://www.ecoplus-mfg.com/test/insert.php?token=6FFA1F&temp="+String(open_status);
      http.begin(link2);
      int httpCode = http.GET();     
        if (httpCode > 0) { //Check the returning code
            String payload = http.getString();   //Get the request response payload
          //  Serial.println(payload);             //Print the response payload
          }
 
       http.end();   //Close connection
    
    //  Serial.println();
     // Serial.println("closing connection");
      
     
   
  
}

void blinkLED(byte cnt)
{
  byte i; 
  /*
  for(i=0; i<cnt; i++)
  {
    digitalWrite(LED,HIGH);
    delay(50);
    digitalWrite(LED,LOW);
    delay(50);
  }
  */
}

void loop() {

//  init2();
  blinkLED(3);

  if (Serial.available()>7)
  {
    inByte[0]=Serial.read();
    inByte[1]=Serial.read();//locID
    inByte[2]=Serial.read();//devID
    inByte[3]=Serial.read();//open  0-open, 1-lock
    inByte[4]=Serial.read();//lock  1-lock, 0-no lock
    inByte[5]=Serial.read();//Temp
    inByte[6]=Serial.read();//maintenance
    inByte[7]=Serial.read();//alerts
    inByte[8]=Serial.read();

    if(inByte[0]=='a')
    {
      /*
      Serial.print("Rec: ");
      Serial.print(inByte[1],HEX); Serial.print(", ");
      Serial.print(inByte[2],HEX);Serial.print(", ");
      Serial.print(inByte[3],HEX);Serial.print(", ");
      Serial.print(inByte[4],HEX);Serial.print(", ");
      Serial.println(inByte[5],HEX);
      */
      locID=inByte[1];
      doorID=inByte[2];
      open_status=inByte[3];
      lock_status=inByte[4];
      temperature=inByte[5];
      maintenance=inByte[6];
      alerts=inByte[7];
      sendWIFI();
    }

     while (Serial.available()>0)
      inByte[9]=Serial.read();

    inByte[0]=0;
    inByte[1]=0;
  }

  checkWIFI();
   
  
  delay(200);
  
}

void checkWIFI()
{
   unsigned long currentMillis = millis();
  // if WiFi is down, try reconnecting every CHECK_WIFI_TIME seconds
  if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >=interval)) {
    Serial.print(millis());
    Serial.println("Reconnecting to WiFi...");
    reconnectWIFI();
    previousMillis = currentMillis;
  }
}

void reconnectWIFI()
{
   WiFi.disconnect();
    //WiFi.reconnect();
    WiFi.begin(ssid, password);
  
    while (WiFi.status() != WL_CONNECTED) {
      delay(100);
      tryCnt++;
      Serial.print(".");
      if(tryCnt>20)
      {
        tryCnt=0;
        Serial.println(".");
      }
    }
    
}

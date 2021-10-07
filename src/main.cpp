#include <Arduino.h>

#define MY_SPLASH_SCREEN_DISABLED

//#define MY_DEBUG
#define MY_RADIO_RF24
#define MY_REPEATER_FEATURE
#define MY_RX_MESSAGE_BUFFER_FEATURE
#define MY_RX_MESSAGE_BUFFER_SIZE (10)
#define MY_RF24_IRQ_PIN 2

#define MY_NODE_ID 3
#define CHILD_ID 1              // Id of the sensor child

#define SKETCH_NAME "Energy Meter SCT013"
#define SKETCH_VERSION "1.0"

//#include <SPI.h>
#include <MySensors.h>  
#include <EmonLib.h> 
 
#define ANALOG_INPUT_SENSOR 1
EnergyMonitor emon1;

MyMessage wattMsg(CHILD_ID,V_WATT);
MyMessage kwhMsg(CHILD_ID,V_KWH);
MyMessage msgKWH(CHILD_ID,V_VAR1);
unsigned long SLEEP_TIME = 60000 - 3735; // sleep for 60 seconds (-4 seconds to calculate values)

float wattsumme = 0;
float kwh = 0;
float wh = 0;
int minuten = 0;  //vorher 61
boolean KWH_received=false;

void setup() 
{  
  Serial.begin(115200);
  emon1.current(ANALOG_INPUT_SENSOR, 60.606);             // Current: input pin, calibration.

  double Irms = emon1.calcIrms(1480);  // initial boot to charge up capacitor (no reading is taken) - testing
  request(CHILD_ID,V_VAR1);
  //end of energy clamp code
}

void presentation() 
{
  sendSketchInfo(SKETCH_NAME, SKETCH_VERSION);
  present(CHILD_ID, S_POWER);
}

void loop()     
{ 
  //KWH reveived check
  if (!KWH_received) request(CHILD_ID,V_VAR1);
  
  // power used each minute
  if (minuten < 60) {
    double Irms = emon1.calcIrms(1480);  // Calculate Irms only
    if (Irms < 0.3) Irms = 0;
    long watt = Irms*240.0; // default was 230 but our local voltage is about 240
    wattsumme = wattsumme+watt;
    minuten++;
    send(wattMsg.set(watt, 3));  // Send watt value to gw
    
    Serial.print(watt);         // Apparent power
    Serial.print("W I= ");
    Serial.println(Irms);          // Irms   
  }
  // end power used each minute
  
  // hours KW reading
  if (minuten >= 60) {
    wh = wh + wattsumme/60;
    kwh = wh/1000;
    send(kwhMsg.set(kwh, 3)); // Send kwh value to gw 
    send(msgKWH.set(kwh, 3)); // Send kwh value to gw
    wattsumme = 0;
    minuten = 0;
  }

 wait(SLEEP_TIME);
}

void receive(const MyMessage &message) {
  if (message.type==V_VAR1) {  
    kwh = message.getFloat();
    wh = kwh*1000;
    Serial.print("Received last KWH from gw:");
    Serial.println(kwh);
    //send(kwhMsg.set(kwh, 3)); // Send kwh value to gw 
    KWH_received = true;
  }
}

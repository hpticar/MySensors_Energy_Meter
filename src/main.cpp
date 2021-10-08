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

#include <MySensors.h>
#include <EmonLib.h>
#include <LibPrintf.h>
 
#define ANALOG_INPUT_SENSOR 1
EnergyMonitor emon1;

MyMessage wattMsg(CHILD_ID,V_WATT);
MyMessage kwhMsg(CHILD_ID,V_KWH);

unsigned long SLEEP_TIME = 0; // sleep time until next second tick
unsigned long time = 0;
unsigned long currentTime = 0;

long watt = 0;
long wattAverage = 0;
double wattTotal = 0;
double kwh = 0;
double wh = 0;
int seconds = 0;
boolean KWH_received=false;

void setup() 
{  
  Serial.begin(115200);
  printf("Start Energy Meter SCT013 v1.0\n");
  // Calibration factor = CT TURNS / burden resistance = 2000 / 33 Ohms = 60.606
  // - or -
  // Calibration factor = CT ratio / burden resistance = (100A / 0.05A) / 33 Ohms = 60.606
  emon1.current(ANALOG_INPUT_SENSOR, 60.606);

  double Irms = emon1.calcIrms(1480);  // initial boot to charge up capacitor (no reading is taken) - testing
  request(CHILD_ID,V_KWH);
  wait(1000);
  time = millis();
}

void presentation() 
{
  sendSketchInfo(SKETCH_NAME, SKETCH_VERSION);
  present(CHILD_ID, S_POWER);
}

void loop()     
{ 
  if (!KWH_received) request(CHILD_ID,V_KWH);

  double Irms = emon1.calcIrms(1480);  // Calculate Irms only
  if (Irms < 0.3) Irms = 0;
  watt = Irms*240.0; // default was 230 but our local voltage is about 240
  wattTotal += watt;
  seconds++;
  
  if (seconds % 5 == 0) { // Send watt value every 5 sec
    //send(wattMsg.set(watt, 0));  
    send(wattMsg.set(wattTotal/seconds, 0)); // send average wattage instead of momentary, makes for a smoother curve
    printf("SND:W=%.0f\n", wattTotal/seconds);
  }
  
  if (seconds == 60){ // Send kWh every 60 seconds
    wh += wattTotal/3600;
    kwh = wh/1000;
    send(kwhMsg.set(kwh, 3));
    printf("SND:KWH=%.3f\n", kwh);
    wattTotal = 0;
    seconds = 0;
  }

  printf("%.3f\tW=%ld\tI=%.2f\tT=%lu\n", (double)time/1000, watt, Irms, SLEEP_TIME);

  currentTime = millis();

  if (currentTime > time) {
    SLEEP_TIME = 1000 - (currentTime - time);
  }
  else { // when it loops over 4,294,967,295
    SLEEP_TIME = 1000 - (4294967295 - time) + currentTime;
  }
  wait(SLEEP_TIME);
  time = millis();
}

void receive(const MyMessage &message) {
  if (message.type==V_KWH) {  
    kwh = message.getFloat();
    wh = kwh*1000;
    printf("RCV:KWH=%.3f\n", kwh);
    KWH_received = true;
  }
}

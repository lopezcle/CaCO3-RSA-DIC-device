//Source code for "Development of a new low-cost device to measure calcium carbonate content, reactive surface area in solid samples and dissolved inorganic carbon content in water samples"
//v1.0 with analog LCD screen 
//By Clément Lopez C.

#include <LiquidCrystal.h>
#include "Seeed_BME280.h"
#include <SD.h>
#include <Wire.h>
#include "RTClib.h"
#include <SoftwareSerial.h>

LiquidCrystal lcd(9, 8, 5, 4, 3, 2); 
SoftwareSerial mySerial(6, 7); // RX, TX
unsigned char hexdata[9] = {0xFF,0x01,0x86,0x00,0x00,0x00,0x00,0x00,0x79}; //Read the gas density command /Don't change the order

#define LOG_INTERVAL  5000 // Logging interval in ms

#define SYNC_INTERVAL 30000 // Interval to write data to the SD card in ms
uint32_t syncTime = 0; // Time of last sync()

#define ECHO_TO_SERIAL   1 // echo data to serial port
#define WAIT_TO_START    0 // Wait for serial input in setup()

BME280 bme280;

RTC_DS1307 RTC; // Define the Real Time Clock object

// For the data logging shield, use digital pin 10 for the SD cs line
const int chipSelect = 10;

// The logging file
File logfile;

void error(char *str)
{
  Serial.print("error: ");
  Serial.println(str);
  while(1);
}

void setup(void)
{
  lcd.begin(16, 2);
  pinMode(A0, OUTPUT);
  digitalWrite(A0, HIGH); //Relay position by default. The pump is off. 
  Serial.begin(9600);
  while (!Serial) {

  }
  mySerial.begin(9600);
  Serial.println();

 #if WAIT_TO_START
  Serial.println("Type any character to start");
  while (!Serial.available());
#endif //WAIT_TO_START

  // initialize the SD card
  Serial.print("Initializing SD card...");
  // Make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(10, OUTPUT);
  
  // See if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    error("Card failed, or not present");
  }
  Serial.println("card initialized.");
  
  // Create a new file
  char filename[] = "LOGGER00.CSV";
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = i/10 + '0';
    filename[7] = i%10 + '0';
    if (! SD.exists(filename)) {
      // Only open a new file if it doesn't exist
      logfile = SD.open(filename, FILE_WRITE); 
      break;  // Leave the loop
    }
  }

  Serial.print("Logging to: ");
  Serial.println(filename);

  // Connect to RTC
  Wire.begin();  
  if (!RTC.begin()) {
    logfile.println("RTC failed");
#if ECHO_TO_SERIAL
    Serial.println("RTC failed");
#endif  //ECHO_TO_SERIAL
  }
  
  logfile.println("millis,CO2,temp_bme,pressure_bme,RH");    
#if ECHO_TO_SERIAL
  Serial.println("Millis\tCO2\tTemp_BME\tPressure_BME\tRH");
#endif //ECHO_TO_SERIAL
 
}

void loop(void)
{
  digitalWrite(A0, LOW); //Activates the pump.
  DateTime now;

  // Delay for the amount of time we want between readings
  delay((LOG_INTERVAL -1) - (millis() % LOG_INTERVAL));
   
  // Log milliseconds since starting
  uint32_t m = millis();
  logfile.print(m);           // Milliseconds since start
  logfile.print(",");    
#if ECHO_TO_SERIAL
  Serial.print(m);         // Milliseconds since start
  Serial.print("\t");  
#endif

  now = RTC.now();

mySerial.write(hexdata,9);
long hi,lo,co2;

 for(int i=0,j=0;i<9;i++)
 {
  if (mySerial.available()>0)
  {
     
     int ch=mySerial.read();

    if(i==2){     hi=ch;   }   //High concentration
    if(i==3){     lo=ch;   }   //Low concentration
    if(i==8) { 
              co2=hi*256+lo;  //CO2 concentration
              logfile.print(co2);
              Serial.print(co2);
             }

  }   
 } 
   
   float temp_bme = bme280.getTemperature();
   int temp_bme_2 = temp_bme;
   delay(10);
   float pressure_bme = bme280.getPressure();
   delay(10);
   float RH = bme280.getHumidity();
   delay(10);

//Logging measurements
  logfile.print(", "); logfile.print(temp_bme);
  logfile.print(", "); logfile.print(pressure_bme);
  logfile.print(", "); logfile.println(RH);

//Display measurements on LCD screen
  lcd.setCursor(0,0); lcd.print("CO2 "); lcd.setCursor(4,0);lcd.print(co2);
  lcd.setCursor(11,0); lcd.print("RH ");lcd.print(RH);
  lcd.setCursor(0,1); lcd.print("T "); lcd.setCursor(2,1);lcd.print(temp_bme_2);
  lcd.setCursor(6,1); lcd.print("P ");lcd.print(pressure_bme);

  delay(1800);
  lcd.clear();

//Display measurements on Arduino serial monitor 
#if ECHO_TO_SERIAL
  Serial.print("\t"); 
  Serial.print(temp_bme);
  Serial.print("\t"); 
  Serial.print(pressure_bme);
  Serial.print("\t"); 
  Serial.println(RH);
  
#endif //ECHO_TO_SERIAL

#if ECHO_TO_SERIAL
  Serial.println();
#endif // ECHO_TO_SERIAL

  if ((millis() - syncTime) < SYNC_INTERVAL) return;
  syncTime = millis();
  logfile.flush(); 
}

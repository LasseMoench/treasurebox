#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>

TinyGPSPlus gps;  // The TinyGPS++ object
SoftwareSerial ss(D7, D6); // The serial connection to the GPS device

// Construct an LCD object and pass it the 
// I2C address, width (in characters) and
// height (in characters). Depending on the
// Actual device, the IC2 address may change.
LiquidCrystal_I2C lcd(0x3F, 20, 4);
int counterAddr = 0;
int switchPin = D5;
int relayPin = 3; //RX pin hopefully doesn't go high on boot
float distance = 9999.9;
bool fixTimeOut = false;


const double TARGET_LAT = 50.76371;
const double TARGET_LNG = 6.01032;

const int TRIES = 20;

// This custom version of delay() ensures that the gps object is being "fed".
static void smartDelay(unsigned long ms){unsigned long start = millis(); do  {while (ss.available()) gps.encode(ss.read());} while (millis() - start < ms);}


void setup() {
  //Shutdown WiFi to save power
  WiFi.mode( WIFI_OFF );
  WiFi.forceSleepBegin();
  delay( 1 );
  
  digitalWrite(switchPin, LOW);
  digitalWrite(relayPin, LOW);
  pinMode(switchPin, OUTPUT);
  pinMode(relayPin, OUTPUT);
  // The begin call takes the width and height. This
  // Should match the number provided to the constructor.
  lcd.begin(20, 4);
  lcd.init();

  ss.begin(9600);
  
  EEPROM.begin(512);

  int tries = EEPROM.read(counterAddr);

  // Turn on the backlight.
  lcd.backlight();

  //TODO: If tries > limit, show failure message
  if(tries > TRIES){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Tries exceeded.");
    lcd.setCursor(0, 1);
    lcd.print("Box locked forever.");
    lcd.setCursor(0, 3);
    lcd.print("Contact a pirate!");
  }else{
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Reading map...");
    lcd.setCursor(0, 3);
    lcd.print("Be patient. ");
  
    //Get position
    smartDelay(200);
    int ms = millis();
    while(gps.satellites.value() < 4 && (millis() - ms) < 120000){
      smartDelay(200);
      lcd.setCursor(17,3);
      lcd.print(gps.satellites.value());
      lcd.print("/4");
      delay(500);
      if(millis() - ms > 115000){
          fixTimeOut = true;
        }
      }

    //Enough GPS satellites but no valid fix yet
    ms = millis();
    while(!(gps.location.isValid()) && (millis() - ms) < 30000){
      smartDelay(100);
      delay(100);  
    }
  
    if(!(gps.location.isValid())){
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Can not orient.");
      lcd.setCursor(0, 1);
      lcd.print("Take me outside!");
      if(fixTimeOut){
          lcd.setCursor(0,3);
          lcd.print("Fix timeout.");
        }
      delay(5000);
      digitalWrite(switchPin, HIGH);
      }
    else{
      distance = gps.distanceBetween(gps.location.lat(), gps.location.lng(), TARGET_LAT, TARGET_LNG);
      }
  
    if(distance < 50.0){
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("You are one");
        lcd.setCursor(0,1);
        lcd.print("clever Pirate!");
        lcd.setCursor(0,3);        
        lcd.print("Box is open.");

        digitalWrite(relayPin, HIGH);
        delay(30000);
        digitalWrite(switchPin, HIGH);
      } else {
          String tryString = "/20 tries";

          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print(tries + tryString);
        
          lcd.setCursor(0,1);
          String distString = "Distance: ";
          String distStringEnd = "m";
          lcd.print(distString + distance + distStringEnd);

          if(tries > 9){
            double courseTo = gps.courseTo(gps.location.lat(), gps.location.lng(), TARGET_LAT, TARGET_LNG);  
            lcd.setCursor(0,2);
            lcd.print("Direction: ");
            lcd.print(courseTo);
            lcd.print(gps.cardinal(courseTo));
          }
          
          tries++;            
          EEPROM.write(counterAddr, tries);
          EEPROM.commit();

          delay(10000);
          digitalWrite(switchPin, HIGH);
        }
    }
    
  //Print emergency gibberish to to make people unplug usb before box opens
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("WARNING:");
  lcd.setCursor(0,1);
  lcd.print("Power too high!");
  lcd.setCursor(0,2);
  lcd.print("Remove USB");
  lcd.setCursor(0,3);
  lcd.print("connection!");

  delay(10000);

  lcd.clear();
  lcd.setCursor(0,3);

  for(int i = 0; i < 80; i++){
    byte randomValue = random(0, 37);
    char letter = randomValue + ' ';
    lcd.print(letter);
    delay(300);
  }

  delay(20000);

  EEPROM.write(counterAddr, 0);
  EEPROM.commit();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("RESET COUNTER");
  delay(1000);
  lcd.noBacklight();

  delay(10000);

  digitalWrite(relayPin, HIGH);
  delay(20000);
}

void loop() {
}

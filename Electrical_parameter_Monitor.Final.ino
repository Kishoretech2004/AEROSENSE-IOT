// ESP32_ENERGY_METER.bot -Bot Name
// https://www.infinitiautomation.com/project2024/1/home.php -Web Page

#include <WiFi.h>
#include <PZEM004Tv30.h>
#include <Wire.h>
#include <LiquidCrystal.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h> 
#include <HTTPClient.h>  

PZEM004Tv30 pzem(Serial2, 16, 17);
LiquidCrystal lcd(33, 25, 26, 27, 14, 13);

#define Fan 15   
#define Exhaustfan 2 
#define EmergencyLight 4    
#define relay4 5 
#define oneWireBus 21 
#define BUZZER 23 
#define gassensor 22 

#define BOTtoken "7706694725:AAGrkSbcg4CNnbER6_I6JIWTPYIKqIYgarM" 
#define CHAT_ID "8191531324"  

OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);
  
WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);   

int botRequestDelay = 1000; 
unsigned long lastTimeBotRan;

const char* URL = "http://www.infinitiautomation.com/project2024/1/save.php?id=1";
char ssid[] = "admin";  
char pass[] = "1234567890";    

float voltage, current, frequency, pf, power, temperatureC, gassenor; 
int sig;
int Fan_var;
int Exhaustfan_var;
int EmergencyLight_var;
bool faultActive = false; 

void setup() {
  Serial.begin(9600);
  lcd.begin(20,4);
  pinMode(Fan, OUTPUT);
  pinMode(Exhaustfan, OUTPUT);
  pinMode(EmergencyLight, OUTPUT);
  pinMode(relay4, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(gassensor, INPUT);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

#ifdef ESP32
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT); 
#endif

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  
  lcd.setCursor(0, 0); 
  lcd.print(" ENERGY_METER ");
  lcd.setCursor(1, 0); 
  lcd.print(" ENERGY_METER ");
  delay(1500);

  digitalWrite(BUZZER, HIGH); 
  delay(1500); 
  digitalWrite(BUZZER, LOW); 

  lcd.clear();
}

void loop() {
  // Regular parameter checks
  Checkparameter();
  Checktemperature();
  Checkgaslevel();
  send_data(); // Update data and receive the sig command

  if (!faultActive) {
    if (sig == 1) {
      digitalWrite(relay4, HIGH); // Turn ON relay4 per command
      Serial.println("Relay4 ON due to remote command");
    } else if (sig == 0) {
      digitalWrite(relay4, LOW); // Turn OFF relay4 per command
      Serial.println("Relay4 OFF due to remote command");
    }
  }


  if (voltage > 250 || voltage < 180 || current > 0.50) {
    // Unsafe condition detected: turn off relay4 and set faultActive
    digitalWrite(relay4, LOW); // Ensure relay turns off
    faultActive = true; // Set fault active to prevent relay from turning on

    Serial.println("Relay4 OFF due to unsafe voltage/current");
    lcd.clear();
    bot.sendMessage(CHAT_ID,  "*** ALERT !!! HIGH Payload Detected  ***  ", "");
    bot.sendMessage(CHAT_ID, "***      FAN ON     ***  ", "");
    Serial.println("Message sent");
    lcd.setCursor(0, 0);
    lcd.print("***** ALERT !!!*****");
    lcd.setCursor(0, 1);
    lcd.print(" Voltage/Current ");
    lcd.setCursor(0, 2);
    lcd.print(" Out of Range ");
    lcd.setCursor(0, 3);
    lcd.print("Relay 4 Turned OFF");

    // Emergency light on for 5 seconds
    digitalWrite(EmergencyLight, HIGH);
    delay(5000);
    digitalWrite(EmergencyLight, LOW);
  } else if (faultActive) {
    // Step 3: If conditions return to normal, reset faultActive
    faultActive = false;
    Serial.println("Safe conditions restored. Relay4 can respond to commands.");
  }

  delay(1500); // Small delay to avoid rapid relay switching
}

void Checkparameter() {
  voltage = pzem.voltage();
  if (!isnan(voltage)) {
    Serial.print("Voltage: ");
    Serial.print(voltage);
    Serial.println("V");
    lcd.setCursor(1, 0);
    lcd.print("V:");
    lcd.print(voltage);
  } else {
    Serial.println("Error reading voltage");
  }

  current = pzem.current();
  if (!isnan(current)) {
    Serial.print("Current: ");
    Serial.print(current);
    Serial.println("A");
    lcd.setCursor(1, 1);
    lcd.print("I:");
    lcd.print(current);
  } else {
    Serial.println("Error reading current");
  }

  power = pzem.power();  // Assign power to the global variable
  if (!isnan(power)) {
    Serial.print("Power: ");
    Serial.print(power);
    Serial.println("W");
    lcd.setCursor(11,0);
    lcd.print("P:");
    lcd.print(power);
  } else {
    Serial.println("Error reading power");
  }

  float energy = pzem.energy();
  if (!isnan(energy)) {
    Serial.print("Energy: ");
    Serial.print(energy, 3);
    Serial.println("kWh");
    lcd.setCursor(11, 1);
    lcd.print("E:");
    lcd.print(energy);
  } else {
    Serial.println("Error reading energy");
  }

  frequency = pzem.frequency();
  if (!isnan(frequency)) {
    Serial.print("Frequency: ");
    Serial.print(frequency, 1);
    Serial.println("Hz");
    lcd.setCursor(1,2);
    lcd.print("F:");
    lcd.print(frequency);
  } else {
    Serial.println("Error reading frequency");
  }

  pf = pzem.pf();
  if (!isnan(pf)) {
    Serial.print("PF: ");
    Serial.println(pf);
    lcd.setCursor(11,2);
    lcd.print("PF:");
    lcd.print(pf);
  } else {
    Serial.println("Error reading power factor");
  }
  delay(1000);
  lcd.clear();
}

void Checktemperature() {
  sensors.requestTemperatures();
  temperatureC = sensors.getTempCByIndex(0);

  Serial.print("Temperature: ");
  Serial.print(temperatureC);
  Serial.println(" °C");

  lcd.setCursor(2, 2);
  lcd.print("Temperature:");
  lcd.print(temperatureC);
  delay(1000);
  lcd.clear();

  if (temperatureC > 36) {
    Serial.println("ALERT, Temperature HIGH");
    Serial.println("FAN On");
    digitalWrite(Fan, HIGH); Fan_var = 1;
    digitalWrite(EmergencyLight, HIGH); delay(5000);
    digitalWrite(EmergencyLight, LOW); delay(250);
    bot.sendMessage(CHAT_ID, "*** ALERT!!! TEMPERATURE HIGH ***...", "");
    bot.sendMessage(CHAT_ID, "***      FAN ON     ***....", "");
    Serial.println("Message sent");

    lcd.setCursor(0, 0);
    lcd.print("***** ALERT !!!*****");
    lcd.setCursor(0, 1);
    lcd.print("* HIGH TEMPERATURE *");
    lcd.setCursor(0, 3);
    lcd.print("***    FAN ON    ***"); 
    delay(2000);
    lcd.clear();

  } else {
    digitalWrite(Fan, LOW); Fan_var = 0;
    digitalWrite(EmergencyLight, LOW);
  }
}

void Checkgaslevel() {
 int value = digitalRead(gassensor);
  if (value == 0) {
    Serial.println("ALERT, Gas Smell Detected ");
    Serial.println("Exhaust Fan On");
    digitalWrite(Exhaustfan, HIGH); Exhaustfan_var = 1;
    digitalWrite(EmergencyLight, HIGH); delay(5000);
    digitalWrite(EmergencyLight, LOW); delay(250);
    bot.sendMessage(CHAT_ID, "***  ALERT !!! GAS SMELL DETECTED  ***..", "");
    bot.sendMessage(CHAT_ID, "***      FAN ON     *** ..", "");
    Serial.println("Message sent");
    lcd.setCursor(0, 0);
    lcd.print("***** ALERT !!!*****");
    lcd.setCursor(0, 1);
    lcd.print(" Gas Smell Detected");
    lcd.setCursor(0, 3);
    lcd.print("** Exhaust Fan On **");
    delay(2000);
    lcd.clear();
  } else {
    digitalWrite(Exhaustfan, LOW); Exhaustfan_var = 0;
  }
}

void send_data() {
  lcd.clear();
  lcd.setCursor(0,0); lcd.print("  ...UPDATING..  ");
 
  if ((WiFi.status() == WL_CONNECTED)) { 
    Serial.print("UPDATING...");
    
    HTTPClient http;    

    String data1 = "&v1=";
    String data2 = "&v2=";
    String data3 = "&v3=";
    String data4 = "&v4=";
    String data5 = "&v5=";
    String data6 = "&v6=";
    String data7 = "&v7=";
    String data8 = "&v8=";
    String data9 = "&v9=";
    String data10 = "&v10=";

    http.begin(URL + data1 + voltage + data2 + current + data3 + power + data4 + frequency + data5 + pf + data6 + temperatureC + data7 + gassenor + data8 + Fan_var + data9 + Exhaustfan_var + data10 + EmergencyLight_var);
    Serial.println(URL + data1 + voltage + data2 + current + data3 + power + data4 + frequency + data5 + pf + data6 + temperatureC + data7 + gassenor + data8 + Fan_var + data9 + Exhaustfan_var + data10 + EmergencyLight_var);

    http.addHeader("Content-Type", "text/plain");  
    int httpCode = http.POST("Message from ESP8266");   
    String payload = http.getString();                  

    Serial.println(httpCode);   
    Serial.println(payload);    
    sig = payload.toInt(); // Convert the payload to an integer
    http.end(); 

    lcd.setCursor(0,1); lcd.print("    success!    "); delay(1500);
    lcd.clear();
  } else {
    Serial.println("Error in WiFi connection");
  }
}

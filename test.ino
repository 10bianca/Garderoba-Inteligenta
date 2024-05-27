#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Stepper.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>
#include <DHT.h> // Asigură-te că librăria este inclusă corect

#define DHTPIN A0      // Pinul la care este conectat senzorul DHT11
#define DHTTYPE DHT11 // Tipul senzorului (DHT11, DHT22, etc.)

DHT dht(DHTPIN, DHTTYPE); // Inițializează senzorul

// Replace with your network credentials
const char* ssid = "Nume internet";
const char* password = "Parola internet";

// Replace with your API key
const char* apiKey = "cdadacc4dcebbd54cd11e8bdbb787111";

// Initialize the LCD
LiquidCrystal_I2C lcd(0x27, 16, 2); // I2C interface on A4 (SDA) and A5 (SCL)
int motorSpeed = 15;  // RPM = Revolution per minute

int pin1 = 8;   // IN1 is connected to 8
int pin2 = 9;   // IN2 is connected to 9
int pin3 = 10;  // IN3 is connected to 10
int pin4 = 11;  // IN4 is connected to 11

int buttonAutumn = 4;  // pinul pentru buton de rotire CW
int buttonSummer = 5;
int buttonSpring = 2;
int buttonWinter = 3;

Stepper myStepper(2048, pin1, pin3, pin2, pin4);

int readingAutumn = 1;
int readingSummer = 1;
int readingSpring = 1;
int readingWinter = 1;

int stepRequired;

int currentSeason = 0;  // 0 -> primavara; 1 -> vara; 2 -> toamna; 3 -> iarna;

int opus = 1024;
int dreapta = 512;
int stanga = -512;

// Initialize SoftwareSerial for ESP8266
SoftwareSerial espSerial(0, 1); // RX, TX

void setup() {
  // Initialize Serial for debugging
  Serial.begin(115200);

  // Initialize SoftwareSerial for ESP8266
  espSerial.begin(115200);

  // Initialize the LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Starting...");
  delay(2000);
  lcd.clear();

  // Initialize buttons
  pinMode(buttonAutumn, INPUT_PULLUP);
  pinMode(buttonSummer, INPUT_PULLUP);
  pinMode(buttonSpring, INPUT_PULLUP);
  pinMode(buttonWinter, INPUT_PULLUP);

  // Set stepper motor speed
  myStepper.setSpeed(motorSpeed);

  // Initialize DHT sensor
  dht.begin();

  // Connect to Wi-Fi
  connectToWiFi();
}

void loop() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    lcd.setCursor(0, 0);
    lcd.print("Failed to read");
    lcd.setCursor(0, 1);
    lcd.print("from DHT sensor");
    return;
  }

  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(t*10);
  lcd.print((char) 223);
  lcd.print("C");
  

  lcd.setCursor(0, 1);
  lcd.print("Hum: ");
  lcd.print(h);
  lcd.print(" %");

  readingAutumn = digitalRead(buttonAutumn);
  readingSummer = digitalRead(buttonSummer);
  readingSpring = digitalRead(buttonSpring);
  readingWinter = digitalRead(buttonWinter);

  myStepper.setSpeed(motorSpeed);

  if (readingAutumn == 0) {
    switch (currentSeason) {
      case 0:
        myStepper.step(opus);
        break;

      case 1:
        myStepper.step(dreapta);
        break;

      case 2:
        myStepper.step(2048);
        break;

      case 3:
        myStepper.step(stanga);
        break;
    }
    currentSeason = 2;
    delay(100);
  }

  if (readingSummer == 0) {
    switch (currentSeason) {
      case 0:
        myStepper.step(dreapta);
        break;

      case 1:
        myStepper.step(2048);
        break;

      case 2:
        myStepper.step(stanga);
        break;

      case 3:
        myStepper.step(opus);
        break;
    }
    currentSeason = 1;
    delay(100);
  }

  if (readingSpring == 0) {
    switch (currentSeason) {
      case 0:
        myStepper.step(2048);
        break;

      case 1:
        myStepper.step(stanga);
        break;

      case 2:
        myStepper.step(opus);
        break;

      case 3:
        myStepper.step(dreapta);
        break;
    }
    currentSeason = 0;
    delay(100);
  }

  if (readingWinter == 0) {
    switch (currentSeason) {
      case 0:
        myStepper.step(stanga);
        break;

      case 1:
        myStepper.step(opus);
        break;

      case 2:
        myStepper.step(dreapta);
        break;

      case 3:
        myStepper.step(2048);
        break;
    }
    currentSeason = 3;
    delay(100);
  }

  // Fetch and display temperature data
  // fetchAndDisplayTemperature();

  // Delay to avoid too frequent API calls
  delay(100);
}

void connectToWiFi() {
  Serial.println("Sending AT");
  espSerial.println("AT");
  delay(1000);
  
  Serial.println("Setting CWMODE");
  espSerial.println("AT+CWMODE=1");
  delay(1000);
  
  Serial.println("Connecting to WiFi");
  espSerial.print("AT+CWJAP=\"");
  espSerial.print(ssid);
  espSerial.print("\",\"");
  espSerial.print(password);
  espSerial.println("\"");
  delay(1000);

  String response = "";
  while (espSerial.available()) {
    response += espSerial.readString();
  }
  Serial.println("WiFi Connection Response: " + response);

  if (response.indexOf("OK") != -1) {
    Serial.println("WiFi connected successfully.");
  } else {
    Serial.println("WiFi connection failed.");
  }
}

void fetchAndDisplayTemperature() {
  espSerial.println("AT+CIPSTART=\"TCP\",\"api.weatherstack.com\",80");
  delay(1000);

  String request = "GET /current?access_key=";
  request += apiKey;
  request += "&query=Bucharest HTTP/1.1\r\n";
  request += "Host: api.weatherstack.com\r\n";
  request += "Connection: close\r\n\r\n";

  espSerial.print("AT+CIPSEND=");
  espSerial.println(request.length());
  delay(1000);
  espSerial.print(request);

  String response = "";
  while (espSerial.available()) {
    response += espSerial.readString();
  }

  int jsonStart = response.indexOf('{');
  int jsonEnd = response.lastIndexOf('}') + 1;
  if (jsonStart != -1 && jsonEnd != -1) {
    String jsonResponse = response.substring(jsonStart, jsonEnd);
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, jsonResponse);
    float temperature = doc["current"]["temperature"];

    // Display temperature on LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Temp: ");
    lcd.print(temperature);
    lcd.print(" C");
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("No data");
  }
}

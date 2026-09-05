/*
 * ================================================================
 * IoT-BASED POULTRY HOUSE MONITORING SYSTEM
 * ================================================================
 * Institution : ULK Polytechnic Institute
 * Programme   : Bachelor of Technology - Electrical & Electronics
 * Location    : South Sudan
 * Year        : 2026
 * ----------------------------------------------------------------
 * Description : Real-time environmental monitoring system for
 *               poultry houses using ESP32, DHT22, MQ-135, LDR,
 *               16x2 LCD, SIM800L GSM, Google Firebase, and LEDs.
 *               Dual Wi-Fi and GSM communication for reliable
 *               operation in low-connectivity rural environments.
 * ================================================================
 */

#include <WiFi.h>
#include <FirebaseESP32.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// FILL IN YOUR CREDENTIALS HERE
#define WIFI_SSID         "YourWiFiName"
#define WIFI_PASSWORD     "YourWiFiPassword"
#define FIREBASE_HOST     "your-project-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH     "YourAPIKey"
#define FARMER_PHONE      "+211XXXXXXXXX"

// PIN DEFINITIONS
#define DHTPIN      4
#define DHTTYPE     DHT22
#define MQ135_PIN   34
#define LDR_PIN     35
#define GSM_RX      14
#define GSM_TX      27
#define LED_RED1    25
#define LED_RED2    26
#define LED_YELLOW  32
#define LED_GREEN   33
#define BUZZER      23

// THRESHOLD VALUES
#define TEMP_MAX     30.0
#define HUMID_MAX    75.0
#define HUMID_MIN    40.0
#define AMMONIA_MAX  25.0

// OBJECTS
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);
HardwareSerial gsmSerial(2);
FirebaseData firebaseData;
FirebaseConfig firebaseConfig;
FirebaseAuth firebaseAuth;

// GLOBAL VARIABLES
float temperature, humidity, ammoniaPPM;
int   lightPercent;
bool  buzzerSent = false;
unsigned long lastReadTime = 0;
const long READ_INTERVAL = 30000;

float getRawToPPM(int rawValue) {
  float voltage = rawValue * (3.3 / 4095.0);
  return voltage * 100.0;
}

void sendSMS(String message) {
  Serial.println("Sending SMS via GSM...");
  gsmSerial.println("AT+CMGF=1");
  delay(1000);
  gsmSerial.println("AT+CMGS=\"" + String(FARMER_PHONE) + "\"");
  delay(1000);
  gsmSerial.print(message);
  gsmSerial.write(26);
  delay(5000);
  Serial.println("SMS sent!");
}

void sendToFirebase(String status) {
  Firebase.setFloat(firebaseData,  "/sensors/temperature", temperature);
  Firebase.setFloat(firebaseData,  "/sensors/humidity",    humidity);
  Firebase.setFloat(firebaseData,  "/sensors/ammonia_ppm", ammoniaPPM);
  Firebase.setInt(firebaseData,    "/sensors/light",       lightPercent);
  Firebase.setString(firebaseData, "/sensors/status",      status);
  Serial.println("Firebase updated. Status: " + status);
}

void updateLCD(String sysStatus) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(temperature, 1);
  lcd.print("C ");
  lcd.print("H:");
  lcd.print((int)humidity);
  lcd.print("%");
  lcd.setCursor(0, 1);
  lcd.print("NH3:");
  lcd.print(ammoniaPPM, 1);
  lcd.setCursor(10, 1);
  lcd.print(sysStatus);
}

String checkThresholds() {
  bool tempAlert     = (temperature > TEMP_MAX);
  bool ammoniaAlert  = (ammoniaPPM  > AMMONIA_MAX);
  bool humidityAlert = (humidity    > HUMID_MAX || humidity < HUMID_MIN);
  bool allNormal     = (!tempAlert && !ammoniaAlert && !humidityAlert);

  digitalWrite(LED_RED1,   tempAlert     ? HIGH : LOW);
  digitalWrite(LED_RED2,   ammoniaAlert  ? HIGH : LOW);
  digitalWrite(LED_YELLOW, humidityAlert ? HIGH : LOW);
  digitalWrite(LED_GREEN,  allNormal     ? HIGH : LOW);

  if ((tempAlert || ammoniaAlert) && !buzzerSent) {
    digitalWrite(BUZZER, HIGH);
    delay(2000);
    digitalWrite(BUZZER, LOW);
    buzzerSent = true;
  }
  if (!tempAlert && !ammoniaAlert) { buzzerSent = false; }

  String alertMsg = "";
  if (tempAlert)     alertMsg += "TEMP HIGH:"  + String(temperature) + "C. ";
  if (ammoniaAlert)  alertMsg += "NH3 HIGH:"   + String(ammoniaPPM)  + "ppm. ";
  if (humidityAlert) alertMsg += "HUM:"        + String(humidity)    + "%. ";

  String sysStatus;
  int alertCount = (int)tempAlert + (int)ammoniaAlert + (int)humidityAlert;
  if      (alertCount == 0)  sysStatus = "SAFE  ";
  else if (alertCount > 1)   sysStatus = "ALRT! ";
  else if (tempAlert)        sysStatus = "TEMP! ";
  else if (ammoniaAlert)     sysStatus = "NH3!  ";
  else                       sysStatus = "HUM!  ";

  if (!allNormal) {
    if (WiFi.status() == WL_CONNECTED) {
      Firebase.setString(firebaseData, "/alerts/latest", "ALERT: " + alertMsg);
    } else {
      sendSMS("POULTRY ALERT!\n" + alertMsg +
              "\nT:" + String(temperature) + "C" +
              " H:" + String(humidity) + "%" +
              " NH3:" + String(ammoniaPPM) + "ppm");
    }
  }
  return sysStatus;
}

void setup() {
  Serial.begin(115200);

  pinMode(LED_RED1,   OUTPUT);
  pinMode(LED_RED2,   OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_GREEN,  OUTPUT);
  pinMode(BUZZER,     OUTPUT);
  digitalWrite(LED_RED1,   LOW);
  digitalWrite(LED_RED2,   LOW);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_GREEN,  LOW);
  digitalWrite(BUZZER,     LOW);

  dht.begin();

  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Poultry Monitor");
  lcd.setCursor(0, 1);
  lcd.print("Starting...");
  delay(2000);

  gsmSerial.begin(9600, SERIAL_8N1, GSM_RX, GSM_TX);
  delay(3000);

  Serial.println("Connecting to Wi-Fi...");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWi-Fi connected: " + WiFi.localIP().toString());
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Connected!");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP().toString());
    delay(2000);
    firebaseConfig.host = FIREBASE_HOST;
    firebaseConfig.signer.tokens.legacy_token = FIREBASE_AUTH;
    Firebase.begin(&firebaseConfig, &firebaseAuth);
    Firebase.reconnectWiFi(true);
    Serial.println("Firebase ready!");
  } else {
    Serial.println("\nWi-Fi failed. GSM fallback active.");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Failed");
    lcd.setCursor(0, 1);
    lcd.print("GSM Fallback ON");
    delay(2000);
  }

  digitalWrite(LED_GREEN, HIGH);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System Ready!");
  lcd.setCursor(0, 1);
  lcd.print("Monitoring ON");
  delay(2000);
}

void loop() {
  unsigned long now = millis();
  if (now - lastReadTime >= READ_INTERVAL) {
    lastReadTime = now;

    temperature  = dht.readTemperature();
    humidity     = dht.readHumidity();
    int mq135Raw = analogRead(MQ135_PIN);
    ammoniaPPM   = getRawToPPM(mq135Raw);
    int ldrRaw   = analogRead(LDR_PIN);
    lightPercent = map(ldrRaw, 4095, 0, 0, 100);

    if (isnan(temperature) || isnan(humidity)) {
      Serial.println("DHT22 read error!");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Sensor Error!");
      return;
    }

    Serial.println("============================");
    Serial.print("Temperature : "); Serial.print(temperature); Serial.println(" C");
    Serial.print("Humidity    : "); Serial.print(humidity);    Serial.println(" %");
    Serial.print("Ammonia     : "); Serial.print(ammoniaPPM);  Serial.println(" ppm");
    Serial.print("Light       : "); Serial.print(lightPercent);Serial.println(" %");
    Serial.print("WiFi        : "); Serial.println(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");

    String sysStatus = checkThresholds();
    updateLCD(sysStatus);

    if (WiFi.status() == WL_CONNECTED) {
      sendToFirebase(sysStatus);
    } else {
      Serial.println("No WiFi. GSM fallback active.");
    }

    Serial.println("Status: " + sysStatus);
    Serial.println("============================");
  }
}
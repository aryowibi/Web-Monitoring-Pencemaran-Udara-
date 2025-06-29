#include "MQ7.h"    // MQ7Sensor By Dustpancake [Edit max ADC by Sony]
#include "MQ135.h"  // https://github.com/amperka/TroykaMQ [Edit BaseMQ resolusi]
#include <DHT.h>
#include <GP2Y1010AU0F.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <HTTPClient.h>

//================== WIFI & THINGSPEAK ==================//
const char *ssid = "Aryoo";
const char *password = "aryo1106";
String APIKey = "WTJBBBSHU10W3T7I";
String serverName = "https://api.thingspeak.com/update?api_key=";

//================== MQ-7 Sensor ==================//
#define MQ7Pin 33
#define VOLTAGE 3.3
MQ7 mq7(MQ7Pin, VOLTAGE);
float CO;

//================== MQ-135 Sensor ==================//
#define SENSOR_PIN 32
MQ135 mq135(SENSOR_PIN);
float CO2;

//================== DHT22 Sensor ==================//
#define DHTPIN 5
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
float Temp, Hum;

//================== GP2Y1010 ==================//
int SHARP_LED_PIN = 25;
int SHARP_VO_PIN = 35;
GP2Y1010AU0F dustSensor(SHARP_LED_PIN, SHARP_VO_PIN);
#define NUM_DUST_READINGS 10
float dustReadings[NUM_DUST_READINGS];
int dustIndex = 0;
float totalDust = 0;
float avgDust = 0;
float PM10;

//================== LCD16x2 I2C ==================//
LiquidCrystal_I2C lcd(0x27, 16, 2);

//================== Waktu Pengiriman ==================//
unsigned long lastSendTime = 0;
unsigned long currentTime = 0;
const unsigned long interval = 180000; //3 menit

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Calibrating MQ Sensor");
  lcd.init();
  lcd.backlight();
  splashScreen("WAITING FOR", "SENSORS SETUP", 500);

  MQ7Calibrate(); delay(200);
  MQ135Calibrate(); delay(200);
  dht.begin(); delay(200);
  initDustSensor(); delay(200);

  splashScreen("AIR", "SENSE", 3000);
  connecting2WiFi(ssid, password);
  Serial.println("Void Setup Selesai");
}

void loop() {
  currentTime = millis();
  if (currentTime - lastSendTime >= interval) {
    // sendDataToThingSpeak(Temp, Hum, CO, CO2, PM10);
    sendDataToServer(Temp, Hum, CO, CO2, PM10);
    lastSendTime = currentTime;
    delay(200);
  } else {
    CO = mq7.readPpm();
    CO2 = mq135.readCO2();
    Temp = dht.readTemperature();
    Hum = dht.readHumidity();
    PM10 = readAverageDust(dustSensor);

    displayOnSerialMonitor(Temp, Hum, CO, CO2, PM10);

    // Tampilkan nilai parameter ke LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("CO2:"); lcd.print(CO2);
    lcd.setCursor(0, 1);
    lcd.print("CO:"); lcd.print(CO);
    delay(3000);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("PM10:"); lcd.print(PM10);
    lcd.setCursor(0, 1);
    lcd.print("T:"); lcd.print(Temp);
    lcd.print("C H:"); lcd.print(Hum);
    delay(3000);

    // Tampilkan hasil fuzzy
    float hasil = fuzzyTSK(CO2, CO, PM10, Temp, Hum);
    String statusUdara = getKategori(hasil);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Kualitas:");
    lcd.setCursor(0, 1);
    lcd.print(statusUdara);
    delay(3000);
  }
}

void sendDataToServer(float temp, float hum, float co, float co2, float pm10) {
  float hasil = fuzzyTSK(CO2, CO, PM10, Temp, Hum);
  String statusUdara = getKategori(hasil);


  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("https://web-airsense-monitoring.fasterstronger.site/api/sensor/json");
    http.addHeader("Content-Type", "application/json");

    String jsonData = "{";
    jsonData += "\"suhu\":\"" + String(temp) + "\",";
    jsonData += "\"kelembapan\":\"" + String(hum) + "\",";
    jsonData += "\"co\":\"" + String(co) + "\",";
    jsonData += "\"co2\":\"" + String(co2) + "\",";
    jsonData += "\"pm10\":\"" + String(pm10) + "\",";
    jsonData += "\"status\":\"" + statusUdara + "\"";
    jsonData += "}";

    int httpResponseCode = http.POST(jsonData);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      Serial.println("Response: " + response);
    } else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  } else {
    Serial.println("WiFi Disconnected");
  }
}


float fuzzyMembership(float x, float a, float b, float c) {
  if (x <= a || x >= c) return 0;
  else if (x == b) return 1;
  else if (x < b) return (x - a) / (b - a);
  else return (b - x) / (c - b);
}

float fuzzyTSK(float co2, float co, float pm10, float suhu, float hum) {
  // Keanggotaan CO2
  float co2_low = fuzzyMembership(co2, 0, 300, 400);
  float co2_med = fuzzyMembership(co2, 401, 600, 800);
  float co2_high = fuzzyMembership(co2, 801, 1000, 2000);

  // Keanggotaan CO
  float co_low = fuzzyMembership(co, 0, 2, 4);
  float co_med = fuzzyMembership(co, 4, 6.5, 9);
  float co_high = fuzzyMembership(co, 9, 10, 20);

  // Keanggotaan PM10
  float pm10_low = fuzzyMembership(pm10, 0, 12.5, 25);
  float pm10_med = fuzzyMembership(pm10, 26, 38, 50);
  float pm10_high = fuzzyMembership(pm10, 50, 75, 150);

  // Keanggotaan Suhu
  float suhu_opt1 = fuzzyMembership(suhu, 15, 18, 21); // nyaman
  float suhu_opt2 = fuzzyMembership(suhu, 26, 28, 30); // nyaman atas
  float suhu_bad = 1 - max(suhu_opt1, suhu_opt2);      // selain itu tidak nyaman

  // Keanggotaan Kelembaban
  float hum_low = fuzzyMembership(hum, 0, 30, 45);
  float hum_opt = fuzzyMembership(hum, 45, 60, 70);
  float hum_high = fuzzyMembership(hum, 70, 80, 100);

  // --- RULE EVALUATION ---

  // Contoh 3 aturan sederhana
  float w1 = min(co2_low, min(co_low, min(pm10_low, min(suhu_opt1, hum_low))));
  float z1 = 1; // Baik

  float w2 = min(co2_med, min(co_med, min(pm10_med, min(suhu_bad, hum_opt))));
  float z2 = 2; // Cukup

  float w3 = min(co2_high, min(co_high, min(pm10_high, min(suhu_bad, hum_high))));
  float z3 = 3; // Buruk

  // --- DEFUZZIFIKASI (Sugeno) ---
  float numerator = w1*z1 + w2*z2 + w3*z3;
  float denominator = w1 + w2 + w3;

  if (denominator == 0) return 0; // tidak terdefinisi

  float output = numerator / denominator;

  return output;
}

String getKategori(float nilai) {
  if (nilai <= 1.3) return "Baik";
  else if (nilai <= 2.4) return "Cukup";
  else return "Buruk";
}

// Pemanggilan fungsi:
// float hasil = fuzzyTSK(co2, co, pm10, suhu, hum);
// String status = getKategori(hasil);


// Fungsi splashScreen, connecting2WiFi, sendDataToThingSpeak, displayOnSerialMonitor, initDustSensor, MQ7Calibrate, MQ135Calibrate,
// readAverageDust belum ditampilkan di sini. Pastikan kamu tetap menyimpan dan menyisipkan fungsi-fungsi itu seperti sebelumnya!

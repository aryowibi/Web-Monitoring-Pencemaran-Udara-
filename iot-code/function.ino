//================================================//
//=================== MQ7 ========================//

void MQ7Calibrate() {
  Serial.println("Calibrating MQ7");
  mq7.calibrate();  // calculates R0
  Serial.print("R0: ");
  Serial.println(mq7.getR0());  // debug
  Serial.println("Calibration done!");
}

//================================================//
//=================== MQ135 ======================//
void MQ135Calibrate() {
  Serial.print("Calibrating MQ135 : ");
  mq135.calibrate();
  delay(1000);
  Serial.println(mq135.readCO2());
  if (mq135.readCO2() > 1000) {
    ESP.restart();
  }
}


//================================================//
//================ GP2Y1010AU0F ==================//
void initDustSensor() {
  dustSensor.begin();

  for (int i = 0; i < NUM_DUST_READINGS; i++) {
    dustReadings[i] = 0;
  }
  totalDust = 0;
  dustIndex = 0;
  avgDust = 0;

  for (int i = 0; i < NUM_DUST_READINGS; i++) {
    PM10 = readAverageDust(dustSensor);
    delay(200);
  }
}

float readAverageDust(GP2Y1010AU0F& sensor) {
  float dust = dustSensor.read();
  if (dust < 0) {
    dust = 0;
  } else {
    dust = 0.15 * dust;  // PM10 (estimasi) = k × dustDensity
  }

  totalDust -= dustReadings[dustIndex];  // Kurangi pembacaan lama
  dustReadings[dustIndex] = dust;        // Simpan pembacaan baru
  totalDust += dustReadings[dustIndex];  // Tambah ke total

  dustIndex = (dustIndex + 1) % NUM_DUST_READINGS;  // Update index
  avgDust = totalDust / NUM_DUST_READINGS;

  return avgDust;
}

//================================================//
//================ LCD 16x2 I2C ==================//
void splashScreen(String row_1, String row_2, int jeda) {
  lcd.clear();
  int row_1_length = row_1.length();        // Panjang teks
  int row_2_length = row_2.length();        // Panjang teks
  int padding_1 = (16 - row_1_length) / 2;  // Hitung padding untuk center
  int padding_2 = (16 - row_2_length) / 2;  // Hitung padding untuk center

  lcd.setCursor(padding_1, 0);
  lcd.print(row_1);

  lcd.setCursor(padding_2, 1);
  lcd.print(row_2);
  delay(jeda);
}

void displayOnLCD(float temp, float hum, float co, float co2, float pm10) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("T:" + String(temp, 1) + (char)223 + "C");
  // lcd.print("T:" + String(temp, 1) + "C");
  lcd.setCursor(9, 0);
  lcd.print("H:" + String(hum, 1) + "%");
  lcd.setCursor(0, 1);
  lcd.print("PM10:" + String(pm10, 0) + " ug/m3");
  delay(1250);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("CO : " + String(co, 1) + " ppm");
  lcd.setCursor(0, 1);
  lcd.print("CO2: " + String(co2, 1) + " ppm");
  delay(1250);


}

//================================================//
//================ Serial Monitor ================//
void displayOnSerialMonitor(float data1, float data2, float data3, float data4, float data5) {
  Serial.print("Temp : " + String(data1) + " C" + "\t");
  Serial.print("Hum : " + String(data2) + " %" + "\t");
  Serial.print("CO: " + String(data3) + " ppm" + "\t");
  Serial.print("CO2 : " + String(data4) + " ppm" + "\t");
  Serial.print("PM10 : " + String(data5) + " ug/m3" + "\t\n");
}

//================================================//
//================ WIFI Module ===================//
void connecting2WiFi(const char* SSID, const char* PASS) {
  WiFi.mode(WIFI_STA);
  int length = strlen(SSID);
  int padding = (16 - length) / 2; // Hitung padding untuk center

  lcd.clear();
  Serial.print("Connecting to ..");
  lcd.setCursor(0,0);
  lcd.print("Connecting to...");
  lcd.setCursor(padding,1);
  lcd.print(SSID);
  delay(2000);

  lcd.clear();
  lcd.setCursor(padding,0);
  lcd.print(SSID);

  WiFi.begin(SSID, PASS);
  int dotCount = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    lcd.setCursor((3 + dotCount), 1);
    lcd.print(".");
    dotCount++;
    if (dotCount > 10) {
      lcd.setCursor(3, 1);
      lcd.print("           "); // Hapus titik-titik
      dotCount = 0;
    }
    Serial.print('.');
    delay(250);
  }
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi Connected!");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
  Serial.println(WiFi.localIP());
  delay(3000);
  lcd.clear();
}


//================================================//
//================ ThingSpeak ====================//
void sendDataToThingSpeak(float data1,float data2,float data3,float data4,float data5){
  HTTPClient http; // Inisialisasi HTTP CLient

  String url = serverName + APIKey + "&field1=" + String(data1) + "&field2=" + String(data2) + "&field3=" + String(data3) + "&field4=" + String(data4) + "&field5=" + String(data5);

   http.begin(url.c_str()); // Send HTTP Reques / Inisialisasi
  int httpResponseCode = http.GET(); // Check status data

  Serial.println("Mengirim data ke Thingspeak");
  if (httpResponseCode > 0){
    Serial.print("HTTP Response code:");
    Serial.println(httpResponseCode);
  }
  else{
     Serial.print("Error COde");
     Serial.println(httpResponseCode);
  }
  http.end();
}

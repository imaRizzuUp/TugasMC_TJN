#include <ESP32Servo.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

// Blynk credentials
#define BLYNK_PRINT Serial  // Tambahkan ini untuk debug Blynk
#define BLYNK_TEMPLATE_ID "TMPL66ftqUCHq"
#define BLYNK_TEMPLATE_NAME "Fish Feeder"
#define BLYNK_AUTH_TOKEN "ynOH9rhJpezrVCeyKDgUP3faa3Fm7Zpf"

// WiFi credentials
const char* ssid = "nama_wifi";     // GANTI DENGAN NAMA WIFI
const char* pass = "pass_wifi";     // GANTI DENGAN PASSWORD WIFI

// Pin definitions
#define trigPin 14 // Pin TRIG sensor ultrasonik
#define echoPin 13 // Pin ECHO sensor ultrasonik
#define ledPin 2   // Pin LED bawaan ESP32 (biasanya GPIO2)
#define pin_Servo 25

// Variables for Blynk
BlynkTimer timer;
bool automaticFeeding = true;
unsigned long lastFeedTime = 0;
const unsigned long feedingInterval = 3600000; // 1 jam dalam milidetik

Servo myServo;

// Function to give food
void giveFoodNow() {
  Serial.println("Memberikan pakan ikan, selamat makan ikan🐟");
  
  // Debug pesan untuk servo
  Serial.println("Servo bergerak ke posisi 35 derajat");
  myServo.write(35);
  delay(1000);
  
  Serial.println("Servo kembali ke posisi 0 derajat");
  myServo.write(0);
  delay(500);
  
  lastFeedTime = millis();
  
  // Update status ke Blynk
  String statusMsg = "Pakan terakhir: " + String(lastFeedTime/60000) + " menit yang lalu";
  Blynk.virtualWrite(V2, statusMsg);
}

// Blynk connected callback
BLYNK_CONNECTED() {
  Serial.println("Blynk Connected!");
  Blynk.syncVirtual(V0);  // Sync automatic feeding status
}

// Handle tombol manual feeding dari Blynk
BLYNK_WRITE(V1) {
  if (param.asInt() == 1) {
    Serial.println("Tombol Feed Now ditekan!");
    giveFoodNow();
  }
}

// Handle switch automatic feeding dari Blynk
BLYNK_WRITE(V0) {
  automaticFeeding = param.asInt();
  if (automaticFeeding) {
    Blynk.virtualWrite(V2, "Mode Otomatis Aktif");
  } else {
    Blynk.virtualWrite(V2, "Mode Manual Aktif");
  }
}

// Function untuk mengecek dan update status pakan
void checkFoodLevel() {
  long duration, distance;

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(4);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  distance = duration * 0.034 / 2;

  // Update gauge level pakan di Blynk (V3)
  int foodLevel = map(distance, 7, 0, 0, 100); // Konversi jarak ke persentase
  foodLevel = constrain(foodLevel, 0, 100);
  Blynk.virtualWrite(V3, foodLevel);

  // Update status LED dan kirim notifikasi
  if (distance >= 7) {
    digitalWrite(ledPin, HIGH);
    Blynk.logEvent("food_warning", "Stok pakan kritis! Segera isi ulang!");
  } else if (distance >= 5) {
    Blynk.logEvent("food_warning", "Stok pakan tinggal setengah!");
  } else {
    digitalWrite(ledPin, LOW);
  }
}

void setup() {
  // Mulai Serial dengan baud rate yang lebih tinggi
  Serial.begin(115200);
  delay(100);
  
  Serial.println("\n\n");
  Serial.println("=== Fish Feeder Starting ===");
  
  // Inisialisasi Servo
  Serial.println("Menginisialisasi servo...");
  ESP32PWM::allocateTimer(0); // Alokasi timer untuk servo
  myServo.setPeriodHertz(50); // Standar 50Hz untuk servo
  myServo.attach(pin_Servo, 500, 2400); // Atur range pulse width
  
  // Test servo
  Serial.println("Testing servo...");
  myServo.write(0);  // Posisi awal
  delay(1000);
  myServo.write(35); // Test gerakan
  delay(1000);
  myServo.write(0);  // Kembali ke posisi awal
  Serial.println("Servo test selesai!");
  
  // Inisialisasi pin lainnya
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(ledPin, OUTPUT);
  
  // Connect to WiFi
  Serial.print("Mencoba koneksi ke WiFi: ");
  Serial.println(ssid);
  
  WiFi.disconnect(true);
  delay(1000);
  WiFi.mode(WIFI_STA);
  delay(1000);
  WiFi.begin(ssid, pass);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Terhubung!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Kekuatan Sinyal (RSSI): ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    
    Serial.println("\nMencoba koneksi ke Blynk...");
    Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
    
    timer.setInterval(30000L, checkFoodLevel);
  } else {
    Serial.println("\nGagal terhubung ke WiFi!");
    Serial.println("Kemungkinan penyebab:");
    Serial.println("1. Password WiFi salah");
    Serial.println("2. Sinyal WiFi terlalu lemah");
    Serial.println("3. SSID (nama WiFi) salah");
    Serial.println("\nSilakan cek kredensial WiFi dan coba lagi.");
    delay(5000);
    ESP.restart();
  }
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    if (Blynk.connected()) {
      Blynk.run();
      timer.run();

      // Automatic feeding logic
      if (automaticFeeding && (millis() - lastFeedTime >= feedingInterval)) {
        giveFoodNow();
      }
    } else {
      Serial.println("Tidak terhubung ke Blynk. Mencoba menghubungkan ulang...");
      Blynk.connect();
    }
  } else {
    Serial.println("WiFi terputus! Mencoba menghubungkan ulang...");
    WiFi.begin(ssid, pass);
    delay(5000);
  }
}
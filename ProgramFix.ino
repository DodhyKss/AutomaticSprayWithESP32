#include <DHT.h>
#include <Arduino.h>
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>


#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"


#define WIFI_SSID "AutomaticSpray"
#define WIFI_PASSWORD "AutomaticSpray"
#define API_KEY "AIzaSyCBceDT1xlJjBAPK_pxTNtpTZemVTVsao8"
#define DATABASE_URL "https://sprayotomation-default-rtdb.firebaseio.com/otomation" 

//Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

bool signupOK = false;

#define relay1 14
#define relay2 16
#define pir 13
#define dhtPin 15
#define dhtType DHT11
int servoPin = 25;

DHT dht(dhtPin, dhtType);

int pulseTime;
int statusPir;
int h;
int t;
int totalSpray = 0;
int push = 0;
int t_reset = 0;
int fan = 0;
int sensor = 1;
int counter = 0;

void setup(){
  Serial.begin(115200);

  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(pir, INPUT);
  pinMode(servoPin, OUTPUT);
  dht.begin();
  digitalWrite(relay1, LOW);
  digitalWrite(relay2, HIGH);
  
}

void loop(){
  if (Firebase.ready() && signupOK){

  
    // matikan sensor
    if(sensor == 0){
      t = 0;
      h = 0;
      Serial.println("sensor mati");
    }else if(sensor == 1){
      h = dht.readHumidity();
      t = dht.readTemperature();
      statusPir = digitalRead(pir);
      Serial.println("sensor nyala");
      Serial.println("sensor PIR: ");
      Serial.print(statusPir);
    }
  
    //nyalakan kipas
    if(fan == 1){
      digitalWrite(relay2, LOW);
      Serial.println("kipas nyala");
      delay(10000);
      digitalWrite(relay2, HIGH);
      Serial.println("kipas mati");
      fan = 0;
    }
  
    //reset semprotan
    if(t_reset == 1){
      totalSpray = 0;
      t_reset = 0;
      Serial.println("tombol reset tertekan");
    }
  
    //semprot
    while(push == 1 && totalSpray < 86){
      if(statusPir == 0){
        spray();
        stopSpray();
        digitalWrite(relay2, LOW);
        Serial.println("kipas nyala");
        delay(10000);
        digitalWrite(relay2, HIGH);
        Serial.println("kipas mati");
        totalSpray += 1;
        counter += 1;
      }else if(statusPir == 1){
        push = 0;
        counter = 0;
      }
      
      if(counter == 5){
        push = 0;
        counter = 0;
      }
    }
    sendToDbase();
    receiveFromDbase();
    
  }
}

void spray() {
  for(pulseTime = 0; pulseTime <= 1050; pulseTime += 50) {
    digitalWrite(servoPin, HIGH);
    delayMicroseconds(pulseTime);
    digitalWrite(servoPin, LOW);
    delay(25);
  }
}

void stopSpray() {
  for(pulseTime = 1200; pulseTime >= 0; pulseTime -= 50) {
    digitalWrite(servoPin, HIGH);
    delayMicroseconds(pulseTime);
    digitalWrite(servoPin, LOW);
    delay(25);
  }
}

void sendToDbase() {
  if(Firebase.RTDB.setString(&fbdo, "otomation/aktif", String(sensor))){
    Serial.println("Data sensor terkirim");
  }
  if(Firebase.RTDB.setString(&fbdo, "otomation/kelembaban", String(h))){
    Serial.println("Data kelembaban terkirim");
  }
  if(Firebase.RTDB.setString(&fbdo, "otomation/kipas", String(fan))){
    Serial.println("Data sensor terkirim");
  }
  if(Firebase.RTDB.setString(&fbdo, "otomation/pir", String(statusPir))){
    Serial.println("Data sensor PIR terkirim");
  }
  if(Firebase.RTDB.setString(&fbdo, "otomation/reset", String(t_reset))){
    Serial.println("Data t_reset terkirim");
  }
  if(Firebase.RTDB.setString(&fbdo, "otomation/spray", String(push))){
    Serial.println("Data push terkirim");
  }
  if(Firebase.RTDB.setString(&fbdo, "otomation/suhu", String(t))){
    Serial.println("Data suhu terkirim");
  }
  if(Firebase.RTDB.setString(&fbdo, "otomation/total", String(totalSpray))){
    Serial.println("Data total spray terkirim");
  }
}


void receiveFromDbase() {
  if(Firebase.RTDB.getString(&fbdo, "otomation/aktif")){
    Serial.println("Data sensor diterima");
    sensor = fbdo.stringData().toInt();
  }
  if(Firebase.RTDB.getString(&fbdo, "otomation/kipas")){
    Serial.println("Data sensor diterima");
    fan = fbdo.stringData().toInt();
  }
  if(Firebase.RTDB.getString(&fbdo, "otomation/reset")){
    Serial.println("Data t_reset diterima");
    t_reset = fbdo.stringData().toInt();
  }
  if(Firebase.RTDB.getString(&fbdo, "otomation/spray")){
    Serial.println("Data push diterima");
    push = fbdo.stringData().toInt();
  }
  if(Firebase.RTDB.getString(&fbdo, "otomation/total")){
    Serial.println("Data total spray diterima");
    totalSpray = fbdo.stringData().toInt();
  } 
}

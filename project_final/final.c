#include <WiFi.h>  // WiFi control for ESP32
#include <DHTesp.h>         // DHT for ESP32 library
#include "ThingsBoard.h"
#include <LiquidCrystal_I2C.h>
#include "RTClib.h"
#include <stdio.h>
#include <stdlib.h>
#include <esp_wifi.h>
#include <Relay.h>

#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))
#define WIFI_AP_NAME                  "Wokwi-GUEST"
#define WIFI_PASSWORD                 ""
#define THINGSBOARD_MQTT_SERVER       "thingsboard.cloud"
#define THINGSBOARD_MQTT_ACESSTOKEN   "Q57KqoBX35FjSiHgp07Q"
#define SERIAL_DEBUG_BAUD    115200
#define DHT_PIN 15
#define TEMP_UPPER_THRESHOLD  30 // upper temperature threshold
#define TEMP_LOWER_THRESHOLD  10 // lower temperature threshold
#define CHY_UPPER_THRESHOLD 6000
#define CHY_LOWER_THRESHOLD 5000
#define HUM_THRESHOLD 67
#define PIR_PIN 13
#define LDR 35
#define RELAY_PIN 2
#define BUZZER 25

const int timer1 = 15;    // lama waktu timer 1 ->15s
const int timer2 = 30;   // lama waktu timer 2 ->20s
const int timer3 = 60;   // lama waktu timer 3 ->60s

uint8_t newMACAddress[] = {0x32, 0xAE, 0xA4, 0x07, 0x0D, 0x11};

int quant = 20;
int led_passed = 0;  // Time passed after LED was turned ON, milliseconds.
int send_passed = 0; // Time passed after temperature/humidity data was sent, milliseconds.
bool subscribed = false; // Set to true if application is subscribed for the RPC messages.
int current_led = 0; // LED number that is currenlty ON.
float temperature = 0;
float humidity = 0;
boolean pir01 = LOW, pompa01 = LOW;
int timer01, S01 = 0, waktu = 0, w01 = 0, w02 = 0, w03 = 0;
int waktukirim = 0;


/*RTC*/
RTC_DS1307 rtc;
int timeStart = 0;
char* substring(char*, int, int);
/*--------------*/

/*RELAY*/
/*
  String statusPompa(int state) {return state==0?"Mati":"Hidup";}
  void dWrite(int pin, bool hilo){
  digitalWrite(pin,hilo);
  delay(2000);
  }
*/
/*--------------/

  /*LDR*/
const float GAMMA = 0.7;
const float RL10 = 50;
float getIntCahaya()
{
  int analogValue = analogRead(LDR);
  float voltage = analogValue / 4096. * 5;
  float resistance = 2000 * voltage / (1 - voltage / 5);
  float lux = pow(RL10 * 1e3 * pow(10, GAMMA) / resistance, (1 / GAMMA));
  return roundf(lux);
}
/*--------------*/

/*PIR ---jika ada orang relay mati*/
int pirState = 0;

void checkMotion() {
  if (digitalRead(PIR_PIN) == HIGH) {
    if (pirState == LOW)
    {
      digitalWrite(RELAY_PIN, LOW);
      cetak(13, 3, "MATI     ");
      delay(10000);
      digitalWrite(RELAY_PIN, HIGH);
      cetak(13, 3, "HIDUP     ");
    }
  }
}

/*DHT*/
DHTesp dht;

/*LCD*/
LiquidCrystal_I2C lcd(0x27, 20, 4);
void cetak( int start, int en, String teks) {
  lcd.setCursor(start, en);
  lcd.print(teks);
}

void lcd_tampil (float temp, float hum, float lux) {
  lcd.init();
  lcd.backlight();
  cetak(0, 0, "Cahaya     :");
  cetak(13, 0, String(lux));
  cetak(0, 1, "Suhu       :");
  cetak(13, 1, String(temp));
  cetak(19, 1, "C");
  cetak(0, 2, "Kelembaban :");
  cetak(13, 2, String(hum));
  cetak(19, 2, "%");
  cetak(0, 3, "Pompa      : MATI ");
}
/*--------------------/
  /*LED*/
const uint8_t leds_control[] = { 33 };
// Initial period of LED cycling.
int led_delay = 1000;
// Period of sending a temperature/humidity data.
int send_delay = 2000;
void runLed(int i, bool send) {
  pinMode(leds_control[i], OUTPUT);
  digitalWrite(leds_control[i], HIGH);
  delay(send ? send_delay : led_delay);
  digitalWrite(leds_control[i], LOW);
}
/*-------------------/

  /*-------Buzzer-----*/
#define MAX_SIZE 30
#define MIN_SIZE 3
//const int buzzer = 25;
const int s[3] = {30, 30, 15};

/*WIFI connection*/
WiFiClient espClient;
ThingsBoard tb(espClient);
int status = WL_IDLE_STATUS;

void reconnect() {
  // Loop until we're reconnected
  status = WiFi.status();
  if ( status != WL_CONNECTED) {
    WiFi.begin(WIFI_AP_NAME, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println(WiFi.localIP());
    Serial.println("Connected to AP");
  }
}

/*--------------------*/
void loop()
{
  DateTime now = rtc.now();     //ambil waktu
  if (now.second() != w01)      //bandingkan second
  {
    waktu = waktu + 1;       //diproses apabila second berbeda
    waktukirim = waktukirim + 1;
  }
  if (w02 == 0) {
    waktu = 0; //apabila timer tdk aktif waktu di set 0
  }
  now = rtc.now();   //ambil waktu untuk perbandingan
  w01 = now.second(); //ambil nilai detiknya

  float lux = getIntCahaya();
  // Reconnect to WiFi, if needed
  if (WiFi.status() != WL_CONNECTED) {
    reconnect();
    return;
  }

  // Reconnect to ThingsBoard, if needed
  if (!tb.connected()) {
    subscribed = false;
    // Connect to the ThingsBoard
    Serial.print("Connecting to: ");
    Serial.println(THINGSBOARD_MQTT_SERVER);
    Serial.print(" with access token ");
    Serial.println(THINGSBOARD_MQTT_ACESSTOKEN);
    if (!tb.connect(THINGSBOARD_MQTT_SERVER, THINGSBOARD_MQTT_ACESSTOKEN)) {
      Serial.println("Failed to connect");
      return;
    }
  }

  pirState = 0;
  if (digitalRead(RELAY_PIN) == 1) {
    checkMotion();
  }

  TempAndHumidity lastValues = dht.getTempAndHumidity();
  cetak(13, 0, String(lux));
  cetak(13, 1, String(lastValues.temperature));
  cetak(13, 2, String(lastValues.humidity));

  if (humidity <= HUM_THRESHOLD or waktu < w02 )
  {
    digitalWrite(RELAY_PIN, HIGH);
    checkMotion();
    if (w02 == 0)
    {
      cetak(13, 3, "HIDUP");
      tb.sendTelemetryBool("Pompa", HIGH);
    } else
    {

      if (w02 - waktu>= 0) {
        cetak(13, 3, String(w02 - waktu) + "    ");
        tb.sendTelemetryBool("Pompa", HIGH);
      }
      else {
        digitalWrite(RELAY_PIN, LOW);
        cetak(13, 3, "MATI   ");
        tb.sendTelemetryBool("Pompa", LOW);
      }
    }
    if (w03 == 0)
    {
      tone(BUZZER, 1000, 20);
      noTone(BUZZER);
      delay(100);
      tone(BUZZER, 1000, 20);
      noTone(BUZZER);
      delay(200); w03 = 1;
    }
  } else
  {
    digitalWrite(RELAY_PIN, LOW);
    cetak(13, 3, "MATI "); w03 = 0;
  }

  if (digitalRead(27) == HIGH) //set timer 1
  {
    delay(100);

    w02 = timer1;
  }
  if (digitalRead(14) == HIGH) //set timer 2
  {
    delay(100);

    w02 = timer2;
  }
  if (digitalRead(12) == HIGH) //Set timer 3
  {
    delay(100);
    w02 = timer3;
  }
  if (w02 == 0) // apabila tdk ada timer diset maka pompa mati
  {
    //pompamati();
    w02 = 0;
  }

  if (waktukirim == 5 ) {
    kirimdata();
  }
  if (waktukirim > 5) {
    waktukirim = 0; //waktu kirim ke data di set 5dtk sekali
  }

  tb.loop();
}

int timer() {
  DateTime timer = rtc.now();
  return timer.second();
}


void InitWiFi()
{
  Serial.println("Connecting to AP ...");
  WiFi.begin(WIFI_AP_NAME, WIFI_PASSWORD);
  tone(BUZZER, 1000, 20);
  noTone(BUZZER);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(WiFi.localIP());
  Serial.println("Connected to AP");
  tone(BUZZER, 2000, 20);
  noTone(BUZZER); delay(100);
  tone(BUZZER, 2000, 20);
  noTone(BUZZER);
}

void setup()
{
  Serial.begin(SERIAL_DEBUG_BAUD);
  WiFi.begin(WIFI_AP_NAME, WIFI_PASSWORD);
  InitWiFi();
  if (! rtc.begin())
  {
    Serial.println("RTC TIDAK TERBACA");
    while (1);
  }

  if (! rtc.isrunning())
  {
    Serial.println("RTC is NOT running!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));//update rtc dari waktu komputer
  }

  esp_wifi_set_mac(WIFI_IF_STA, &newMACAddress[0]);
  timeStart = timer();
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(PIR_PIN, INPUT);
  pinMode(BUZZER, OUTPUT);

  // Initialize temperature sensor
  dht.setup(DHT_PIN, DHTesp::DHT22);

  lcd.init();
  lcd.backlight();
  cetak(3, 0, "Papan Informasi");
  cetak(2, 1, "Suhu & Kelembaban");
  cetak(3, 3, "Kelompok 1");
  delay(3000);
  lcd.clear();
  TempAndHumidity lastValues = dht.getTempAndHumidity();
  float lux = getIntCahaya();
  lcd_tampil (lastValues.temperature, lastValues.humidity, lux);

}

void kirimdata()
{
  float lux = getIntCahaya();
  TempAndHumidity lastValues = dht.getTempAndHumidity();

  if (isnan(lastValues.humidity) || isnan(lastValues.temperature) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    tone(BUZZER, 2000, 20);;
    noTone(BUZZER);
  }
  else
  {
    temperature = lastValues.temperature;
    humidity = lastValues.humidity;
    tb.sendTelemetryFloat("temperature", temperature);
    tb.sendTelemetryFloat("humidity", humidity);
  }
  tb.sendTelemetryFloat("cahaya", lux);
  if (digitalRead(PIR_PIN) == HIGH) {
    pir01 = HIGH;
  }
  else {
    pir01 = LOW;
  }
  tb.sendTelemetryBool("Motion", pir01 );
  if (humidity <= HUM_THRESHOLD)
  {
    tb.sendTelemetryBool("Pompa", HIGH );
  } else
  {
    tb.sendTelemetryBool("Pompa", LOW );
  }
  DateTime time = rtc.now();
  int timeEnd = timer();

  Serial.println(String("Waktu : ") + time.timestamp(DateTime::TIMESTAMP_TIME));
  Serial.println("Cahaya     : " + String(lux));
  Serial.println("Suhu       : " + String(temperature));
  Serial.println("Kelembaban : " + String(humidity));
  Serial.println("data Terkirim....");
  tone(BUZZER, 2000, 20);
  noTone(BUZZER);
  delay(100);
  tone(BUZZER, 2000, 20);
  noTone(BUZZER);
  delay(100);
  tone(BUZZER, 2000, 20);
  noTone(BUZZER);
  delay(200);
}

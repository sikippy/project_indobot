#include <ArduinoJson.h>
#include <WiFi.h>  // WiFi control for ESP32
#include <DHTesp.h>         // DHT for ESP32 library
#include <LiquidCrystal_I2C.h>
#include <stdio.h>
#include <stdlib.h>
#include <Relay.h>
#include "ThingsBoard.h"

#define WIFI_AP_NAME                  "Wokwi-GUEST"
#define WIFI_PASSWORD                 ""
#define THINGSBOARD_MQTT_SERVER       "thingsboard.cloud"
#define THINGSBOARD_MQTT_ACESSTOKEN   //"eHRy6F4hnvUUGTlpdWlD"
#define SERIAL_DEBUG_BAUD    115200
#define DHT_PIN 15
#define TEMP_UPPER_THRESHOLD  30 // upper temperature threshold
#define TEMP_LOWER_THRESHOLD  10 // lower temperature threshold
#define CHY_UPPER_THRESHOLD 250
#define CHY_LOWER_THRESHOLD 200
#define HUM_THRESHOLD  67
#define PIR_PIN 13
#define LDR 35
#define RELAY_PIN 2
#define COMPCOUNT 5
#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

/*global*/
int  waktu, w01 = 0;
int temperature = 0;
float humidity = 0;
//uint8_t newMACAddress[] = {0x32, 0xAE, 0xA4, 0x07, 0x0D, 0x12};
typedef struct Component {
  int index;
  int pin;
  int execTime;
  int delayTime;
} component;

Component comp[5];

/*---------------*/

int arrComp[5][4] = {{1, 26, 0, 10}, //merah
  {2, 27, 60, 10},//kuning
  {3, 14, 300, 10},//hijau
  {4, 12, 600, 10},//hitam
  {5, 22, 0, 10} //abu
};

void insertComp (component *comp, int index, int pin, int execTime, int delayTime) {
  comp->index = index ++;
  comp->pin = pin;
  comp->execTime = execTime;
  comp->delayTime = delayTime;
  pinMode(pin, INPUT);
}
/*-------------*/
/*RELAY*/
String statusPompa(int state) {
  return state == 0 ? "MATI" : "HIDUP";
}
void dWrite(int pin, bool hilo) {
  digitalWrite(pin, hilo);
}
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
  return lux;
}
/*--------------*/

/*PIR ---jika ada orang relay pompa untuk siram mati*/
int pirState = 0;

void checkMotion() {
  if (digitalRead(PIR_PIN) == HIGH) {
    if (pirState == LOW)
    {
      dWrite(RELAY_PIN, LOW);
      pirState = HIGH;
    }
  }
  else {
    if (pirState == HIGH) {
      dWrite(RELAY_PIN, HIGH);
      pirState = LOW;
    }
  }
}

/*DHT*/
DHTesp dht;
/*---------------------/
  /*LCD*/
LiquidCrystal_I2C lcd(0x27, 20, 4);
void cetak( int start, int en, String teks) {
  lcd.setCursor(start, en);
  lcd.print(teks);
}

void lcd_tampil (int temp, float hum, float lux, bool start) {
  if (!start) {
    cetak(0, 0, "Cahaya     :");
    cetak(13, 0, String(lux));
    cetak(0, 1, "Suhu       :");
    cetak(13, 1, String(temp));
    cetak(19, 1, "C");
    cetak(0, 2, "Kelembaban :");
    cetak(13, 2, String(hum));
    cetak(19, 2, "%");
    cetak(0, 3, "Pompa      :");
    cetak(13, 3, String(statusPompa(digitalRead(RELAY_PIN))));
  } else {
    cetak(3, 0, "Papan Informasi");
    cetak(2, 1, "Suhu & Kelembaban");
    cetak(3, 3, "Kelompok 1");
    delay(3000);
    lcd.clear();
  }
}
/*--------------------/
  /*LED*/
const uint8_t leds_control[] = {33};
// Initial period of LED cycling.
int led_delay = 80;
// Period of sending a temperature/humidity data.
int send_delay = 50;
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
const int buzzer = 25;
const int s[3] = {30, 30, 15};
const int doremi[MAX_SIZE] = {
  0x00770076, 0x0086007F, 0x0093008D, 0x009B0096, 0x00A600A1,
  0x00AC00A8, 0x00B300AF, 0x00C000B8, 0x00C400C4, 0x00C900C5,
  0x00D400CD, 0x00DE00DC, 0x00E200DF, 0x00E200DF, 0x00DE00DC,
  0x00D400CD, 0x00C900C5, 0x00C400C4, 0x00C000B8, 0x00B300AF,
  0x00AC00A8, 0x00A600A1, 0x009B0096, 0x0093008D, 0x0086007F
};

const int mario[MAX_SIZE] = {
  0x00C900C5, 0x00C900C5, 0x00C900C5, 0x00C000B8, 0x00C900C5,
  0x00DE00DC, 0x00DE00DC, 0x00C000B8, 0x00A600A1, 0x0093008D,
  0x00AC00A8, 0x00B300AF, 0x00AC00A8, 0x00AC00A8, 0x00A600A1,
  0x00C900C5, 0x00DE00DC, 0x00E200DF, 0x00DE00DC, 0x00C900C5,
  0x00C000B8, 0x00C400C4, 0x00B300AF
};

const int tone_alarm[MAX_SIZE] = { 6645, 6645, 6645, 4186, 5588, 4186, 6645, 4186, 5588, 4186, 6645, 4186};

int arrAtr[3][4] = {{20000, 0, 50, 30},
  {10000, 0, 20, 30},
  {50, 0, 20, 15}
};

struct Node *head;

struct Node {
  int index;
  int nfs;
  int ndelay;
  int nduration;
  int size;
  struct Node *next;
};

void insertArray(int index, int fs, int udelay, int dur, int s) {
  struct Node* lP = (struct Node*) malloc(sizeof(struct Node));
  lP->index = index + 1;
  lP->nfs = fs;
  lP->ndelay = udelay;
  lP->nduration = dur;
  lP->size = s;
  lP->next = head;
  head = lP;
  pinMode(buzzer, OUTPUT);
}

const int *getArr(int mode) {
  switch (mode) {
    case 1 : return doremi; break;
    case 2 : return mario; break;
    case 3 : return tone_alarm; break;
  }
}

void setTone(int mode) {
  struct Node* ptr;
  ptr = head;

  while (ptr != NULL) {
    if (ptr->index == mode) {
      for (int i = 0; i < ptr->size; i++)
      {
        tone(buzzer,  getArr(mode)[i] / ptr->nfs, ptr->nduration);
        delay(ptr->ndelay);
      }
    }
    ptr = ptr->next;
  }
}
/*----------------------*/

/*WIFI connection*/
WiFiClient espClient;
ThingsBoard tb(espClient);
int status = WL_IDLE_STATUS;

/*wifi*/
void InitWiFi()
{
  Serial.println("Connecting to AP ...");
  // attempt to connect to WiFi network
  WiFi.begin(WIFI_AP_NAME, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to AP");
}

void reconnect() {
  // Loop until we're reconnected
  status = WiFi.status();
  if ( status != WL_CONNECTED) {
    WiFi.begin(WIFI_AP_NAME, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("Connected to AP");
  }
}
/*---------------------*/
void hitung(int execTime, int ndelay) {
  delay(ndelay);
  while (execTime != 0) {
    Serial.println(execTime);
    dWrite(RELAY_PIN, HIGH);
    cetak(13, 3, String(execTime = execTime - 1 ) + "   ");
  }
  dWrite(RELAY_PIN, LOW);
  cetak(13, 3, "MATI   ");
}
/*--------------------*/
void loop()
{
  String message = "";
  // if (WiFi.status() != WL_CONNECTED) {
  //   reconnect();
  // }
  // if (!tb.connected()) {
  //   message += "Connecting MQTT to: "+String(THINGSBOARD_MQTT_SERVER)+
  //             " with access token "+String(THINGSBOARD_MQTT_ACESSTOKEN);
  //   Serial.println(message);
  // }

  float lux = getIntCahaya();
  TempAndHumidity lastValues = dht.getTempAndHumidity();
  lcd_tampil (lastValues.temperature, lastValues.humidity, lux, false);

  if (lux > CHY_UPPER_THRESHOLD) {
    setTone(1);
    Serial.println("nyanyi 1");
  } else if (CHY_LOWER_THRESHOLD > lux) {
    setTone(2);
    Serial.println("nyanyi 2");
  } else {
    tone(buzzer,0,0);
    Serial.println("no nyanyi");
  }

  if (digitalRead(comp[0].pin) == HIGH) {
    Serial.println("pb0");
    delay(comp[0].delayTime);
    dWrite(RELAY_PIN, HIGH);
    cetak(13, 3, "HIDUP ");
    checkMotion();
  }
  else if (digitalRead(comp[1].pin) == HIGH) {
    Serial.println("pb1");
    hitung(comp[1].execTime, comp[1].delayTime);
  }
  else if (digitalRead(comp[2].pin) == HIGH) {
    Serial.println("pb2");
    hitung(comp[2].execTime, comp[2].delayTime);
  }
  else if (digitalRead(comp[3].pin) == HIGH) {
    Serial.println("pb3");
    hitung(comp[3].execTime, comp[3].delayTime);
  }
  else if (digitalRead(comp[4].pin) == HIGH) {
    Serial.println("pb4");
    dWrite(RELAY_PIN, LOW);
    cetak(13, 3, "MATI   ");
  }
  else if (lastValues.humidity < HUM_THRESHOLD) {
    setTone(3);
    dWrite(RELAY_PIN, HIGH);
    cetak(13, 3, "HIDUP   ");
    checkMotion();
    Serial.println("1");
  }
  else  if (TEMP_LOWER_THRESHOLD > lastValues.temperature) {
    setTone(3);
    dWrite(RELAY_PIN, HIGH);
    cetak(13, 3, "DINGIN");
    Serial.println("2");
  }
  else if (TEMP_UPPER_THRESHOLD < lastValues.temperature) {
    setTone(3);
    dWrite(RELAY_PIN, HIGH);
    cetak(13, 3, "PANAS");
    Serial.println("3");
  }

waktu = 1;
int timestart = waktu * 10;
//handle kirim tb
while (waktu < timestart) {
  float lux = getIntCahaya();
  TempAndHumidity lastValues = dht.getTempAndHumidity();
  lcd_tampil (lastValues.temperature, lastValues.humidity, lux, false);
  waktu = waktu + 1;
  int timeend = waktu;
  if (timestart == timeend) {
    kirimdata(lux);
    runLed(0, true);
  }
}

tb.loop();
}
void kirimdata(float lux) {
  TempAndHumidity lastValues = dht.getTempAndHumidity();
  lcd_tampil (lastValues.temperature, lastValues.humidity, lux, false);
  if (isnan(lastValues.humidity) || isnan(lastValues.temperature) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    setTone(3);
  }
  else {
    temperature = lastValues.temperature;
    humidity = lastValues.humidity;
    tb.sendTelemetryInt("temperature", temperature);
    tb.sendTelemetryFloat("humidity", humidity);
    tb.sendTelemetryFloat("cahaya", lux);
    runLed(0, true);
  }
}
void setup() {
  Serial.begin(SERIAL_DEBUG_BAUD);
  //InitWiFi();
  //esp_wifi_set_mac(WIFI_IF_STA, &newMACAddress[0]);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(PIR_PIN, INPUT);
  for (size_t i = 0; i < COUNT_OF(leds_control); ++i) {
    pinMode(leds_control[i], OUTPUT);
  }
  for (int i = 0; i < MIN_SIZE; i++) {
    insertArray(i, arrAtr[i][0], arrAtr[i][1], arrAtr[i][2], arrAtr[i][3]);
  }
  for (int j = 0; j < COMPCOUNT; j++) {
    insertComp(&comp[j], j, arrComp[j][1], arrComp[j][2], arrComp[j][3]);
  }
  dht.setup(DHT_PIN, DHTesp::DHT22);
  lcd.init();
  lcd.backlight();
  lcd_tampil(0, 0, 0, true);
}


#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>  // WiFi control for ESP32
#endif
#include <DHTesp.h>         // DHT for ESP32 library
#include "ThingsBoard.h"
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include "RTClib.h"
#include <stdio.h>
#include <stdlib.h>
#include <esp_wifi.h>
#include <Relay.h>

#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))
#define WIFI_AP_NAME                  "Wokwi-GUEST"
#define WIFI_PASSWORD                 ""
#define THINGSBOARD_MQTT_SERVER       "thingsboard.cloud"
#define THINGSBOARD_MQTT_ACESSTOKEN   "eHRy6F4hnvUUGTlpdWlD"
#define SERIAL_DEBUG_BAUD    115200
#define DHT_PIN 15
#define TEMP_UPPER_THRESHOLD  30 // upper temperature threshold
#define TEMP_LOWER_THRESHOLD  27 // lower temperature threshold
#define CHY_UPPER_THRESHOLD 30
#define CHY_LOWER_THRESHOLD 10
#define PIR_PIN 13
#define LDR 35
#define RELAY_PIN 2

uint8_t newMACAddress[] = {0x32, 0xAE, 0xA4, 0x07, 0x0D, 0x12};

int quant = 20;
int led_passed = 0;  // Time passed after LED was turned ON, milliseconds.
int send_passed = 0; // Time passed after temperature/humidity data was sent, milliseconds.
bool subscribed = false; // Set to true if application is subscribed for the RPC messages.
int current_led = 0; // LED number that is currenlty ON.
float temperature = 0;
float humidity = 0;

/*RTC*/
RTC_DS1307 rtc;
int timeStart= 0;
char* substring(char*, int, int);
/*--------------*/

/*RELAY*/
String statusPompa(int state) {return state==0?"Mati":"Hidup";}
void dWrite(int pin, int hilo){
  digitalWrite(pin,hilo);
  delay(600);
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

/*PIR ---jika ada orang tutup putaran keran*/
int pirState = 0;

void checkMotion() {
  if (digitalRead(PIR_PIN) == HIGH) {
    if (pirState == LOW)
    {
      dWrite(RELAY_PIN, HIGH);
      pirState = HIGH;
    }
  }
  else {
    if (pirState == HIGH) {
      dWrite(RELAY_PIN, LOW);
      pirState = LOW;
    }
  }
}

/*DHT*/
DHTesp dht;
/*---------------------/

  /*Motor*/
Servo myservo;
/*---------------------/

  /*LCD*/
LiquidCrystal_I2C lcd(0x27, 20, 4);
void cetak( int start, int en, String teks) {
  lcd.setCursor(start, en);
  lcd.print(teks);
}

void lcd_tampil (float temp, float hum, float lux) {
  lcd.init();
  lcd.backlight();
  cetak(0, 0, "Cahaya :");
  cetak(13, 0 ,String(lux));
  cetak(0, 1, "Suhu :");
  cetak(13, 1, String(temp));
  cetak(19, 1, "C");
  cetak(0, 2, "Kelembaban :");
  cetak(13, 2, String(hum));
  cetak(19, 2, "%");
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

const int tone_alarm[MAX_SIZE] = { 6645, 6645, 6645, 4186, 5588, 4186, 6645, 4186, 5588, 4186, 6645, 4186, 0, 0, 0};

int arrAtr[3][4] = {{20000, 0, 500, 30},
  {10000, 0, 200, 30},
  {100, 0, 200, 15}
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
    Serial.println(ptr->index);
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

RPC_Response processDelayChange(const RPC_Data & data)
{
  Serial.println("Received the set delay RPC method");
  // Process data
  led_delay = data;
  Serial.print("Set new delay: ");
  Serial.println(led_delay);
  return String(led_delay);
}

RPC_Response processGetDelay(const RPC_Data & data)
{
  Serial.println("Received the get value method");
  return String(led_delay);
}

RPC_Response processSetGpioState(const RPC_Data & data)
{
  Serial.println("Received the set GPIO RPC method");
  int pin = data["pin"];
  bool enabled = data["enabled"];
  Serial.print("Setting LED ");
  Serial.print(pin);
  Serial.print(" to state ");
  Serial.println(enabled);

  // Red: 26, Green: 33, Blue: 25
  digitalWrite(pin, enabled);
  return String("{\"" + String(pin) + "\": " + String(enabled ? "true" : "false") + "}");
}

RPC_Response processGetGpioState(const RPC_Data & data)
{
  Serial.println("Received the get GPIO RPC method");
  String respStr = "{";

  for (size_t i = 0; i < COUNT_OF(leds_control); ++i) {
    int pin = leds_control[i];
    Serial.print("Getting LED ");
    Serial.print(pin);
    Serial.print(" state ");
    bool ledState = digitalRead(pin);
    Serial.println(ledState);

    respStr += String("\"" + String(pin) + "\": " + String(ledState ? "true" : "false") + ", ");
  }  //"pin" :
  respStr = respStr.substring(0, respStr.length() - 2);
  respStr += "}";
  return respStr;
}

// RPC handlers
RPC_Callback callbacks[] = {
  { "setValue",         processDelayChange },
  { "getValue",         processGetDelay },
  { "setGpioStatus",    processSetGpioState },
  { "getGpioStatus",    processGetGpioState },
};

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
  // delay(quant);
  // led_passed += quant;
  // send_passed += quant;
 Relay relay(RELAY_PIN, LOW);
  checkMotion();
  float lux=getIntCahaya();
  // Reconnect to WiFi, if needed
  if (WiFi.status() != WL_CONNECTED) {
    reconnect();
    return;
  }

  // Reconnect to ThingsBoard, if needed
  if (!tb.connected()) {
    subscribed = false;

    // Connect to the ThingsBoard
    Serial.print("Connecting MQTT to: ");
    Serial.print(THINGSBOARD_MQTT_SERVER);
    Serial.print(" with access token ");
    Serial.println(THINGSBOARD_MQTT_ACESSTOKEN);
    if (!tb.connect(THINGSBOARD_MQTT_SERVER, THINGSBOARD_MQTT_ACESSTOKEN)) {
      Serial.println("Failed to connect");
      return;
    }
  }

  // Subscribe for RPC, if needed
  if (!subscribed) {
    Serial.println("Subscribing for RPC... ");

    // Perform a subscription. All consequent data processing will happen in
    // callbacks as denoted by callbacks[] array.
    if (!tb.RPC_Subscribe(callbacks, COUNT_OF(callbacks))) {
      Serial.println("Failed to subscribe for RPC");
      return;
    }
    Serial.println("Subscribe done");
    subscribed = true;
  }

  // Check if it is a time to send DHT22 temperature and humidity
  //if (send_passed > send_delay) {
    Serial.print("Sending data... ");
    TempAndHumidity lastValues = dht.getTempAndHumidity();
    lcd_tampil (lastValues.temperature, lastValues.humidity,lux);
    if (isnan(lastValues.humidity) || isnan(lastValues.temperature) || isnan(temperature)) {
      Serial.println("Failed to read from DHT sensor!");
      setTone(3);
    }
    else {
      temperature = lastValues.temperature;
      humidity = lastValues.humidity;
      tb.sendTelemetryFloat("temperature", temperature);
      tb.sendTelemetryFloat("humidity", humidity);
    }
    tb.sendTelemetryFloat("cahaya",lux);
    //float temperature = DHT_PIN.readTemperature();  // read temperature in Celsius
    if (temperature >= TEMP_UPPER_THRESHOLD||lux > CHY_UPPER_THRESHOLD) {
      setTone(3);
      dWrite(RELAY_PIN, HIGH);
      runLed(2, true);

    } else if (temperature <= TEMP_LOWER_THRESHOLD||CHY_LOWER_THRESHOLD<lux) {
      setTone(2);
      dWrite(RELAY_PIN, HIGH);
      runLed(1, true);
    }
    else if (TEMP_LOWER_THRESHOLD <= temperature >= TEMP_UPPER_THRESHOLD) {
      setTone(1);
      dWrite(RELAY_PIN, LOW);
      runLed(0, true);
    }

   
    // wait a 2 seconds between readings
    delay(2000);
   // send_passed = 0;
  //}
  
  //lakukan penyiraman dengan menghitung waktu selama 15 detik saja
  //jika sudah selesai maka relay akan otomatis melakukan penutupan keran atau saat ini
  //hanya akan menghidupkan lampu
  DateTime time = rtc.now();
  int timeEnd = timer();
  if((timeEnd==timeStart+15)||(60-timeStart+timeEnd==15)){
    dWrite(RELAY_PIN, HIGH);
  }
  else{
    dWrite(RELAY_PIN, LOW);
  }
  Serial.println(String("DateTime::TIMESTAMP_TIME:\t")+time.timestamp(DateTime::TIMESTAMP_TIME));
   
   cetak(0, 3, "Pompa :");
   cetak(13, 3, String(statusPompa(digitalRead(RELAY_PIN))));

  //write to text for the log
  tb.loop();
}

int timer(){
  DateTime timer = rtc.now();
  return timer.second();
}


void InitWiFi()
{
  Serial.println("Connecting to AP ...");
  WiFi.begin(WIFI_AP_NAME, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(WiFi.localIP());
  Serial.println("Connected to AP");
}

void setup() {
  Serial.begin(SERIAL_DEBUG_BAUD);
  WiFi.begin(WIFI_AP_NAME, WIFI_PASSWORD);
  InitWiFi();

  if (! rtc.begin()) {
    Serial.println("RTC TIDAK TERBACA");
    while (1);
  }

  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));//update rtc dari waktu komputer
  }
  esp_wifi_set_mac(WIFI_IF_STA, &newMACAddress[0]);
  timeStart = timer();
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(PIR_PIN, INPUT);
  for (size_t i = 0; i < COUNT_OF(leds_control); ++i) {
    pinMode(leds_control[i], OUTPUT);
  }

  // Initialize temperature sensor
  dht.setup(DHT_PIN, DHTesp::DHT22);

  lcd.init();
  lcd.backlight();
  cetak(3,0,"Papan Informasi");
  cetak(2, 1,"Suhu & Kelembaban");
  cetak(3, 3,"Kelompok 1");
  delay(3000);
  lcd.clear();

  myservo.attach(23);
  for (int i = 0; i < MIN_SIZE; i++) {
    insertArray(i, arrAtr[i][0], arrAtr[i][1], arrAtr[i][2], arrAtr[i][3]);
  }
}


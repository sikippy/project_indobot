#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHTesp.h>

#define TINGGI_LAYAR 64 // Tinggi layar OLED yang digunakan
#define LEBAR_LAYAR 128 // Lebar layar OLED yang digunakan
#define MAX_SIZE 30
#define MIN_SIZE 3
const int buzzer = 25;
const int arrayLed[3]  = {12,14,27};
const int DHT_PIN = 15;
int s[3]={30,30,15};

int suhu =0;
//[row][column]
//CANNOT USE CONSTANT TO DECLARE ARRAY
const int doremi[MAX_SIZE] = {
  0x00770076, 0x0086007F, 0x0093008D, 0x009B0096, 0x00A600A1,
  0x00AC00A8, 0x00B300AF, 0x00C000B8, 0x00C400C4, 0x00C900C5, 
  0x00D400CD, 0x00DE00DC, 0x00E200DF, 0x00E200DF, 0x00DE00DC,
  0x00D400CD, 0x00C900C5, 0x00C400C4, 0x00C000B8, 0x00B300AF,
  0x00AC00A8, 0x00A600A1, 0x009B0096, 0x0093008D, 0x0086007F
  };

const int mario[MAX_SIZE]={
  0x00C900C5, 0x00C900C5, 0x00C900C5, 0x00C000B8, 0x00C900C5,
  0x00DE00DC, 0x00DE00DC, 0x00C000B8, 0x00A600A1, 0x0093008D,
  0x00AC00A8, 0x00B300AF, 0x00AC00A8, 0x00AC00A8, 0x00A600A1,
  0x00C900C5, 0x00DE00DC, 0x00E200DF, 0x00DE00DC, 0x00C900C5,
  0x00C000B8, 0x00C400C4, 0x00B300AF
  };

const int tone_alarm[30]={ 6645,6645, 6645,4186,5588,4186,6645,4186,5588,4186,6645,4186,0,0,0};

const int *arr[MAX_SIZE] = {doremi, mario,tone_alarm};
 

int arrAtr[3][4] = {{20000,0,500,30},
                 {10000,0,200,30},
                 {1000,0,500,15}};

struct Node *head;   
DHTesp dhtSensor;
Adafruit_SSD1306 oled(LEBAR_LAYAR, TINGGI_LAYAR, &Wire, -1);

struct Node{
  int index;
  int nfs;
  int ndelay;
  int nduration;
  int size;
  struct Node *next;
};


void insertArray(int index,int fs, int udelay, int dur,int s){
  struct Node* lP = (struct Node*) malloc(sizeof(struct Node));  
   lP->index = index+1; 
   lP->nfs = fs;
    lP->ndelay = udelay;
    lP->nduration = dur;
    lP->size = s;
   lP->next = head; 
   head =lP;
   pinMode(buzzer, OUTPUT);
  }



void runLed(int i){
   pinMode(arrayLed[i], OUTPUT);
   digitalWrite(arrayLed[i],HIGH);
   //delay(del[i]);
   digitalWrite(arrayLed[i],LOW);
}

void setup() {
 
  Serial.begin(9600);
  pinMode(buzzer, OUTPUT);
  // initialize OLED display with I2C address 0x3C
  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("failed to start SSD1306 OLED"));
    while (1);
  }
  setOled();
  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);
  for (int i=0;i<MIN_SIZE;i++){
  insertArray(i,arrAtr[i][0],arrAtr[i][1],arrAtr[i][2],arrAtr[i][3]);
  }
}


void oper(int suhu){
  if (suhu<29) {
        oled.print("SUHU RENDAH"); 
        runLed(0);
        setTone(3);
  }
  else if (suhu>29&suhu<35) {
        oled.print("SUHU CUKUP"); 
        runLed(1);
        setTone(2);
  }
  else if (suhu>35) {
        oled.print("SUHU PANAS"); 
        pinMode(buzzer,INPUT_PULLUP);
        runLed(2);
        setTone(1);
  }
}

const int *getArr(int mode){
switch(mode){
case 1 : return doremi; break;
case 2 : return mario; break;
case 3 : return tone_alarm;break;
}
}

void setTone(int mode){
  struct Node* ptr;
   ptr = head;

while (ptr!= NULL) { 
   Serial.println(ptr->index);
  if(ptr->index==mode){     
  for(int i = 0; i<ptr->size;i++)
    { 
      Serial.println(doremi[i]);
      Serial.println(getArr(mode)[i]);
      tone(buzzer,  getArr(mode)[i]/ptr->nfs,ptr->nduration);
      delay(ptr->ndelay);
    }
  }
   ptr = ptr->next; 
}
}

void setOled(){
  oled.clearDisplay();
  oled.setTextColor(WHITE); 
  oled.setCursor(0,11);
}

void loop() {
  setOled();
  TempAndHumidity  data = dhtSensor.getTempAndHumidity();
  oper(data.temperature);
  oled.display();
}
 

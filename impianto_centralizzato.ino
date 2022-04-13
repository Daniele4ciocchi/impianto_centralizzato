//sketch created by Daniele Quattrociocchi
#include <Wire.h> //per connessione i2c
#include <LiquidCrystal_I2C.h> //per schermo lcd
#include <IRremote.h> //per telecomando
#include <EEPROM.h> //per gestire le operazioni all'interno della memoria
#include <DHT.h>
#include <RTClib.h>

//assegnazione dei codici del telecomando ir
#define fdx 16761405
#define fsx 16720605
#define fg 16769055
#define fs 16748655
#define t0 16738455
#define t1 16724175
#define ok 16712445
#define stop 16769565
#define settings 16756815
#define on 16753245

//assegnazione dei pin utilizzati
#define DHTPIN 7
#define DHTTYPE DHT11
#define IR 11
#define RL1 2
#define RL2 3
#define RL3 4
#define RL4 5


//variabili globali

bool v[7] = {0,0,0,0,0,0,0}; // defnisce i giorni in cui la pompa deve accendersi
int om[2]; // definisce ore e minuti di accensione della pompa
int durata[4] ;    // definisce il tempo di accensione di una singola pompa
int valve = -1;    // definisce la valvola che deve essere aperta
unsigned long long time = 0,ts = -1; //variabili per la gestione del tempo di apertura delle valvole
bool lcdlight = 0;
float h,t;

LiquidCrystal_I2C lcd(0x27, 16, 2);
IRrecv receiver(IR);
decode_results results;
DHT dht(DHTPIN, DHTTYPE);
RTC_DS3231 rtc;

void dhtSensor(){
  h = dht.readHumidity();
  t = dht.readTemperature();
}

//funzione per cancellare una sola riga del display
void clearLine(int line){
  lcd.setCursor(0,line);
  lcd.print("                  ");
}

//imposta l'orario di apertura delle valvole
int impostaOra(int orario){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("imposta l'ora: ");
  lcd.setCursor(0,1);
  lcd.print(orario);
  bool valore = 0;
  while(true){
    if (receiver.decode(&results)){

      receiver.resume();
      if(results.value==fs){
        orario++;
        orario = orario%24;
        clearLine(1);
        lcd.setCursor(0,1);
        lcd.print(orario);
      }
      if(results.value==fg){
        orario--;
        if (orario == 0)orario = 1;
        orario = orario%24;
        clearLine(1);
        lcd.setCursor(0,1);
        lcd.print(orario);
      }
      if(results.value==ok)valore = 1;

    }
    if (valore) break;
  }
  return orario;
}
//imposta l'orario di apertura delle valvole
int impostaMinuti(int minuti){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("imposta i minuti: ");
  lcd.setCursor(0,1);
  lcd.print(minuti);
  bool valore = 0;
  while(true){
    if (receiver.decode(&results)){

      receiver.resume();
      if(results.value==fs){
        minuti++;
        minuti = minuti%60;
        clearLine(1);
        lcd.setCursor(0,1);
        lcd.print(minuti);
      }
      if(results.value==fg){
        minuti--;
        if (minuti == 0)minuti = 1;
        minuti = minuti%60;
        clearLine(1);
        lcd.setCursor(0,1);
        lcd.print(minuti);
      }
      if(results.value==ok)valore = 1;

    }
    if (valore) break;
  }
  return minuti;
}
//imposta la durata di apertura delle valvole
void impostaDurata(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("minuti accensione:");
  for (int i = 0;i<4;i++){
    clearLine(1);
    lcd.setCursor(0,1);
    lcd.print("valvola: ");
    lcd.print(i+1);
    lcd.print(" | ");
    lcd.print(durata[i]);
    bool valore = 0;
    while(true){
      if (receiver.decode(&results)){

        receiver.resume();
        if(results.value==fs){
          durata[i]++;
          clearLine(1);
          lcd.setCursor(0,1);
          lcd.print("valvola: ");
          lcd.print(i+1);
          lcd.print(" | ");
          lcd.print(durata[i]);
        }
        if(results.value==fg){
          durata[i]--;
          if (durata[i] == 0)durata[i] = 1;
          clearLine(1);
          lcd.setCursor(0,1);
          lcd.print("valvola: ");
          lcd.print(i+1);
          lcd.print(" | ");
          lcd.print(durata[i]);
        }
        if(results.value==ok)valore = 1;

      }
      if (valore) break;
    }
  }
}
//funzione per impostare i giorni di irrigazione
void impostaGiorni(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("lmmgvsd ");
  int indx = 0;
  bool valore = 0;

  do{
    if (receiver.decode(&results)){
      lcd.setCursor(0,1);
      for (int i=0;i<7;i++){
        if (v[i] == 0) lcd.print(" ");
        if (v[i] == 1) lcd.print("x");
      }
      lcd.setCursor(indx,1);
      if (v[indx] == 1) lcd.print(":");
      if (v[indx] == 0) lcd.print(".");
      receiver.resume();
      if(results.value==fdx)indx++;
      if(results.value==fsx)indx--;
      if(indx ==-1)indx = 0;
      if(indx == 7)indx = 6;
      if(results.value==on)valore = 1;
      if(results.value==ok){
        if (v[indx]==0)v[indx]=1;
        else if (v[indx]==1)v[indx]=0;
      }
    }
    if (valore) break;
  }while(true);
}
//funzione che da inizio al timer di irrigazione
void inizio(){
  clearLine(0);
  lcd.setCursor(0,0);
  lcd.print("apertura: ");
  ts = millis();
  valve = 0;
  clearLine(1);
  lcd.setCursor(0,1);
  lcd.print("valvola: ");
  lcd.print(valve+1);
}
//funzione che apre e chiude le valvole
//strettamente legata ad inizio()
void openValve(){


  time = millis()-ts;
  //Serial.println(millis()-ts);
  if (time > (60000*durata[valve]) && valve != -1 ){
    ts = millis();
    if (valve < 4) valve++;
    clearLine(1);
    lcd.setCursor(0,1);
    lcd.print("valvola: ");
    lcd.print(valve+1);
  }

  if (valve == 0){
    digitalWrite(RL1,HIGH);
  }
  if (valve == 1){
    digitalWrite(RL1,LOW);
    delay(100);
    digitalWrite(RL2,HIGH);
  }
  if (valve == 2){
    digitalWrite(RL2,LOW);
    delay(100);
    digitalWrite(RL3,HIGH);
  }
  if (valve == 3){
    digitalWrite(RL3,LOW);
    delay(100);
    digitalWrite(RL4,HIGH);
  }
  if (valve == 4){
    digitalWrite(RL4,LOW);
    valve = -1;
  }


}

//funzione di impostazione di:
//-orario apertura valvole
//-tempo apertura valvole
//-giorni apertura valvole
void setTime(){

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("apertura alle: ");
  lcd.setCursor(0,1);
  lcd.print(om[0]);
  lcd.print(":");
  lcd.print(om[1]);
  lcd.setCursor(7,1);
  lcd.print("corretto? s1 n0");
  bool valore = 0;
  while(true){
    if (receiver.decode(&results)){

      receiver.resume();
      if(results.value==t1) valore = 1;
      if(results.value==t0) {
        om[0] = impostaOra(om[0]);
        om[1] = impostaMinuti(om[1]);
        impostaDurata();
        impostaGiorni();
        valore = 1;
      }
    }
    if (valore) break;
  }
  EEPROM.update(0,om[0]);
  EEPROM.update(1,om[1]);
  EEPROM.update(2,durata[0]);
  EEPROM.update(3,durata[1]);
  EEPROM.update(4,durata[2]);
  EEPROM.update(5,durata[3]);
  EEPROM.update(6,v[0]);
  EEPROM.update(7,v[1]);
  EEPROM.update(8,v[2]);
  EEPROM.update(9,v[3]);
  EEPROM.update(10,v[4]);
  EEPROM.update(11,v[5]);
  EEPROM.update(12,v[6]);

}

void setup(){
  Serial.begin(9600);
  /*
  if (rtc.lostPower()){
    rtc.adjust(DateTime(F(__DATE__),F(__TIME__)));
  }
  */
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
    //rtc.adjust(DateTime(2022, 04, 06, 21, 36, 00));
    //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  dht.begin();
  Serial.begin(9600);
  receiver.enableIRIn();
  pinMode(RL1,OUTPUT);
  pinMode(RL2,OUTPUT);
  pinMode(RL3,OUTPUT);
  pinMode(RL4,OUTPUT);



	lcd.init();
	lcd.backlight();
  lcdlight = 1;
	lcd.clear();
	lcd.setCursor(0,0);
	lcd.print("");

  //caricamento dalla memoria interna dei dati di
  //irrigazione
  om[0] = EEPROM.read(0);
  om[1] = EEPROM.read(1);
  durata[0] = EEPROM.read(2);
  durata[1] = EEPROM.read(3);
  durata[2] = EEPROM.read(4);
  durata[3] = EEPROM.read(5);
  v[0] = EEPROM.read(6);
  v[1] = EEPROM.read(7);
  v[2] = EEPROM.read(8);
  v[3] = EEPROM.read(9);
  v[4] = EEPROM.read(10);
  v[5] = EEPROM.read(11);
  v[6] = EEPROM.read(12);

  for (int i =0;i<7;i++){
    Serial.println(v[i]);
  }


}

void loop(){

  DateTime now = rtc.now();

  dhtSensor();
  if (millis()%2000 == 0){
    clearLine(0);
    lcd.setCursor(0,0);
    lcd.print(now.hour());
    lcd.setCursor(2,0);
    lcd.print(":");
    lcd.print(now.minute());
    lcd.setCursor(7,0);
    lcd.print(t);
    lcd.print("C");
    if (valve == -1){
      clearLine(1);
      lcd.setCursor(0,1);
      lcd.print("apertura: ");
      lcd.setCursor(10,1);
      lcd.print(om[0]);
      lcd.setCursor(12,1);
      lcd.print(":");
      lcd.setCursor(13,1);
      lcd.print(om[1]);
    }
  }
  if(now.hour() == om[0] && now.minute() == om[1] && now.second() == (0 || 1 || 2) && valve == -1 ){
    if (v[now.dayOfTheWeek()-1] == 1){
      inizio();
    }
  }

  if (receiver.decode(&results)){

    receiver.resume();
    if(results.value==stop){
      digitalWrite(RL1,LOW);
      digitalWrite(RL2,LOW);
      digitalWrite(RL3,LOW);
      digitalWrite(RL4,LOW);
      valve = -1;
      ts = -1;

    }
    if(results.value==settings){
      setTime();
    }
    if(results.value==on){
      if (lcdlight == 1){
        lcd.noBacklight();
        lcdlight = 0;
      }else{
        lcd.backlight();
        lcdlight = 1;
      }

    }
    if(results.value==fdx) inizio();
  }


  openValve();

}

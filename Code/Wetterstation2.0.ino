#include "DHT.h"
#include <LiquidCrystal.h>
#include <IRremote.hpp>

#include <avr/sleep.h>

// LCD
#define lcdPower 8
#define rs 13
#define enable 10
#define contrast 11

// Temperatur- und Luftfeuchtigkeitssensor
#define DHTPIN 3
#define DHTTYPE DHT11

// Fotowiderstand (Lichtmessung)
#define PHOTOPIN A0

// IR Sensor
#define IRPIN 4
#define IRINTERRUPTPIN 2

const int dataPins [] = {9, 6, 5, 7};

LiquidCrystal lcd(rs, enable, dataPins[0], dataPins[1], dataPins[2], dataPins[3]);
DHT dht(DHTPIN, DHTTYPE);

volatile bool powerOn;
volatile bool returnFromStandby;

bool inLightMode;

float lastDataFetch;
volatile bool updateImmediatly;

void setup() {
  Serial.begin(9600); // Serielle Verbindung starten

  powerOn = true;
  
  pinMode(lcdPower, OUTPUT);
  digitalWrite(lcdPower, HIGH);

  lcd.begin(16,2);

  pinMode(contrast, OUTPUT);
  analogWrite(contrast, 20); // Kontrast

  dht.begin(); // DHT11 Sensor starten

  pinMode(IRPIN, INPUT);
  
  IrReceiver.begin(IRPIN, DISABLE_LED_FEEDBACK);

  lcd.print("Starte...");
}

void loop() {  
  if (returnFromStandby) {
    powerOn = true;
    digitalWrite(lcdPower, LOW);
    digitalWrite(enable, HIGH);
    lcd.begin(16,2);
    returnFromStandby = false;
    updateImmediatly = true;
    return;
  }

  if (!powerOn) {
    return;
  }

  if (millis() - lastDataFetch > 4000 || updateImmediatly) {
    updateImmediatly = false;
    
    lcd.clear();

    float t = dht.readTemperature();
    float h;
    float l;
    
    if (inLightMode)
      l = (float)400 / (float)analogRead(PHOTOPIN);
    else
      h = dht.readHumidity();

    String tempStr = String(t, 2);
    String tempOutput = String("Temp.: " + tempStr);

    lcd.setCursor(1, 0);
    lcd.print(tempOutput);

    if (!inLightMode) {
      String humStr = String(h, 2);
      String humOutput = String("Hum.:  " + humStr);

      lcd.setCursor(1, 1);
      lcd.print(humOutput);
    } else {
      String lightStr = String(l, 2);
      String lightOutput = String("Light:  " + lightStr);

      lcd.setCursor(1, 1);
      lcd.print(lightOutput);
    }

    lastDataFetch = millis();
  }

  //Serial.println(digitalRead(IRPIN));
  if (IrReceiver.decode()) {    

    int data = IrReceiver.decodedIRData.command;
    Serial.println(data);

    if (data == 69) { // On / Off (nice)
      if (powerOn) {
        // In den Standby gehen
        powerOn = false;
        // LCD
        digitalWrite(lcdPower, LOW);
        digitalWrite(rs, LOW); // Wichtig
        digitalWrite(enable, LOW);
        lcd.noDisplay();
        // Sleep mit Interrupt
        delay(1000);
        attachInterrupt(digitalPinToInterrupt(IRINTERRUPTPIN), wakeup_isr, RISING);
        delay(500);
        set_sleep_mode(SLEEP_MODE_PWR_DOWN);
        sleep_mode();
        IrReceiver.resume();  
        return;
      }
    } else if (data == 24) { // 2
      inLightMode = true;
    } else if (data == 12) { // 1
      inLightMode = false;
    } else if (data == 68 || data == 67) { // Arrows
      inLightMode = !inLightMode;
    }

    updateImmediatly = true;

    IrReceiver.resume();  
  }

  //Serial.println(output);

  //Serial.println("Temp: "); Serial.println(t);
  
  //Serial.println("Licht: "); Serial.println(l);
}

void wakeup_isr() {
  detachInterrupt(IRINTERRUPTPIN); // Interrupt deaktivieren

  returnFromStandby = true;
}

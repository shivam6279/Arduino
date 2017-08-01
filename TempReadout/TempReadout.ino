#include <LiquidCrystal.h>
#include <DHT.h>
#include <DallasTemperature.h>
#include <OneWire.h>

#define DHTPIN 9    
#define DHTTYPE DHT22 
DHT dht(DHTPIN, DHTTYPE);

OneWire oneWire(10);
DallasTemperature sensors(&oneWire);

LiquidCrystal lcd(12,11,5,4,3,2);

void setup() {
  lcd.begin(16, 2);
  dht.begin();
  sensors.begin();
  lcd.setCursor(0, 0);
  lcd.println("T1    H1    T2  ");
}

void loop() {
  float t1, t2, h;
  t1 = dht.readTemperature();
  delay(200);
  h = dht.readHumidity();
  sensors.requestTemperatures();
  t2 = sensors.getTempCByIndex(0);
  lcd.setCursor(0, 1);
  lcd.print(t1);
  lcd.print(',');
  lcd.print(h);
  lcd.print(',');
  lcd.print(t2);
}

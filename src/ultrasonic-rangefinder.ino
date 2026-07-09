#include <BigFont01_I2C.h>
#include <NewPing.h>
#include <EncButton.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h> 

// Сонар
#define ECHO 2
#define TRIG 3
#define SVCC 4

// Дисплей
#define SCL A5
#define SDA A4


// Переключатель
#define BUTTIN 11
#define BUTTOUT 12

// Энкодер
#define ECLK 5
#define EDT 6
#define ESW 7
#define EPLUS 8 
#define EGND 9


NewPing sonar(TRIG, ECHO, 400);
LiquidCrystal_I2C lcd(0x27,20,4);
EncButton eb(ECLK, EDT, ESW);
BigFont01_I2C big(&lcd);

unsigned long senseTimer = 0;
int i = 0;
int dist_arr[3] = {0, 0, 0}; 
int dist;
int case_size = 12;
int measurments[8] = {-1,-1,-1,-1,-1,-1,-1,-1};
int enc = 1;
int str;
int line;




void setup() {

  pinMode(SVCC, OUTPUT);
  pinMode(BUTTIN, INPUT_PULLUP);
  pinMode(BUTTOUT, OUTPUT);
  pinMode(EGND, OUTPUT);
  pinMode(EPLUS, OUTPUT);

  digitalWrite(SVCC, HIGH);
  digitalWrite(BUTTOUT, LOW);
  digitalWrite(EGND, LOW);
  digitalWrite(EPLUS, HIGH);

  lcd.init();                     
  lcd.backlight();

  big.begin();

  // Загрузка сохраненных значений при старте
  for(int i = 0; i < 8; i++) {
    EEPROM.get(i * sizeof(int), measurments[i]);
    if(measurments[i] < -1 || measurments[i] > 400) measurments[i] = -1;
  }

}


void loop() {
  eb.tick();

  bool changed = false;

  if (eb.turn()) {
    changed = true;
    if (eb.left()) {
      enc += 1;
      if (enc >= 8) {
        enc = 8;
      }
    }
    if (eb.right()) {
      enc -= 1;
      if (enc <= 1) {
        enc = 1;
      }
    }
  }

  if (enc <= 4) {
    str = 8;
    line = enc - 1;
  } else {
    str = 18;
    line = enc - 5;
  }

  if (eb.click()) {
    measurments[enc - 1] = distance();
    // Сохранение в EEPROM
    for(int i = 0; i < 8; i++) {
      EEPROM.put(i * sizeof(int), measurments[i]);
    }
    changed = true;
  }

  // Долгое нажатие - стереть выбранное значение
  if (eb.hold()) {
    measurments[enc - 1] = -1;
    for(int i = 0; i < 8; i++) {
      EEPROM.put(i * sizeof(int), measurments[i]);
    }
    changed = true;
  }

  // Двойной клик - стереть все значения
  if (eb.hasClicks(2)) {
    for(int i = 0; i < 8; i++) {
      measurments[i] = -1;
    }
    for(int i = 0; i < 8; i++) {
      EEPROM.put(i * sizeof(int), measurments[i]);
    }
    changed = true;
  }

  static bool first = true;
  if (changed || first) {
    first = false;
    lcd.clear();
  

    lcd.setCursor(str, line);
    lcd.print("<");
    for (int j = 1; j <= 4; j++) {
      if (measurments[j - 1] == -1) {
        lcd.setCursor(0, j - 1);
        lcd.print(j); lcd.print(")"); lcd.print("---");
      } else {
        lcd.setCursor(0, j - 1);
        lcd.print(j); lcd.print(")"); lcd.print(measurments[j - 1]); lcd.print("cm");      
      }
    }

    for (int j = 5; j <= 8; j++) {
      if (measurments[j - 1] == -1) {
        lcd.setCursor(10, j - 5);
        lcd.print(j); lcd.print(")"); lcd.print("---");
      } else {
        lcd.setCursor(10, j - 5);
        lcd.print(j); lcd.print(")"); lcd.print(measurments[j - 1]); lcd.print("cm");
      }
    }
  }

}

int median(int val1, int val2, int val3) {
  int arr[3] = {val1, val2, val3};
  for (int j = 0; j < 2; j++) {
    for (int i = 0; i < 2; i++) {
      if (arr[i] > arr[i + 1]) {
        int buf = arr[i];
        arr[i] = arr[i + 1];
        arr[i + 1] = buf;
      }
    }
  }
  return arr[1];
}

int distance() {
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("Current distance");
  lcd.setCursor(5, 3);
  lcd.print("centimeter");

  int lastdist = -1;

  while (true) {
    eb.tick();

    if (eb.click()) {
      lcd.clear();
      return dist;
    }
     if (millis() - senseTimer > 100) {

      if (i > 1) {
        i = 0;
      } else {
        i++;
      }
      dist_arr[i] = (int)sonar.ping_cm(400); 
      if (digitalRead(BUTTIN) == LOW) {
        dist_arr[i] += case_size;
      }
      dist = median(dist_arr[0], dist_arr[1], dist_arr[2]);
      if (dist != lastdist) {
      lcd.setCursor(0, 1); lcd.print("                    ");
      lcd.setCursor(0, 2); lcd.print("                    ");
        if (dist < 10) {
          big.writeint(1, 9, dist, 1, true);
        } else if (dist >= 10 && dist <= 99) {
          big.writeint(1, 7, dist, 2, true);
        } else {
          big.writeint(1, 5, dist, 3, true);
        }
        lastdist = dist;
      }
        senseTimer = millis();
     }
  }
}
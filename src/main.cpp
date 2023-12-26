#include <Arduino.h>

#define BTN_PIN 4        // кнопка подключена сюда (BTN_PIN --- КНОПКА --- GND)

#include "GyverButton.h"
GButton touch(BTN_PIN);

#define STRIP_PIN 13     // пин ленты
#define NUMLEDS 110      // кол-во светодиодов

#define COLOR_DEBTH 3
#include <microLED.h>   // подключаем библу
microLED<NUMLEDS, STRIP_PIN, MLED_NO_CLOCK, LED_WS2818, ORDER_GRB, CLI_AVER> strip;

#include "GyverTimer.h"
GTimer_ms effectTimer(60);

// Статус лампы вкл/выкл
boolean powerActive = false;

//Переменные для регулирования яркоси
boolean wasStep = false;
boolean brightDirection = false;

byte brightness = 150;
byte static_color = 40;

byte next_effect = 0;

// Переменные вкл/выкл лампы
int8_t num_leds_effect[] = {44, -1, 99, 42, 100};

void setup() {
  strip.setBrightness(150);
  strip.clear();
  strip.show();
  Serial.begin(9600);

  // HIGH_PULL - кнопка подключена к GND, пин подтянут к VCC (BTN_PIN --- КНОПКА --- GND)
  // LOW_PULL  - кнопка подключена к VCC, пин подтянут к GND
  // по умолчанию стоит HIGH_PULL
  touch.setType(LOW_PULL);

  // NORM_OPEN - нормально-разомкнутая кнопка
  // NORM_CLOSE - нормально-замкнутая кнопка
  // по умолчанию стоит NORM_OPEN
  touch.setDirection(NORM_OPEN);

  touch.setTimeout(300);
  touch.setStepTimeout(50);
  effectTimer.setInterval(60);
}

//Статичный цвет
void static_full_fill(){
  for (byte i=0; i < NUMLEDS; i++) strip.set(i, mWheel8(static_color));
}

// Радуга
void rainbow() {
  static byte counter = 0;
  for (int i = 0; i < NUMLEDS; i++) {
    strip.set(i, mWheel8(counter + i * 255 / NUMLEDS));   // counter смещает цвет
  }
  counter += 3;   // counter имеет тип byte и при достижении 255 сбросится в 0
}

// Плавная смена цветов
void colorCycle() {
  static byte counter = 0;
  strip.fill(mWheel8(counter));
  counter += 3;
}

//Эффект включения лампы
bool effect_on(){

  for (byte i = 0; i < NUMLEDS; i++) {
// НИЗ
    if (num_leds_effect[0] > 17) strip.set(num_leds_effect[0]-=1, mWheel8(static_color));    
    if (num_leds_effect[2] < 110) strip.set(num_leds_effect[2]+=1, mWheel8(static_color));
    if (num_leds_effect[2] == 110 && num_leds_effect[1] < 17) strip.set(num_leds_effect[1]+=1, mWheel8(static_color));

// // ВЕРХ
     if (num_leds_effect[3] < 73) strip.set(num_leds_effect[3]+=1, mWheel8(static_color));
     if (num_leds_effect[4] > 72) strip.set(num_leds_effect[4]-=1, mWheel8(static_color));
    strip.show();
    delay(30);
  }
  powerActive = true;
  return powerActive;
} 

void effect_off(){
  for (byte i = 0; i < NUMLEDS; i++) {
// НИЗ
    if (num_leds_effect[0] < 44) strip.set(num_leds_effect[0]+=1, mBlack);
    if (num_leds_effect[1]) strip.set(num_leds_effect[1]-=1, mBlack);     
    if (num_leds_effect[1] == 0 && num_leds_effect[2] > 99) strip.set(num_leds_effect[2]-=1, mBlack);

// ВЕРХ
    if (num_leds_effect[3] > 42) strip.set(num_leds_effect[3]-=1, mBlack);
    if (num_leds_effect[4] < 100) strip.set(num_leds_effect[4]+=1, mBlack);
    strip.show();
    delay(30);
  }
  strip.clear();
  strip.show();
  // Для того что бы зажигался 1-й диод
  num_leds_effect[1] = -1;  
}

//Регулирование яркости
void change_brightness(){
  if (powerActive) {
    wasStep = true;
    if (brightDirection) {
      brightness += 5;
      Serial.println(brightness);
    } else {
      brightness -= 5;
      Serial.println(brightness);
      }
    strip.setBrightness(brightness);
    strip.show();
    }

    if (touch.isRelease()) {
      if (wasStep) {
        wasStep = false;
        brightDirection = !brightDirection;
      }
    }
}

// Изменяем цвет
void change_color(){
  if (powerActive) {
    wasStep = true;
    if (brightDirection) {
      static_color += 2;
    } else {
      static_color -= 2;
      }
    for (byte i=0; i < NUMLEDS; i++) strip.set(i, mWheel8(static_color));
    strip.show();
    }

    if (touch.isRelease()) {
      if (wasStep) {
        wasStep = false;
        brightDirection = !brightDirection;
      }
    }
}

void loop() {

// Опрос кнопки
  touch.tick(); 

//  Одинарный клик Вкл/выкл подстветку
  if (touch.isSingle()) {
    if (powerActive == false) {
      // Первое вклчение лампы
        effectTimer.start();
        powerActive = true;
        effect_on();
      }else if (powerActive == true)
      {
        effect_off();
        powerActive = false;
        effectTimer.stop();
      }
    }

  //Двойной клик, следующий эфект
  if (touch.isDouble()) {
    if (next_effect < 2) {
      next_effect += 1;
    }
    else {
      next_effect = 0;
    }
    Serial.println(next_effect);
  }

  //Тройной клик, предыдующий эфект
  if (touch.isTriple()){
    if (next_effect > 0) {
      next_effect -= 1;
      }
    else {
      next_effect = 2;
    }
    Serial.println(next_effect);
  }

// Смена яркости ленты
if (touch.isStep()) {
    effectTimer.setInterval(360);
    change_brightness();
  }

// Смена цвета
if (touch.isStep(1)) {
  if (next_effect==0){
    effectTimer.setInterval(360);
    change_color();
  }
}

 if (effectTimer.isReady() && powerActive) {
    switch (next_effect){
    case 0:
      static_full_fill();
      break;
    case 1:
      effectTimer.setInterval(800);
      colorCycle();
      break;
    case 2: 
      effectTimer.setInterval(60);
      rainbow();
      break;
    default:
      break;
    }
    strip.show();
 }
}


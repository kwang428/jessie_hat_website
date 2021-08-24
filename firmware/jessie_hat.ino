#include <Servo.h>
#include <FastLED.h>

#define NUM_LEDS 60
#define DATA_PIN 6 // LED
#define TOP_INT_PIN 2
#define BOT_INT_PIN 3
#define COIL_PIN 4
#define STIRAP_PIN 5
#define RESET_PIN 7
#define SERVO_PIN 9

int refresh_t = 25;

CRGB leds[NUM_LEDS];
int num_on = 8;
int LED_idx = 0;

Servo myservo;
int pos = 0; // pos 0, full speed one way, pos 90, stop, pos 180, full speed other way
int up = 110; // position for going up, counter clockwise > 90
int stop_move = 90;
int down = 70; // position for going down
int motor_pos = 2; // 0 if on bottom, 1 if on top, otherwise unknown

int COIL_STATE = 0;
int STIRAP_STATE = 0;
int RESET_STATE = 0;

unsigned long first_coil = 0;

bool reset_in_progress = false;
bool stirap_ready = false;
bool stirap_in_progress = false;
bool initial_coil = true;

void setup() {
  Serial.begin(9600);
  Serial.println("Hello");
  // put your setup code here, to run once:
  myservo.attach(SERVO_PIN);
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  pinMode(TOP_INT_PIN, INPUT_PULLUP);
  pinMode(BOT_INT_PIN, INPUT_PULLUP);
  pinMode(COIL_PIN, INPUT_PULLUP);
  pinMode(STIRAP_PIN, INPUT_PULLUP);
  pinMode(RESET_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(TOP_INT_PIN), top_hit, FALLING);
  attachInterrupt(digitalPinToInterrupt(BOT_INT_PIN), bot_hit, FALLING);
  myservo.write(90);
  //LED_off();
}

void loop() {
  // put your main code here, to run repeatedly:
  //myservo.write(180);
  //delay(1000);
  //myservo.write(90);
  //delay(1000);
  //myservo.write(100);
  if (true) {
  RESET_STATE = !digitalRead(RESET_PIN);
  if (RESET_STATE) {
    reset_in_progress = true;
    reset(true);
    delay(refresh_t);
    return;
  }
  int coil_now = !digitalRead(COIL_PIN);
  if (coil_now != COIL_STATE) {
    // edge detection
    if (coil_now) {
      first_coil = millis(); 
      stirap_ready = true;
    }
    else {
      reset_in_progress = true;
      reset(true);
    }
  }
  COIL_STATE = coil_now;
  if (COIL_STATE) {
    if (!stirap_in_progress) {
      unsigned long time_now = millis();
      if (time_now - first_coil > 15 * 1000) {
        // longer than 10 seconds
        //Serial.println("too long");
        stirap_ready = false;
        LED_err();
        reset_in_progress = true;
        reset(false);
      }
      else {
        //Serial.println("waiting for stirap");
        if (motor_pos == 0) {
          motor_pos = 2;
          myservo.write(up);
        }
        else if (motor_pos == 1) {
          motor_pos = 2;
          myservo.write(down);
        }
        else if (initial_coil) {
          initial_coil = false;
          myservo.write(up);
        }
        LED_step(LED_idx);
        LED_idx++;
        if (LED_idx >= NUM_LEDS) {
          LED_idx = 0;
        }
      }
    }
  }
  int STIRAP_STATE = !digitalRead(STIRAP_PIN);
  if (STIRAP_STATE && stirap_ready) {
    LED_grn();
    if (motor_pos != 0) {
      myservo.write(down);
    }
    stirap_in_progress = true;
  }
  delay(refresh_t);
  }
}

void LED_step(int i) {
  while (i < 0) {
    i += NUM_LEDS;
  }
  i = i % NUM_LEDS;
  int idxs [num_on];
  for (int j = 0; j < num_on; j++) {
    if (i - j < 0) {
      idxs[j] = (i - j) + NUM_LEDS;
    }
    else {
      idxs[j] = i - j;
    }
    leds[idxs[j]].r = 200 * (num_on - j) * (num_on - j)/(num_on * num_on);
  }
  int black_idx;
  if (i - num_on < 0) {
    black_idx = i - num_on + NUM_LEDS;
  }
  else {
    black_idx = i - num_on;
  }
  leds[black_idx] = CRGB::Black;
  FastLED.show();
}

void LED_off() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Black;
  }
  FastLED.show();
}

void LED_err() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i].r = 200;
  }
  FastLED.show();
}

void LED_grn() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i].g = 200;
  }
  FastLED.show();
}

void bot_hit() {
  Serial.println("hit bot");
  if (stirap_in_progress) {
    myservo.write(stop_move);
    motor_pos = 0;
  }
  else if (COIL_STATE && motor_pos != 0) {
    myservo.write(up);
    motor_pos = 2;
  }
}

void top_hit() {
  Serial.println("hit top");
  if (reset_in_progress) {
    myservo.write(stop_move);
    reset_in_progress = false;
    motor_pos = 1;
  }
  else if (COIL_STATE && motor_pos != 1) {
    myservo.write(down);
    motor_pos = 2;
  }
}

void reset(bool led) {
  stirap_in_progress = false;
  if (led) {
    LED_off();
  }
  if (motor_pos != 1) {
    myservo.write(up);
  }
}

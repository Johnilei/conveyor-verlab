/*
 * - Position 0: Motor OFF, Green LED OFF.
 * - Position 1: Speed 2, Green LED ON.
 * - Position 2: Speed 4, Green LED ON.
 * - Position 3: Speed 6, Green LED ON.
 * - Position 4: Speed 8, Green LED ON.
 * Motor desliga se ficar mais de 1 segundo sem estar em nenhuma posição.
 */

#include <Arduino.h>

int RPWM = 6;
int LPWM = 7;
int L_EN = 51;
int R_EN = 50;

int GREEN_LED = 52;
int RED_LED = 53;

const int POS1_PIN = 22;
const int POS2_PIN = 24;
const int POS3_PIN = 26;
const int POS4_PIN = 28;

const int SPEED_MOTOR_OFF = 0;
const int SPEED_2 = 64;
const int SPEED_4 = 128;
const int SPEED_6 = 191;
const int SPEED_8 = 255;

int target_speed = SPEED_MOTOR_OFF; 
unsigned long lastActiveTime = 0;   

void setup() {
  pinMode(RPWM, OUTPUT);
  pinMode(LPWM, OUTPUT);
  pinMode(L_EN, OUTPUT);
  pinMode(R_EN, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);

  pinMode(POS1_PIN, INPUT_PULLUP);
  pinMode(POS2_PIN, INPUT_PULLUP);
  pinMode(POS3_PIN, INPUT_PULLUP);
  pinMode(POS4_PIN, INPUT_PULLUP);

  Serial.begin(9600);

  digitalWrite(R_EN, HIGH);
  digitalWrite(L_EN, HIGH);

  digitalWrite(RED_LED, HIGH);
  digitalWrite(GREEN_LED, HIGH); 
  analogWrite(RPWM, SPEED_MOTOR_OFF);
  analogWrite(LPWM, SPEED_MOTOR_OFF);
  
  lastActiveTime = millis();
  
  delay(1000);
}

void loop() {
  bool pos1isActive = (digitalRead(POS1_PIN) == LOW);
  bool pos2isActive = (digitalRead(POS2_PIN) == LOW);
  bool pos3isActive = (digitalRead(POS3_PIN) == LOW);
  bool pos4isActive = (digitalRead(POS4_PIN) == LOW);

  if (pos1isActive) {
    target_speed = SPEED_2;
    lastActiveTime = millis(); 
  } else if (pos2isActive) {
    target_speed = SPEED_4;
    lastActiveTime = millis(); 
  } else if (pos3isActive) {
    target_speed = SPEED_6;
    lastActiveTime = millis(); 
  } else if (pos4isActive) {
    target_speed = SPEED_8;
    lastActiveTime = millis(); 
  } else {
    if (millis() - lastActiveTime > 1000) {
      target_speed = SPEED_MOTOR_OFF;

    }
  }

  if (target_speed > 0) {
    digitalWrite(RED_LED, HIGH); 
    digitalWrite(GREEN_LED, LOW);     
    analogWrite(LPWM, 0);             
    analogWrite(RPWM, target_speed);  
  } else {
    digitalWrite(RED_LED, HIGH);
    digitalWrite(GREEN_LED, HIGH);    
    analogWrite(RPWM, 0);
    analogWrite(LPWM, 0);             
  }

  delay(50);
}

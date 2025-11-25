#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

// ===================== KONFIGURACJA L298N =====================
// PODSTAW SWOJE PINY ESP32:
const int IN1 = 5;   // kierunek silnik A
const int IN2 = 6;   // kierunek silnik A
const int ENA = 18;   // PWM silnik A

const int IN3 = 7;   // kierunek silnik B
const int IN4 = 8;   // kierunek silnik B
const int ENB = 21;   // PWM silnik B

// ===================== KLASA ROBOTDRIVE =====================

class RobotDrive {
public:
  RobotDrive(int in1, int in2, int ena,
             int in3, int in4, int enb)
    : _in1(in1), _in2(in2), _ena(ena),
      _in3(in3), _in4(in4), _enb(enb) {}

  void begin() {
    pinMode(_in1, OUTPUT);
    pinMode(_in2, OUTPUT);
    pinMode(_ena, OUTPUT);

    pinMode(_in3, OUTPUT);
    pinMode(_in4, OUTPUT);
    pinMode(_enb, OUTPUT);

    stop();
  }

  // Sterowanie jazdą na podstawie osi X (0–1023)
  void setSpeedFromJoystick(int xVal) {
    const int JOY_MIN    = 0;
    const int JOY_MAX    = 1023;
    const int JOY_CENTER = 512;
    const int DEADZONE   = 80;   // martwa strefa wokół środka

    int delta = xVal - JOY_CENTER;

    if (abs(delta) < DEADZONE) {
      stop();
      return;
    }

    int speed;

    if (delta > 0) {
      // Do przodu
      speed = map(delta, DEADZONE, JOY_MAX - JOY_CENTER, 0, 255);
      forward(speed);
    } else {
      // Do tyłu
      delta = -delta;
      speed = map(delta, DEADZONE, JOY_CENTER - JOY_MIN, 0, 255);
      backward(speed);
    }
  }

  void stop() {
    digitalWrite(_in1, LOW);
    digitalWrite(_in2, LOW);
    digitalWrite(_in3, LOW);
    digitalWrite(_in4, LOW);
    analogWrite(_ena, 0);
    analogWrite(_enb, 0);
  }

private:
  int _in1, _in2, _ena;
  int _in3, _in4, _enb;

  void forward(int speed) {
    digitalWrite(_in1, HIGH);
    digitalWrite(_in2, LOW);
    digitalWrite(_in3, HIGH);
    digitalWrite(_in4, LOW);

    speed = constrain(speed, 0, 255);
    analogWrite(_ena, speed);
    analogWrite(_enb, speed);
  }

  void backward(int speed) {
    digitalWrite(_in1, LOW);
    digitalWrite(_in2, HIGH);
    digitalWrite(_in3, LOW);
    digitalWrite(_in4, HIGH);

    speed = constrain(speed, 0, 255);
    analogWrite(_ena, speed);
    analogWrite(_enb, speed);
  }
};

// Tworzymy obiekt sterujący robotem (globalnie, żeby był widoczny w callbacku)
RobotDrive robot(IN1, IN2, ENA, IN3, IN4, ENB);

// ===================== TWÓJ KOD ESP-NOW =====================

enum manager_state{
  STANDBY = 0,
  MOVING  = 1,
  SCANNING= 2,
  UPLOADING = 3
};

struct message{
  int x,y;
  bool start, select, x_b, y_b, b_b, a_b;
  manager_state state;
};

message rx_message;

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len){
  memcpy(&rx_message, incomingData,len);

  Serial.print("Joystick: X: ");
  Serial.print(rx_message.x);
  Serial.print(" | Y: ");
  Serial.println(rx_message.y);

  // Prosta logika:
  // - jeśli stan to MOVING -> jedź wg joysticka
  // - w innym stanie -> zatrzymaj robota
  if (rx_message.state == MOVING) {
    robot.setSpeedFromJoystick(rx_message.x);
  } else {
    robot.stop();
  }

  // Jeśli chcesz, możesz odkomentować debug stanu:
  /*
  String state_str;
  switch(rx_message.state)
  {
    case STANDBY:   state_str = "STANDBY"; break;
    case MOVING:    state_str = "MOVING"; break;
    case SCANNING:  state_str = "SCANNING"; break;
    case UPLOADING: state_str = "UPLOADING"; break;
  }
  Serial.println(state_str);
  */
}

void setup() {
  Serial.begin(115200);
  delay(100);

  // Start sterownika silników
  robot.begin();

  // ESP-NOW
  WiFi.mode(WIFI_STA);

  if(esp_now_init() != ESP_OK){
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
  Serial.println("The receiver has been initiated");
}

void loop() {
  // Główna pętla może być pusta – sterowanie jest w callbacku ESP-NOW
  delay(100);
}

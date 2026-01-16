/// @file ArcadeDrive.cpp
/// @brief Provides definitions for functions from ArcadeDrive.h.

#include "ArcadeDrive.h"

int timer2 = 0;

namespace Engines{

  // ------------------------------------------------ //
  // ----------------- SILNIK CLASS ----------------- //
  // ------------------------------------------------ //

  // Konstruktor
  Silnik::Silnik(int ena, int in1, int in2, int ch) {
    pinENA = ena;
    pinIN1 = in1;
    pinIN2 = in2;
    channel = ch;

  }

  // Inicjalizacja 
  void Silnik::begin() {
    pinMode(pinIN1, OUTPUT);
    pinMode(pinIN2, OUTPUT);
    
    // Konfiguracja PWM (LEDC) dla ESP32
    // 20kHz usuwa piszczenie silników
    //ledcAttach(pinENA, 20000, 8);
    pinMode(pinENA, OUTPUT);

    // Old code below ->
    //ledcSetup(channel, 20000, 8); // (channel, freq, resolution)
    //ledcAttachPin(pinENA, channel); // (pin - which pin is used, channel)
  }

  // Ustawienie celu - tę funkcję wywołuje Joystick/Mixer
  // Zakres wejściowy: -255 (tył) do 255 (przód)
  void Silnik::setTargetSpeed(int speed) {
    targetSpeed = constrain(speed, -255, 255);
  }

  // Główna funkcja aktualizująca (wywoływać w loop non-stop)
  void Silnik::update() {
    unsigned long now = millis();
    
    // Sprawdzamy, czy nadszedł czas na kolejny krok zmiany prędkości
    if (now - lastUpdate < rampTime) 
      return;
    
    lastUpdate = now;

    // --- 1. Logika Ramping (Soft Start / Soft Stop) ---
    if (currentSpeed < targetSpeed) {
      currentSpeed += rampStep;
      if (currentSpeed > targetSpeed) currentSpeed = targetSpeed;
    } 
    else if (currentSpeed > targetSpeed) {
      currentSpeed -= rampStep;
      if (currentSpeed < targetSpeed) currentSpeed = targetSpeed;
    }

    // --- 2. Sterowanie sprzętowe ---
    int pwmOutput = (int)abs(currentSpeed); // PWM musi być dodatnie

    if (currentSpeed > 0) {
      // Jazda w przód
      digitalWrite(pinIN1, HIGH);
      digitalWrite(pinIN2, LOW);
    } else if (currentSpeed < 0) {
      // Jazda w tył
      digitalWrite(pinIN1, LOW);
      digitalWrite(pinIN2, HIGH);
    } else {
      // Stop (hamowanie swobodne)
      digitalWrite(pinIN1, LOW);
      digitalWrite(pinIN2, LOW);
    }

    // Wysłanie sygnału PWM
    //ledcWrite(channel, pwmOutput);
    //ledcWrite(pinENA, pwmOutput);

    analogWrite(pinENA, pwmOutput);
  }
  
  void Silnik::stop(){
    //ledcWrite(channel, 0);
    analogWrite(pinENA, 0);
    targetSpeed = 0;
    

    digitalWrite(pinIN1, LOW);
    digitalWrite(pinIN2, LOW);

  }

  // ------------------------------------------------ //
  // ------------- DOUBLE ENGINE CLASS -------------- //
  // ------------------------------------------------ //

  // ------------------ Constructors ------------------ //

  DoubleEngine::DoubleEngine(uint8_t l_in1, uint8_t l_in2, uint8_t l_ena, uint8_t r_in1, uint8_t r_in2, uint8_t r_enb) : 
                motorLeft(l_ena, l_in1, l_in2, 0),
                motorRight(r_enb, r_in1, r_in2, 1),
                leftSpeed(0), rightSpeed(0),
                deadZone(JOYSTICK_DEADZONE) {}

  DoubleEngine::~DoubleEngine() {}

  // ------------------ Initialization ------------------ //

  void DoubleEngine::initEngines(){
    motorLeft.begin();
    motorRight.begin();
  }

  // ------------------ Steering ------------------ //

  void DoubleEngine::stop(){
    motorLeft.stop();
    motorRight.stop();
  }

  void DoubleEngine::applySteering(int xx, int yy){
    //Transformacja wejscia z joysticka na predkosc/napiecie

    int throttle = map(yy, 0, 1023, -255, 255);
    int steering = map(xx, 0, 1023, -255, 255);

    // Aplikacja Martwej Strefy (Deadzone)
    if (abs(throttle) < deadZone) throttle = 0;
    if (abs(steering) < deadZone) steering = 0;

    // MIXER RÓŻNICOWY (Arcade Drive)
    leftSpeed  = throttle + steering;
    rightSpeed = throttle - steering;
    leftSpeed  = constrain(leftSpeed, -255, 255);
    rightSpeed = constrain(rightSpeed, -255, 255);
  }

  void DoubleEngine::update(){
    motorLeft.setTargetSpeed(leftSpeed);
    motorRight.setTargetSpeed(rightSpeed);

    // Update the engines
    motorLeft.update();
    motorRight.update();
  }

  // ------------------ Getters ------------------ //

  int DoubleEngine::getLeftSpeed(){
    return this->leftSpeed;
  }

  int DoubleEngine::getRightSpeed(){  
    return this->rightSpeed;
  }

};


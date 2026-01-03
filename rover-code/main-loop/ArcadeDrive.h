#ifndef SILNIK_H
#define SILNIK_H

#include <Arduino.h>

#define JOYSTICK_DEADZONE 20

class Silnik {
  private:
    // Piny fizyczne (zgodne z L298N/TB6612)
    int pinENA; // PWM (Enable)
    int pinIN1; // Kierunek A
    int pinIN2; // Kierunek B
    int channel; // Kanał PWM (ESP32 ma 16 kanałów, 0-15)

    // Zmienne Soft Start (Ramping)
    float currentSpeed = 0;   // Aktualna prędkość chwilowa (float dla precyzji)
    int targetSpeed = 0;      // Prędkość docelowa (-255 do 255)
    
    unsigned long lastUpdate = 0;
    
    // --- KONFIGURACJA PŁYNNOŚCI ---
    // Im mniejszy rampStep, tym wolniej silnik reaguje (większa bezwładność)
    // Im mniejszy rampTime, tym częściej aktualizujemy prędkość
    const int rampTime = 5;      // [ms] Czas między krokami zmiany prędkości
    const float rampStep = 4.0;  // O ile zmieniamy PWM w jednym kroku (0-255)
    
  public:
    // Konstruktor
    Silnik(int ena, int in1, int in2, int ch) {
      pinENA = ena;
      pinIN1 = in1;
      pinIN2 = in2;
      channel = ch;
    }

    // Inicjalizacja 
    void begin() {
      pinMode(pinIN1, OUTPUT);
      pinMode(pinIN2, OUTPUT);
      
      // Konfiguracja PWM (LEDC) dla ESP32
      // 20kHz usuwa piszczenie silników
      ledcAttach(pinENA, 20000, 8);


      // Old code below ->
      //ledcSetup(channel, 20000, 8); // (channel, freq, resolution)
      //ledcAttachPin(pinENA, channel); // (pin - which pin is used, channel)
    }

    // Ustawienie celu - tę funkcję wywołuje Joystick/Mixer
    // Zakres wejściowy: -255 (tył) do 255 (przód)
    void setTargetSpeed(int speed) {
      targetSpeed = constrain(speed, -255, 255);
    }

    // Główna funkcja aktualizująca (wywoływać w loop non-stop)
    void update() {
      unsigned long now = millis();
      
      // Sprawdzamy, czy nadszedł czas na kolejny krok zmiany prędkości
      if (now - lastUpdate < rampTime) return;
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
      ledcWrite(channel, pwmOutput);
    }
    
    void stop(){
      ledcWrite(channel, 0);
      digitalWrite(pinIN1, LOW);
      digitalWrite(pinIN2, LOW);

    }
};

/// @brief Controls two engines of class Silnik, both should be powered from L298N bridge.
class DoubleEngine{
private:
  const int deadZone; ///< Specifies the range around joystick 'zero' which is ingored, so minor fluctuations of pwm value ar not consdiered.

  Silnik motorLeft;  ///< The left motor controller.
  Silnik motorRight; ///< The right motor controller.

  int leftSpeed, rightSpeed; ///< Speed of left and right motor. 

public:

  // ------------------ Constructors ------------------ //

  /// @brief Initilizes the egines controller. 
  /// @param l_in1 
  /// @param l_in2 
  /// @param l_ena 
  /// @param r_in1 
  /// @param r_in2 
  /// @param r_enb 
  DoubleEngine(uint8_t l_in1, uint8_t l_in2, uint8_t l_ena, uint8_t r_in1, uint8_t r_in2, uint8_t r_enb) : 
                motorLeft(l_in1, l_in2, l_ena, 0),
                motorRight(r_in1, r_in2, r_enb, 1),
                leftSpeed(0), rightSpeed(0),
                deadZone(JOYSTICK_DEADZONE) {}

  /// @brief Does nothing
  ~DoubleEngine() {}

  // ------------------ Initialization ------------------ //

  /// @brief Initlizes both engines. 
  void initEngines(){
    motorLeft.begin();
    motorRight.begin();
  }

  // ------------------ Steering ------------------ //

  /// @brief Stops both engines
  void stop(){
    motorLeft.stop();
    motorRight.stop();
  }

  /// @brief Applies sttering values and adjusts engines rotation speed and direction.
  /// @param xx Pwm value in range 0-1023. (Should be taken from joystick).
  /// @param yy Pwm value in range 0-1023. (Should be taken from joystick).
  void applySteering(int xx, int yy){
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

  /// @brief Causes the engines to apply roation.
  void update(){
    motorLeft.setTargetSpeed(leftSpeed);
    motorRight.setTargetSpeed(rightSpeed);

    // Update the engines
    motorLeft.update();
    motorRight.update();
  }

};

#endif
#ifndef SILNIK_H
#define SILNIK_H

#include <Arduino.h>

#define JOYSTICK_DEADZONE 20

namespace Engines{

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
      Silnik(int ena, int in1, int in2, int ch);

      // Inicjalizacja 
      void begin();

      // Ustawienie celu - tę funkcję wywołuje Joystick/Mixer
      // Zakres wejściowy: -255 (tył) do 255 (przód)
      void setTargetSpeed(int speed);

      // Główna funkcja aktualizująca (wywoływać w loop non-stop)
      void update();
      
      void stop();
  };

  /// @brief Controls two engines of class Silnik, both should be powered from L298N bridge.
  class DoubleEngine{
  private:
    const int deadZone; ///< Specifies the range around joystick 'zero' which is ingored, so minor fluctuations of pwm value ar not consdiered.

    Silnik motorLeft;   ///< The left motor controller.
    Silnik motorRight;  ///< The right motor controller.

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
    DoubleEngine(uint8_t l_in1, uint8_t l_in2, uint8_t l_ena, uint8_t r_in1, uint8_t r_in2, uint8_t r_enb);

    /// @brief Does nothing
    ~DoubleEngine();

    // ------------------ Initialization ------------------ //

    /// @brief Initlizes both engines. 
    void initEngines();

    // ------------------ Steering ------------------ //

    /// @brief Stops both engines
    void stop();

    /// @brief Applies sttering values and adjusts engines rotation speed and direction.
    /// @param xx Pwm value in range 0-1023. (Should be taken from joystick).
    /// @param yy Pwm value in range 0-1023. (Should be taken from joystick).
    void applySteering(int xx, int yy);

    /// @brief Causes the engines to apply roation.
    void update();

    // ------------------ Getters ------------------ //

    /// @brief Returns the #leftSpeed value.
    ///        If its negative then the motor is going backwards.
    /// @return #leftSpeed.
    int getLeftSpeed();

    /// @brief Returns the #rightSpeed value.
    ///        If its negative then the motor is going backwards.
    /// @return #rightSpeed.
    int getRightSpeed();

  };
};


#endif
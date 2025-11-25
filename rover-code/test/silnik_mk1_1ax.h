
#ifndef SILNIK_H
#define SILNIK_H

#include <Arduino.h>

/*
 * Klasa Silnik do obsługi podwozia robota na ESP32.
 * Funkcjonalności:
 * 1. Sterowanie PWM (prędkością) za pomocą drivera (np. L298N, TB6612).
 * 2. Obsługa Martwej Strefy (Dead Zone) joysticka.
 * 3. Soft Start - płynne rozpędzanie i hamowanie chroniące przekładnie.
 */

class Silnik {
  private:
    // Piny fizyczne
    int pinA;
    int pinB;

    // Ustawienia PWM ESP32
    int kanalPWM_A;
    int kanalPWM_B;
    const int czestotliwosc = 5000;
    const int rozdzielczosc = 8; // 8 bitów = zakres 0-255

    // Zmienne do logiki Soft Start
    int aktualnaMoc = 0;          // Wartość chwilowa (-255 do 255)
    int docelowaMoc = 0;          // Wartość, do której dążymy (z joysticka)

    unsigned long ostatniaZmianaCzasu = 0;
    const int interwalKroku = 5;  // Czas w ms między krokami (im mniej, tym płynniej)
    const int wielkoscKroku = 3;  // O ile zwiększamy moc w jednym kroku (im więcej, tym szybciej)

  public:
    // Konstruktor
    Silnik(int pA, int pB, int kanalA, int kanalB) {
      pinA = pA;
      pinB = pB;
      kanalPWM_A = kanalA;
      kanalPWM_B = kanalB;

      // Konfiguracja kanałów PWM (LEDC)
      ledcSetup(kanalPWM_A, czestotliwosc, rozdzielczosc);
      ledcSetup(kanalPWM_B, czestotliwosc, rozdzielczosc);

      // Przypisanie pinów do kanałów
      ledcAttachPin(pinA, kanalPWM_A);
      ledcAttachPin(pinB, kanalPWM_B);
    }

    // Główna funkcja wywoływana w pętli loop()
    // Przyjmuje surową wartość z joysticka (0-1024)
    void ustawCel(int joystickInput) {

      // 1. Mapowanie joysticka na docelową moc (-255 do 255)
      // DEAD ZONE (500-530)
      if (joystickInput >= 500 && joystickInput <= 530) {
        docelowaMoc = 0;
      }
      // JAZDA DO PRZODU (> 530)
      else if (joystickInput > 530) {
        if (joystickInput < 630) docelowaMoc = 64;       // ~25%
        else if (joystickInput < 730) docelowaMoc = 128; // ~50%
        else if (joystickInput < 830) docelowaMoc = 191; // ~75%
        else docelowaMoc = 255;                          // 100%
      }
      // JAZDA DO TYŁU (< 500)
      else if (joystickInput < 500) {
        if (joystickInput > 400) docelowaMoc = -64;
        else if (joystickInput > 300) docelowaMoc = -128;
        else if (joystickInput > 200) docelowaMoc = -191;
        else docelowaMoc = -255;
      }

      // 2. Wykonanie kroku Soft Startu
      aktualizujSoftStart();
    }

  private:
    // Funkcja obliczająca pośrednie wartości prędkości
    void aktualizujSoftStart() {
      unsigned long teraz = millis();

      // Sprawdzamy czy minął czas interwału
      if (teraz - ostatniaZmianaCzasu >= interwalKroku) {

        // Jeśli aktualna moc jest mniejsza od celu -> zwiększamy
        if (aktualnaMoc < docelowaMoc) {
          aktualnaMoc += wielkoscKroku;
          if (aktualnaMoc > docelowaMoc) aktualnaMoc = docelowaMoc; // Nie przestrzel celu
        }
        // Jeśli aktualna moc jest większa od celu -> zmniejszamy
        else if (aktualnaMoc > docelowaMoc) {
          aktualnaMoc -= wielkoscKroku;
          if (aktualnaMoc < docelowaMoc) aktualnaMoc = docelowaMoc; // Nie przestrzel celu
        }

        ostatniaZmianaCzasu = teraz;

        // Zastosowanie nowej wartości na silnik
        zastosujMocNaSilnik();
      }
    }

    // Funkcja bezpośrednio sterująca pinami
    // Korzysta ze zmiennej klasowej 'aktualnaMoc', nie potrzebuje argumentów
    void zastosujMocNaSilnik() {
      if (aktualnaMoc > 0) {
        // Jazda do przodu
        ledcWrite(kanalPWM_A, aktualnaMoc);
        ledcWrite(kanalPWM_B, 0);
      }
      else if (aktualnaMoc < 0) {
        // Jazda do tyłu (abs zamienia np. -128 na 128)
        ledcWrite(kanalPWM_A, 0);
        ledcWrite(kanalPWM_B, abs(aktualnaMoc));
      }
      else {
        // Stop
        ledcWrite(kanalPWM_A, 0);
        ledcWrite(kanalPWM_B, 0);
      }
    }
};

#endif
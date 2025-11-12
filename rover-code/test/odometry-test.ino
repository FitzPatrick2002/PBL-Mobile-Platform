// TO DO:

// Odometry theory
// https://medium.com/@nahmed3536/wheel-odometry-model-for-differential-drive-robotics-91b85a012299
// https://ecse.monash.edu/centres/irrc/LKPubs/MECSE-1996-6.pdf

// Encoders
// https://wiki.dfrobot.com/Wheel_Encoders_for_DFRobot_3PA_and_4WD_Rovers__SKU_SEN0038_

// Interrupts:
// https://lastminuteengineers.com/handling-esp32-gpio-interrupts-tutorial/

// Protecting data during reading (critical section)
// https://freertos.org/Documentation/02-Kernel/04-API-references/04-RTOS-kernel-control/01-taskENTER_CRITICAL_taskEXIT_CRITICAL
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/freertos_idf.html#critical-sections

// Think about setting the single core use only, dual core requires mutextes, etc which complicate things a little bit

// PROBLEMS:
// 0. When should (x, y) be updated?
//    - Fixed timer calls interrupt every 250ms?
//    - Handled in the main loop in the DRIVING state (called sequentially)

// TO DO:
//-1. Straight line case -> deltaTheta = 0 (ok)
// 0. Create doxygen docs for the code
//    Document limitations (max distance travel before overflow, etc)
// 1. Read about the freeRTOS critical section: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/freertos_idf.html#critical-sections
// 2. Encapsulate all of the code in single namesapce
// 3. Test the code on actual equipment
// 4. Add getters and setters

// ADDITIONALS:
// 0. Measure time between calls of .updatePosition()
// 1. Calculate speed based on that
// 2. Debugging printing using macros?

#include <Odometer2Wheel.h>

portMUX_TYPE odometrySpinlock = portMUX_INITIALIZER_UNLOCKED;
Odometry::Odometer2Wheel odometer(0, 1, 10, 25, 10, 150);

void IRAM_ATTR leftTick(){
  odometer.leftRotation();
}

void IRAM_ATTR rightTick(){
  odometer.rightRotation();
}

void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);
  attachInterrupt(odometer.getLeftPin(), leftTick, RISING);
  attachInterrupt(odometer.getRightPin(), rightTick, RISING);
}

void loop() {
  // put your main code here, to run repeatedly:
  if(odometer.updatePosition()){
    Serial.print("Position: ()");
    Serial.print(odometer.getXpos());
    Serial.print(", ");
    Serial.print(odometer.getYpos());
    Serial.println(")");
  }
}

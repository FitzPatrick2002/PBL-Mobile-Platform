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

// TO DO:
// 0. Create doxygen docs for the code
// 1. Read about the freeRTOS critical section: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/freertos_idf.html#critical-sections
// 3. Test the code on actual equipment

#include <math.h>
#include <mutex>

static portMUX_TYPE odometrySpinlock = portMUX_INITIALIZER_UNLOCKED;

/// @brief Converts number of ticks of the encoders to angle byc which the wheel rotated.
float ticksToAngle(uint8_t ticksPerRotation, uint8_t ticks){
  return ((float)ticks / (float)ticksPerRotation) * M_PI;
}

/// @brief Calculates the displacement based on the angle by which a wheel has been turned.
/// @param radius Radius of the circle in mm.
/// @param theta Angle by which the circle rotated.
/// @return Length of the angle specified by the angle and radius.
float rotationToDistance(float radius, float theta){
  return radius * theta;
}

/// @brief Class handles calculation of position with respect to starting position of a 2 wheeled robot.
///       It's a singleton class. Only a single positioning system is utilized anyways.
class Odometer2Wheel{
public:
  // Pinout of the encoders
  uint8_t leftPin;
  uint8_t rightPin;

private:
  // Counts number of rotations of each encoder. [0] - left, [1] - right, looking from rear towards the front of the rover.
  volatile uint16_t coder[2] = {0, 0}; ///< Counts rotations since last measurement.
  uint16_t totalDistance[3] = {0, 0, 0}; ///< Total count of rotations of each wheel and the center point between them.

  // Position handling
  float x, y = 0.0; ///< Position in mm. Assuming (0, 0) on the start.
  float theta = 0.0; ///< Hard to explain in short

  // Timer
  uint32_t timer = 0; ///< Internal timer.
  uint32_t frequency = 10; ///< Frequency of measurements in Hz.

  // Physical dimensions 
  float wheelRadius = 0; ///< Wheel radius in mm.
  uint8_t ticksPerRotation = 10; ///< Number of ticks per rotation of the encoder wheel.

  float wheelSpacing = 10; ///< Distance between wheels on the platforma
  
private:

public:

  // ---------- Constructr & Destructor ---------- //
  
  Odometer2Wheel(uint8_t left, uint8_t right, uint32_t freq, float radius, uint8_t ticksPerRot, float wheelSpace){
    leftPin = left;
    rightPin = right;

    frequency = freq;
    wheelRadius = radius;
    ticksPerRotation = ticksPerRot;
    wheelSpacing = wheelSpace;
  }

  ~Odometer2Wheel() {}

  // ---------- Setters and Getters ---------- //

  void setLeftPin(uint8_t left){
    this->leftPin = left;
  }

  void setRightPin(uint8_t right){
    this->rightPin = right;
  }

  uint8_t getLeftPin(){
    return this->leftPin;
  }

  uint8_t getRightPin(){
    return this->rightPin;
  }

  float getXpos(){
    return this->x;
  }

  float getYpos(){
    return this->y;
  }

  // ---------- Interrupt routines ---------- //

  void IRAM_ATTR leftRotation(){
    this->coder[0]++;
  }

  void IRAM_ATTR rightRotation(){
    this->coder[1]++;
  }

  // ---------- Distance calculation ---------- //

  /// @brief Calcualtes the new position of the rover
  bool updatePosition(){
    if(millis() - timer > 1000 / frequency){
      // Calculate distance ceovered by each wheel
      taskENTER_CRITICAL(&odometrySpinlock);
      float leftDistance = rotationToDistance(wheelRadius, ticksToAngle(ticksPerRotation, coder[0]));
      float rightDistance = rotationToDistance(wheelRadius, ticksToAngle(ticksPerRotation, coder[1]));
      taskEXIT_CRITICAL(&odometrySpinlock);

      // Calculate parameters of the arc that the rover travelled between the measurements
      float deltaTheta = (rightDistance - leftDistance) / wheelSpacing;
      //float radius = 2.0 * leftDistance * rightDistance / (rightDistance - leftDistance) + wheelSpacing;
      float averageDistance = (leftDistance + rightDistance) / 2.0;

      // Calcualte the new position
      x = x + averageDistance * cos(theta + deltaTheta / 2.0);
      y = y + averageDistance * sin(theta + deltaTheta / 2.0);
      theta = theta + deltaTheta;

      // Resets the coders
      taskENTER_CRITICAL(&odometrySpinlock);
      // Save the total distance
      totalDistance[0] += coder[0];
      totalDistance[1] += coder[1];
      totalDistance[2] += (coder[0] + coder[1]) / 2;

      // Reset the distanc emeasurement
      coder[0] = 0;
      coder[1] = 0;
      taskEXIT_CRITICAL(&odometrySpinlock);

      // Update the timer
      timer = millisz();

      // Return true when position has been updated
      return true;
    }

    return false;
  }

};

Odometer2Wheel odometer(0, 1, 10, 25, 10, 150);

void IRAM_ATTR leftTick(){
  odometer.leftRotation();
}

void IRAM_ATTR rightTick(){
  odometer.rightRotation();
}

unsigned long timer = 0;


void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);
  attachInterrupt(odometer.getLeftPin(), leftTick, CHANGE);
  attachInterrupt(odometer.getRightPin(), rightTick, CHANGE);
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

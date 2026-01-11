#include "Odometer2Wheel.h"

namespace Odometry{

  // ------------------------------------------------ //
  // ------------- ODOMETER2WHEEL CLASS ------------- //
  // ------------------------------------------------ //

  // ----------- Constructr & Destructor ----------- //
    
  /// @brief Constructs the Odometer2Wheels class.
  ///        By default motion direction for both wheels is set to #MotionDirection::FORWARD.
  /// @param left Digital pin of the left encoder.
  /// @param right Digital pin of the right encoder.
  /// @param freq Maximal frequency of position updates in Hertz.
  /// @param radius Radius of the wheels [mm].
  /// @param ticksPerRot Number of ticks which encoder can measure per full wheel rotation.
  /// @param wheelSpace Spacing of the wheels (distance between them, rozstaw po polskiemu).
  Odometer2Wheel::Odometer2Wheel(uint8_t left, uint8_t right, uint32_t freq, float radius, uint8_t ticksPerRot, float wheelSpace){
      // Pins
      leftPin = left;
      rightPin = right;

      // "Constants" 
      frequency = freq;
      wheelRadius = radius;
      ticksPerRotation = ticksPerRot;
      wheelSpacing = wheelSpace;

      // Direction of driving
      leftWheelDirection = MotionDirection::FORWARD;
      rightWheelDirection = MotionDirection::FORWARD;
  }

  /// @brief Destroys the Odometer2Wheels object, for now its empty.
  Odometer2Wheel::~Odometer2Wheel() {}

  // ---------- Setters and Getters ---------- //

  void Odometer2Wheel::setLeftPin(uint8_t left){
      this->leftPin = left;
  }

  void Odometer2Wheel::setRightPin(uint8_t right){
      this->rightPin = right;
  }

  uint8_t Odometer2Wheel::getLeftPin(){
      return this->leftPin;
  }

  uint8_t Odometer2Wheel::getRightPin(){
      return this->rightPin;
  }

  float Odometer2Wheel::getXpos(){
      return this->x;
  }

  float Odometer2Wheel::getYpos(){
      return this->y;
  }

  float Odometer2Wheel::getTheta(){
      return theta;
  }

  /// @brief Sets the left wheel motion direction.
  /// @param dir New motion direction
  void Odometer2Wheel::setLeftWheelMotionDirection(MotionDirection dir){
      leftWheelDirection = dir;
  }

  /// @brief Sets the left wheel motion direction.
  /// @param dir New motion direction
  void Odometer2Wheel::setRightWheelMotionDirection(MotionDirection dir){
      rightWheelDirection = dir;
  }

  /// @brief Returns the left wheels motion direction.
  /// @return left wheels motion direction.
  MotionDirection Odometer2Wheel::getLeftWheelMotionDirection(){
      return leftWheelDirection;
  }

  /// @brief Returns the left wheels motion direction.
  /// @return left wheels motion direction.
  MotionDirection Odometer2Wheel::getRightWheelMotionDirection(){
      return rightWheelDirection;
  }

  // ---------- Debugging Section ---------- //

  /// @brief Sets the left #coder [0] encoder rotations to given number.
  /// @param rots Number of rotations set.
  void Odometer2Wheel::setCoderLeft(int8_t rots){
      coder[0] = rots;
  }

  /// @brief Sets the right #coder [0] encoder rotations to given number.
  /// @param rots Number of rotations set.
  void Odometer2Wheel::setCoderRight(int8_t rots){
      coder[1] = rots;
  }

  /// @brief Resets all the values to their defaults (0.0)
  void Odometer2Wheel::reset(){
      coder[0] = 0;
      coder[1] = 0;
      theta = 0;
      x = 0.0;
      y = 0.0;
      totalDistance[0] = 0;
      totalDistance[1] = 0;
      totalDistance[2] = 0;
  }

  // ---------- Communication ---------- //

  /// @brief Writes the odometry data into a given stream in a csv file format.
  ///        Data is not transposed, rows are rows and columns are columns.
  ///        Default separator: ';'
  ///        Default decimal:   '.'
  ///        Columns: x | y | theta | totalDistance[0] (coder[0] distance) | totalDistance[1] (coder[1] distance) | totalDistance[2] ((coder[0] + coder[1]) / 2)
  /// @param output Class inheriting from Stream. Needs methods .print() and .println().
  /// @param sep Separator character in CSV file. Defaults to ';'.
  void Odometer2Wheel::writeToCSV(Stream& output, char sep){
      output.print(x);
      output.print(sep);
      output.print(y);
      output.print(sep);
      output.print(theta);
      output.print(sep);

      output.print(totalDistance[0]);
      output.print(sep);
      output.print(totalDistance[1]);
      output.print(sep);
      output.print(totalDistance[2]);
      output.println("");
  }

  // ---------- Interrupt Routines ---------- //

  void IRAM_ATTR Odometer2Wheel::leftRotation(){
      this->coder[0] += (int16_t)this->leftWheelDirection;
  }

  void IRAM_ATTR Odometer2Wheel::rightRotation(){
      this->coder[1] += (int16_t)this->rightWheelDirection;
  }

  // ---------- Distance calculation ---------- //

  /// @brief Calcualtes the new position of the rover
  /// @param thetaOffset Driving direction accoridng to external sensor, for example imu (yaw euelr angle).
  bool Odometer2Wheel::updatePosition(float thetaOffset){
      if(millis() - timer > 1000 / frequency){
          // Calculate distance covered by each wheel
          taskENTER_CRITICAL(&odometrySpinlock);

          // Account for minor anomalies / slips in wheels
          if(abs(coder[0] - coder[1]) <= 2)
          {
              if(coder[0] > coder[1]){
                  coder[1] = coder[0];
              }
              else{
                  coder[0] = coder[1];
              }
          }

          float leftDistance = Odometry::rotationToDistance(wheelRadius, Odometry::ticksToAngle(ticksPerRotation, coder[0]));
          float rightDistance = Odometry::rotationToDistance(wheelRadius, Odometry::ticksToAngle(ticksPerRotation, coder[1]));
          taskEXIT_CRITICAL(&odometrySpinlock);

          // Legacy code below based on dead reconing.
          
          // Calculate parameters of the arc that the rover travelled between the measurements
          //float deltaTheta = (rightDistance - leftDistance) / wheelSpacing;

          //float radius = 2.0 * leftDistance * rightDistance / (rightDistance - leftDistance) + wheelSpacing;
          float averageDistance = (leftDistance + rightDistance) / 2.0;
          

          // Update theta value based on some external sensor value
          theta = thetaOffset; 

          // Calcualte the new position
          x = x + averageDistance * cos(theta); // + deltaTheta / 2.0);
          y = y + averageDistance * sin(theta); // + deltaTheta / 2.0);
          //theta = theta + deltaTheta;

          // Resets the coders
          taskENTER_CRITICAL(&odometrySpinlock);
          // Save the total distance
          totalDistance[0] += coder[0];
          totalDistance[1] += coder[1];
          totalDistance[2] += ((float)coder[0] + (float)coder[1]) / 2.0f; // If only one wheel is used 1/2 is rounded to 0.0 :(

          // Reset the distanc emeasurement
          coder[0] = 0;
          coder[1] = 0;
          taskEXIT_CRITICAL(&odometrySpinlock);

          // Update the timer
          timer = millis();

          // Return true when position has been updated
          return true;
      }

      return false;
  }

};
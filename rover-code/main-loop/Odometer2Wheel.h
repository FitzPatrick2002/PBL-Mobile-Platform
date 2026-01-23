/// @file Odometer2Wheel.h
/// @brief Namespace @ref Odometry contains class @ref Odometer2Wheel which handles calculation 
///        of mobile platform position.

#ifndef Odometer_2_WHEEL_H
#define Odometer_2_WHEEL_H

#include <math.h>
#include <Arduino.h>

/// @brief Contains odometry related functions and implements class for naviagtion of 2 wheeled robot.
///        Exemplary use is provided below.
/**
 * @code
 * #include <Odometer2Wheel.h>
 * 
 * 
 * // Create the Odometer2Wheel object
 * Odometry::Odometer2Wheel odometer(0, 1, 10, 25, 10, 150);
 * 
 * // Specify the ISR which counts ticks on the left wheel
 * void IRAM_ATTR leftTick(){
 *      odometer.leftRotation();
 * }
 *
 * // Specify ISR which counts ticks on the right wheel
 * void IRAM_ATTR rightTick(){
 *     odometer.rightRotation();
 * }
 *
 *
 * void setup() {
 *   // put your setup code here, to run once:
 *
 *   // Init UART
 *   Serial.begin(9600);
 * 
 *   // Attach interrupt routines to specified pins. 
 *   // Ticks will be counted on the RISING edge of signals.
 *   // FALLING can also be used here.
 *   attachInterrupt(odometer.getLeftPin(), leftTick, RISING);
 *   attachInterrupt(odometer.getRightPin(), rightTick, RISING);
 *   delay(500);
 *
 *   // Reset the odometer, just in case there was jitter on ISR pins, which would cause it to fire early
 *   odometer.reset();
 * }
 * 
 *  void loop() {
 *  // put your main code here, to run repeatedly:
 * 
 *  // After each position update print the results
 *  if(odometer.updatePosition()){
 *      odometer.writeToCSV(Serial, ';');
 *  }
 * }
 * @endcode
 */
namespace Odometry{
    /// @brief Converts number of ticks of the encoders to angle byc which the wheel rotated.
    inline float ticksToAngle(int16_t ticksPerRotation, int16_t ticks){
        return ((float)ticks / (float)ticksPerRotation) * 2.0 * M_PI;
    }

    /// @brief Calculates the displacement based on the angle by which a wheel has been turned.
    /// @param radius Radius of the circle in mm.
    /// @param theta Angle by which the circle rotated.
    /// @return Length of the angle specified by the angle and radius.
    inline float rotationToDistance(float radius, float theta){
        return radius * theta;
    }
  
    /// @brief Specifies the direction of rotation forward / backward -> (1 / -1)
    enum MotionDirection{
        FORWARD = 1, ///< Add covered distance.
        BACKWARD = -1 ///< Subtract covered distance.
    };

    /// @brief Class handles calculation of position with respect to starting position of a 2 wheeled robot.
    ///        Odometry is calculated based on patterns derived in here:
    /// @link https://medium.com/@nahmed3536/wheel-odometry-model-for-differential-drive-robotics-91b85a012299.
    /// @warning The @ref theta parameter is currently set based on data provided from an IMU sensor.
    ///          It can be calculated purely from wheel rotations data but currently is not.
    /// @todo Provide some way to switch between ways of calculation of the @ref theta parameter.
    class Odometer2Wheel{
    public:
        // Pinout of the encoders
        uint8_t leftPin;  ///< GPIO pin on which encoder for the left wheel is connected.
        uint8_t rightPin; ///< GPIO pin on which encoder for the right wheel is connected.

        MotionDirection leftWheelDirection;  ///< Specifies direction of rotation of the left wheel.
        MotionDirection rightWheelDirection; ///< Specifies direction of rotation of the left wheel.

    private:
        // Counts number of rotations of each encoder. [0] - left, [1] - right, looking from rear towards the front of the rover.
        portMUX_TYPE odometrySpinlock = portMUX_INITIALIZER_UNLOCKED; ///< Spinlock protecting #coder table from races.
        
        volatile int16_t coder[2] = {0, 0};          ///< Counts rotations since last measurement.
        float totalDistance[3] = {0.0f, 0.0f, 0.0f}; ///< Total count of rotations of each wheel and the center point between them.

        // Position handling
        float x, y = 0.0;              ///< Position in mm. Assuming (0, 0) on the start.
        float theta = 0.0;             ///< Hard to explain in short. Also, theta in normal polar coordinates. Angle between x-axis and direction of driving.

        // Timer
        uint32_t timer = 0;            ///< Internal timer.
        uint32_t frequency = 10;       ///< Frequency of measurements in Hz.

        // Physical dimensions 
        float wheelRadius = 0;         ///< Wheel radius in mm.
        uint8_t ticksPerRotation = 10; ///< Number of ticks per rotation of the encoder wheel.

        float wheelSpacing = 10;       ///< Distance between wheels on the platforma
    private:

    public:

        // ---------- Constructr & Destructor ---------- //
        
        /// @brief Constructs the Odometer2Wheels class.
        ///        By default motion direction for both wheels is set to #MotionDirection::FORWARD.
        /// @param left Digital pin of the left encoder.
        /// @param right Digital pin of the right encoder.
        /// @param freq Maximal frequency of position updates in Hertz.
        /// @param radius Radius of the wheels [mm].
        /// @param ticksPerRot Number of ticks which encoder can measure per full wheel rotation.
        /// @param wheelSpace Spacing of the wheels (distance between them, rozstaw po polskiemu).
        Odometer2Wheel(uint8_t left, uint8_t right, uint32_t freq, float radius, uint8_t ticksPerRot, float wheelSpace);

        /// @brief Destroys the Odometer2Wheels object, for now its empty.
        ~Odometer2Wheel();

        // ---------- Setters and Getters ---------- //

        /// @brief Sets the pin on which ticks from left encoder will be registered.
        /// @param left GPIO pin.
        void setLeftPin(uint8_t left);

        /// @brief Sets the pin on which ticks from right encoder will be registered.
        /// @param right GPIO pin.
        void setRightPin(uint8_t right);

        /// @brief Getter for @ref leftPin.
        uint8_t getLeftPin();

        /// @brief Getter for @ref rightPin.
        uint8_t getRightPin();

        /// @brief Returns the @ref x position.
        float getXpos();

        /// @brief Returns the @ref y position.
        float getYpos();

        /// @brief Returns the current driving direction @ref theta.
        float getTheta();

        /// @brief Sets the left wheel motion direction.
        /// @param dir New motion direction
        void setLeftWheelMotionDirection(MotionDirection dir);

        /// @brief Sets the left wheel motion direction.
        /// @param dir New motion direction
        void setRightWheelMotionDirection(MotionDirection dir);

        /// @brief Returns the left wheels motion direction.
        /// @return left wheels motion direction.
        MotionDirection getLeftWheelMotionDirection();

        /// @brief Returns the left wheels motion direction.
        /// @return left wheels motion direction.
        MotionDirection getRightWheelMotionDirection();

        // ---------- Debugging Section ---------- //

        /// @brief Sets the left #coder [0] encoder rotations to given number.
        /// @param rots Number of rotations set.
        void setCoderLeft(int8_t rots);

        /// @brief Sets the right #coder [0] encoder rotations to given number.
        /// @param rots Number of rotations set.
        void setCoderRight(int8_t rots);

        /// @brief Resets all the values to their defaults (0.0)
        void reset();

        // ---------- Communication ---------- //

        /// @brief Writes the odometry data into a given stream in a csv file format.
        ///        Data is not transposed, rows are rows and columns are columns.
        ///        Default separator: ';'
        ///        Default decimal:   '.'
        ///        Columns: x | y | theta | totalDistance[0] (coder[0] distance) | totalDistance[1] (coder[1] distance) | totalDistance[2] ((coder[0] + coder[1]) / 2)
        /// @param output Class inheriting from Stream. Needs methods .print() and .println().
        /// @param sep Separator character in CSV file. Defaults to ';'.
        void writeToCSV(Stream& output, char sep = ';');

        // ---------- Interrupt Routines ---------- //

        /// @brief ISR which counts the number of ticks on the left wheel.
        ///        Each time tick occures @ref leftWheelDirection is added to [0]th entry in @ref coder.
        void IRAM_ATTR leftRotation();

        /// @brief ISR which counts the number of ticks on the right wheel.
        /// Each time tick occures @ref rightWheelDirection is added to [1]st entry in @ref coder.
        void IRAM_ATTR rightRotation();

        // ---------- Distance calculation ---------- //

        /// @brief Calcualtes the new position of the rover
        /// @param thetaOffset Driving direction accoridng to external sensor, for example imu (yaw euelr angle).
        bool updatePosition(float thetaOffset);
    };
};

#endif

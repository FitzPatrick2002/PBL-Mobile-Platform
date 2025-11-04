#include "lds_all_models.h"
#include <Math.h>
#include <Vector.h>

// TO SEE:
// https://manuals.plus/pl/slamtec/a1m8-rplidar-a1-low-cost-360-degree-laser-range-scanner-manual

// DONE:

// 2. Create callback for storing scans (ok)
// 3. Start function (ok)
// 4. Stop function (ok)
// 5. Scan function (ok)
// 6. Count scans number (ok)
//    - Reset function (ok)
// Vector2dPolar:
// 1. getX(), getY() -> for cartesian coords (ok)

// TO DO:

// 0. Read about ledc...() functions,
//    - Make simple project with flashing diode or smth for pwm, connecting pwm etc
// 1. Create class for simplified lidar control
// 2. Non-blocking scanning function for n-rotations
// 3. Setter for every_nth_point 

// 4. Odometry class: 
//    file:///C:/Users/milos/Downloads/Accurate_calibration_of_kinematic_parameters_for_t.pdf


// For the time being we want to debug the stuff that we read from or send to lidar
#define DEBUG_GPIO
#define DEBUG_SERIAL_OUT

/// @brief Stores measurement points in polar coordinates
struct Vector2dPolar{
  float theta; ///< Angle from x-axis in degrees
  float radius; ///< Distance from origin of the coordinate system

  float getX(){
    return radius * cos(theta * M_PI/180.0);
  }

  float getY(){
    return radius * sin(theta * M_PI/180.0);
  }
};

struct Vector2dCartesian{
  float x;
  float y;
};

/// @brief A controller class, which simplifies communication with lidar and performs most of the setup
class LidarController{
private:
  LDS* lidar; ///< Main lidar class, throygh which we communicate with lidar
  HardwareSerial LidarSerial = HardwareSerial(1); ///< Serial communication with lidar via UART 1 (Serial 1)

  // ----------- Lidar Pinout ----------- //
  uint8_t gpio_en = 2; ///< Enable pin
  uint8_t gpio_rx = 0; ///< UART receive pin
  uint8_t gpio_tx = 1; ///< UART transmit pin
  uint8_t gpio_pwm = 3; ///< PWM pin controlling LiDARs motor

  // Lidar internal setup & scanning setup
  uint8_t every_nth_point = 20; ///< When scan is performed, only every n-th point is included, other are discarded

  // Storage of measurement points
  Vector<Vector2dPolar> points;
  uint8_t point_counter = 0; ///< Counts how many points have been scanned since last point that has been saved. See @every_nth_point
  uint8_t scan_counter = 0; ///< Counts how many scans have been performed.

  // ----------- Constants ----------- //
  // Constants specify the communication protocol parameters 

  const uint32_t SERIAL_MONITOR_BAUD = 115200;
  const uint32_t LIDAR_PWM_FREQ = 10000;
  const uint32_t LIDAR_PWM_BITS = 11;
  const uint32_t LIDAR_PWM_CHANNEL = 2;
  const uint32_t LIDAR_SERIAL_RX_BUF_LEN = 1024;
  const uint32_t HEX_DUMP_WIDTH = 16;

  uint8_t hex_dump_pos = 0; ///< Required by the serailReadCallback

public:

  // ----------- Constructors & Destructors ----------- //

  LidarController() {}
  ~LidarController() {}

  static LidarController& getInstance(){
    static LidarController lidarControllerInstance;
    return lidarControllerInstance;
  }

  // ----------- Initilization ----------- //

  /// @brief Initilizes the lidar, defines callbacks and sets them
  void init(){
    // Create instance of used lidar
    lidar = new LDS_RPLIDAR_A1();

    this->lidar->setScanPointCallback(LidarController::getInstance().staticScanPointCallback);
    this->lidar->setPacketCallback(LidarController::getInstance().staticPacketCallback);
    this->lidar->setSerialWriteCallback(LidarController::getInstance().staticSerialWriteCallback);
    this->lidar->setSerialReadCallback(LidarController::getInstance().staticSerialReadCallback);

    this->lidar->setMotorPinCallback(LidarController::staticMotorPinCallback);
    this->lidar->setInfoCallback(LidarController::getInstance().staticInfoCallback);
    this->lidar->setErrorCallback(LidarController::getInstance().staticErrorCallback);

    delay(200);

    this->LidarSerial.begin(lidar->getSerialBaudRate(), SERIAL_8N1, gpio_tx, gpio_rx);

    delay(200);

    this->lidar->init();
  }

  // ----------- Operation ----------- //

  /// @brief Starts the lidar
  /// @returns The result of the startup contains information if it was successfull.
  ///          Refer to the LDS__result_t enum
  LDS::result_t start(){
    return this->lidar->start();
  }

  /// @brief Stops the lidar
  ///        Resets the @point_counter and @scan_counter
  void stop(){
    this->lidar->stop();
    this->resetPointCounter();
    this->resetScanCounter();
  }

  /// @brief Invokes the lidars loop.
  ///        Measurements are gathered 
  void loop(){
    this->lidar->loop();
  }

  /// @brief Sets the point counter to zero
  void resetPointCounter(){
    this->point_counter = 0;
  }

  void resetScanCounter(){
    this->scan_counter = 0;
  }

  /// @brief Performs N rotations of the lidar and saves the data
  /// @param[in] n How many times lidar should rotate
  void scanNtimes(uint8_t n){
    // Prepare the lidar
    this->stop();

    // Start the lidar and print outcome
    delay(1000);
    LDS::result_t start_result = this->start();

    Serial.print("scanNtimes() result: ");
    Serial.println(lidar->resultCodeToString(start_result));

    // Perform the scan n times
    while(this->scan_counter < n){
      this->lidar->loop();
    }

    this->stop();

    delay(1000);
  }

  /// @brief Clears the storage vector @points
  void clearPoints(){
    this->points.clear();
  }

  // ----------- Communication ----------- //

  /// @brief Sends the data to desired target.
  //         After points are copied, the storage vector @points is emptied
  /// @param[out] target Target storage to which all of the points will be copied
  void requestData(Vector<Vector2dPolar> &target){
    target = points;
    this->clearPoints();
  }

  /// @brief Prints data via chosen Stream
  /// @param s Stream to which data will be transmitted
  void requestData(Stream &s){
    for(Vector2dPolar v : this->points){
      s.print("Point data: ");
      s.print(v.radius);
      s.print(" | ");
      s.println(v.theta);
    }

    this->clearPoints();
  }

private:

  // ----------- Storage ----------- //

  /// @brief Saves the scan point into the @scanPoints vector
  /// @param angle_deg Angle of measurement expressed in degrees
  /// @param distance_mm Distance measured expressed in mm
  void saveScanPoint(float angle_deg, float distance_mm){
    points.push_back({angle_deg, distance_mm});
  }

  // ----------- Callbacks ----------- //
  // Callbacks specify the callback functions for the LDS lidar
  // Each callback has a static wrapper that is used in the init() method to set the LDS lidar callbacks

  /// @brief Callabck routine for the lidar scanPointCallback
  ///        Saves points into @points vector. Skips avery n-th pointed, specified by @every_nth_point
  void scanPointCallback(float angle_deg, float distance_mm, float quality, bool scan_completed){
    if(scan_completed)
        scan_counter++;

      if(point_counter % every_nth_point == 0){
        saveScanPoint(angle_deg, distance_mm);
        point_counter = 0;
      }

      point_counter++;
  }

  /// @brief Static wrapper of @scanPointCallback()
  static void staticScanPointCallback(float angle_deg, float distance_mm, float quality, bool scan_completed){
    LidarController::getInstance().scanPointCallback(angle_deg, distance_mm, quality, scan_completed);
  }

  void packetCallback(uint8_t *packet, uint16_t length, bool scan_completed){
    #ifdef DEBUG_PACKETS
    Serial.println();
    Serial.print("Packet callback, length=");
    Serial.print(length);
    Serial.print(", scan_completed=");
    Serial.println(scan_completed);
    #endif
    
    return;
  }

  static void staticPacketCallback(uint8_t *packet, uint16_t length, bool scan_completed){
    LidarController::getInstance().packetCallback(packet, length, scan_completed);
  }

  size_t serialWriteCallback(const uint8_t * buffer, size_t length) {
    #ifdef DEBUG_SERIAL_OUT
    Serial.println();
    Serial.print('>');
    printBytesAsHex(buffer, length);
    Serial.println();
    #endif
    
    return LidarSerial.write(buffer, length);
  }

  static size_t staticSerialWriteCallback(const uint8_t * buffer, size_t length) {
    return LidarController::getInstance().serialWriteCallback(buffer, length);
  }

  int serialReadCallback() {
    #ifdef DEBUG_SERIAL_IN
    int ch = LidarSerial.read();
    if (ch != -1) {
      if (hex_dump_pos++ % HEX_DUMP_WIDTH == 0)
        Serial.println();
      printByteAsHex(ch);
    }
    return ch;
    #else
    return LidarSerial.read();
    #endif
  }

  static int staticSerialReadCallback(){
    return LidarController::getInstance().serialReadCallback();
  }

  void infoCallback(LDS::info_t code, String info) {
    Serial.print("LiDAR info ");
    Serial.print(lidar->infoCodeToString(code));
    Serial.print(": ");
    Serial.println(info);
  }

  static void staticInfoCallback(LDS::info_t code, String info){
    LidarController::getInstance().infoCallback(code, info);
  }

  void errorCallback(LDS::result_t code, String aux_info) {
    Serial.print("LiDAR error ");
    Serial.print(lidar->resultCodeToString(code));
    Serial.print(": ");
    Serial.println(aux_info);
  }

  static void staticErrorCallback(LDS::result_t code, String aux_info){
    LidarController::getInstance().errorCallback(code, aux_info);
  }

  // This callback is invoked later in a wrapper: setMotorPin()
  // setMotorPin appears in LDS_RPLIDAR_A1.enableMotor() (.cpp)
  // The lidar_pin part seems to stay the same: "LDS_MOTOR_PWM_PIN"
  // wheres depending on if motor is enabled or disabled we get:
  // value = DIR_OUTPUT_PWM, follwed by value = 0.8
  // And when it is disabled
  // value = DIR_OUTPUT_CONST, VALUE_LOW
  // DIR_OUTPUT_PWM -> gives signal to setup the pin 
  // if(!ledcAttachChannel(pin, this->LIDAR_PWM_FREQ, this->LIDAR_PWM_BITS, this->LIDAR_PWM_CHANNEL)) -> connects the pin as pwm pin, nothing else happens
  // 0.8 -> passes if(value < (float)LDS::VALUE_PWM) and goes to else which sets the pwm value on the pin: "ledcWriteChannel()"

  // Then when the motor is shut down:
  // DIR_OUTPUT_CONST -> sets the mode (configures the pin mode / output)
  // VALUE_LOW -> sets the pin low

  // Basically if float value == integer (from 0 to -5) then it sets a mode of the pin
  // If it's a float (eg. 0.8) -> it sets the pwm value on the pin
  // I can overwrite it the way I want, I just have to keep in mind how it's called in the library:
  /*
  void LDS_RPLIDAR_A1::enableMotor(bool enable) {
    motor_enabled = enable;

    if (enable) {
      setMotorPin(DIR_OUTPUT_PWM, LDS_MOTOR_PWM_PIN);
    // TODO add PID
      setMotorPin(0.8, LDS_MOTOR_PWM_PIN);
    } else {
      setMotorPin(DIR_OUTPUT_CONST, LDS_MOTOR_PWM_PIN);
      setMotorPin(VALUE_LOW, LDS_MOTOR_PWM_PIN);
    }
  }
  */
  void motorPinCallback(float value, LDS::lds_pin_t lidar_pin){

    // If the lidar pin is set to motor_enable set 'int pin' as the gpio_pin and otherwise as gpio_pwm pin
    int pin = (lidar_pin == LDS::LDS_MOTOR_EN_PIN) ? this->gpio_en : this->gpio_pwm;

    #ifdef DEBUG_GPIO
    Serial.print("GPIO ");
    Serial.print(pin);
    Serial.print(' ');
    Serial.print(lidar->pinIDToString(lidar_pin));
    Serial.print(" mode set to ");
    Serial.println(lidar->pinStateToString((LDS::lds_pin_state_t) int(value)));
    #endif

    // If value lesser then DIR_INPUT there are 3 options:
    // value == DIR_INPUT, DIR_OUTPUT_CONST, DIR_OUTPUT_PWM
    if(value <= (float)LDS::DIR_INPUT){

      // Configure pin direction
      if(value == (float)LDS::DIR_OUTPUT_PWM){

        // Here would be an if statement, for ESP_IDF_VERSION_MAJOR < 5, we have to resort to ledcSetup
        if(!ledcAttachChannel(pin, this->LIDAR_PWM_FREQ, this->LIDAR_PWM_BITS, this->LIDAR_PWM_CHANNEL)){
          Serial.println("motorPinCallback() ledcAttachChannel() error");
        }
        
      }
      else{
        pinMode(pin, (value == (float)LDS::DIR_INPUT) ? INPUT : OUTPUT);
      }

      return;
    }

    if(value < (float)LDS::VALUE_PWM){
      // Constant output
      // If the value is set to high, write HIGH, if not, write LOW
      digitalWrite(pin, (value == (float)LDS::VALUE_HIGH) ? HIGH : LOW);
    }
    else{
      #ifdef DEBUG_GPIO
        Serial.print("PWM value set to: ");
        Serial.println(value);
      #endif 

      // Invert PWM duty cycle
      #ifdef INVERT_PWM_PIN
        value = 1 - value;
      #endif

      int pwm_value = ((1 << this->LIDAR_PWM_BITS) - 1) * value;

      ledcWriteChannel(this->LIDAR_PWM_CHANNEL, pwm_value);

      #ifdef DEBUG_GPIO
        Serial.print(' ');
        Serial.println(pwm_value);
      #endif
    }
  }

  static void staticMotorPinCallback(float value, LDS::lds_pin_t lidar_pin){
    LidarController::getInstance().motorPinCallback(value, lidar_pin);
  }

  // ----------- Formatting hex in Serial ----------- //

  void printByteAsHex(uint8_t b) {
    if (b < 16)
        Serial.print('0');
    Serial.print(b, HEX);
    Serial.print(' ');
  }

  void printBytesAsHex(const uint8_t * buffer, uint16_t length) {
    if (length == 0)
      return;

    for (uint16_t i = 0; i < length; i++) {
      printByteAsHex(buffer[i]);
    }
  }

};

LidarController lidar_controller;

long timer = 0;
bool all_scans_completed = false;

void setup() {
  // put your setup code here, to run once:

  Serial.begin(115200);
}

void loop() {
  // put your main code here, to run repeatedly:
  
  if(millis() - timer > 25000){
    lidar_controller.scanNtimes(5);
    all_scans_completed = true;
    timer = millis();
  }

  if(all_scans_completed == true){

    lidar_controller.requestData(Serial);
    all_scans_completed = false;
  }
}






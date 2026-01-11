/// @file LidarController.cpp
/// @brief Provides implementations for elements from LidarController.h.

namespace Lidar{

  LidarController::LidarController() {

  } 

  // ----------- Destructor ----------- //

  LidarController::~LidarController() {

  }

  // ----------- Initilization ----------- //

  void LidarController::init(){
      // Create instance of a used lidar
      lidar = new LDS_RPLIDAR_A1();

      // Set the callbacks 
      this->lidar->setScanPointCallback(LidarController::getInstance().staticScanPointCallback);
      this->lidar->setPacketCallback(LidarController::getInstance().staticPacketCallback);
      this->lidar->setSerialWriteCallback(LidarController::getInstance().staticSerialWriteCallback);
      this->lidar->setSerialReadCallback(LidarController::getInstance().staticSerialReadCallback);

      this->lidar->setMotorPinCallback(LidarController::staticMotorPinCallback);
      this->lidar->setInfoCallback(LidarController::getInstance().staticInfoCallback);
      this->lidar->setErrorCallback(LidarController::getInstance().staticErrorCallback);

      // Wait for everything to initialize 
      // and start serial communication through the SoftwareSerial object.
      delay(200);
      this->LidarSerial.begin(lidar->getSerialBaudRate(), SERIAL_8N1, gpio_tx, gpio_rx);
      delay(200);

      this->lidar->init();
  }

  // ----------- Operation ----------- //

  LDS::result_t LidarController::start(){
      return this->lidar->start();
  }

  void LidarController::stop(){
      this->lidar->stop();
      this->resetPointCounter();
      this->resetScanCounter();
  }

  void LidarController::loop(){
      this->lidar->loop();
  }

  void LidarController::resetPointCounter(){
      this->point_counter = 0;
  }

  void LidarController::resetScanCounter(){
      this->scan_counter = 0;
  }

  void LidarController::scanNtimes(uint8_t n = -1){
      // Prepare the lidar
      this->stop();
      delay(5000);

      // Start the lidar and print outcome
      LDS::result_t start_result = this->start();

      Serial.print("scanNtimes() result: ");
      Serial.println(lidar->resultCodeToString(start_result));

      // If the number of scans has been set globally, perform #desired_scans_num number of scans.
      // If not, perform the number specified locally.
      int loopBound = (n != -1 ? n : this->desired_scans_num);
      while(this->scan_counter < n){
          this->lidar->loop();
      }

      this->stop();

      delay(5000);
  }

  void LidarController::clearPoints(){
      this->points.clear();
  }

  // ----------- Communication ----------- //

  void LidarController::copyData(std::vector<float> &target, bool clearPoints = false){
      // Copy points from the original location 
      target = points;

      // If specified, clear the points vector
      if (clearPoints == true){
          this->clearPoints();
      }
  }

  std::vector<float>& LidarController::accessData(){
      return this->points;
  }

  void LidarController::printData(Stream &s, bool clearPoints = false){  
      s.println("Format: radius [mm] | phi [deg] | theta [deg]");

      // Print the gathered data
      for(int i = 0; i < this->points.size(); i += 3){
          s.print("Point: ");
          s.print(this->points[i]);
          s.print(" | ");
          s.print(this->points[i + 1]);
          s.print(" | ");
          s.println(this->points[i + 2]);
      }

      if (clearPoints == true){
          this->clearPoints();
      }
  }

  // ----------- Getters & Setters ----------- //

  void LidarController::setPinout(uint8_t en, uint8_t rx, uint8_t tx, uint8_t pwm){
      gpio_en = en;
      gpio_rx = rx;
      gpio_tx = tx;
      gpio_pwm = pwm;
  }

  void LidarController::setEveryNthPoint(uint8_t n){
      every_nth_point = n;
  }
  void LidarController::setDesiredScansNumber(int n){
      if(n > 0){
          this->desired_scans_num = n;
      }
  }

  void LidarController::setHardwareSerial(uint8_t num){
      LidarSerial = HardwareSerial(num);
  }

  void LidarController::setInclination(float inc){
      this->inclination = inc;
  }

  // ----------- Storage ----------- //

  void LidarController::saveScanPoint(float distance_mm, float phi_deg, float theta_deg){
      points.push_back(distance_mm);
      points.push_back(phi_deg);
      points.push_back(theta_deg);
  }

  // ----------- Callbacks ----------- //

  // Callbacks specify the callback functions for the LDS lidar
  // Each callback has a static wrapper that is used in the init() method to set the LDS lidar callbacks

  void LidarController::scanPointCallback(float angle_deg, float distance_mm, float quality, bool scan_completed){
      if(scan_completed)
          scan_counter++;

      if(point_counter % every_nth_point == 0){
          saveScanPoint(distance_mm, angle_deg, inclination);
          point_counter = 0;
      }

      point_counter++;
  }

  size_t LidarController::serialWriteCallback(const uint8_t * buffer, size_t length) {
      #ifdef DEBUG_SERIAL_OUT
      Serial.println();
      Serial.print('>');
      printBytesAsHex(buffer, length);
      Serial.println();
      #endif
      
      return LidarSerial.write(buffer, length);
  }

  int LidarController::serialReadCallback() {
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

  void LidarController::infoCallback(LDS::info_t code, String info) {
      Serial.print("LiDAR info ");
      Serial.print(lidar->infoCodeToString(code));
      Serial.print(": ");
      Serial.println(info);
  }

  void LidarController::errorCallback(LDS::result_t code, String aux_info) {
      Serial.print("LiDAR error ");
      Serial.print(lidar->resultCodeToString(code));
      Serial.print(": ");
      Serial.println(aux_info);
  }

  void LidarController::motorPinCallback(float value, LDS::lds_pin_t lidar_pin){

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
  // ----------- Formatting hex in Serial ----------- //

  void LidarController::printByteAsHex(uint8_t b) {
      if (b < 16)
          Serial.print('0');
      Serial.print(b, HEX);
      Serial.print(' ');
  }

  void LidarController::printBytesAsHex(const uint8_t* buffer, uint16_t length) {
      if (length == 0)
      return;

      for (uint16_t i = 0; i < length; i++) {
      printByteAsHex(buffer[i]);
      }
  }
}
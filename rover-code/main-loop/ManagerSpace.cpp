#include "ManagerSpace.h"

namespace ManagerSpace{

  // -------------- CLASS MANAGER -------------- //

  // -------------- Constructors & Destructors -------------- //

  Manager::Manager(Lidar::LidarController& lidarControllerInstance) : 
                                                                      lidarController(lidarControllerInstance),
                                                                      httpCommunicator(getSetting("wifi-ssid"),
                                                                                       getSetting("wifi-password"), 
                                                                                       getSetting("pc-server-name")) {
                                                                                       
  }
  
  Manager::~Manager() {}

  // -------------------------------------------- //
  // -------------- Public Methods -------------- //
  // -------------------------------------------- //

  // -------------- Components Initialization -------------- //

  /// @brief Initilizes and start the asynchronous server #asyncServer. 
  ///        Use at the end or after lidar has been initialized.
  void Manager::initAsyncServer(){
    // Init the endpoints where requests will be send 
    asyncServer.initCommandEndpoint();
    asyncServer.initScanEndpoint();

    // Start the server
    asyncServer.begin();
  }

  /// @brief Wrapper for engines initilization.
  /// @see #engineController
  void Manager::initEngines(){
    engineController.initEngines();
  }

  /// @brief Initilizes lidar.
  void Manager::initLidar(){
    lidarController.setPinout(LIDAR_EN, LIDAR_RX, LIDAR_TX, LIDAR_PWM);
    lidarController.setInclination(90.0f); // Inclination in degrees
    lidarController.init();
  }

  /// @brief Initializes imu.
  void Manager::initIMU(){
    // Init imu to use dmp, set the last bit of i2c address to 1 and show debug messages
    imu.init(true, 1, true);
  }

  // -------------- Main Loop Actions -------------- //

  /// @brief Main operation loop.
  void Manager::mainLoop(){
    // Check if any message has been received
    listenToMessage(); 

    // Perform action based on current state and other flags
    performAction();

    // Some routines need to be run during every execution of the main loop
    // They are run in here
    runEveryStep(); 
  }

  void Manager::printIMUdata(){
    ICM_IMU::EulerAngles data;
    imu.getEulerAngles(data);
    Serial.print("Euler: ");
    Serial.print(data.yaw);
    Serial.print(", ");
    Serial.print(data.pitch);
    Serial.print(", ");
    Serial.print(data.roll);
    Serial.println();
  }

  /// @brief Performs action based on the current state.
  void Manager::performAction(){
    //printIMUdata(); // DEBUG only

    switch(state){
      case ManagerState::STANDBY:
        // Do nothing
        Serial.println("Standby");
        EspNowCallback::tx_message.is_scanning=false;
        EspNowCallback::tx_message.status_text="Standby";
        delay(200);
      break;

      case ManagerState::MOVING:
        moveRover();
        updateOdometryDirection();
        Serial.println("Moving");
        EspNowCallback::tx_message.is_scanning=false;
        EspNowCallback::tx_message.status_text="Moving";
        delay(200);
      break;

      case ManagerState::SCANNING:
        Serial.println("Scanning");

        // Disable the ESP NOW for the duration of the scan and re-enable it after the scan
        esp_now_unregister_recv_cb();
        lidarScan();
        esp_now_register_recv_cb(esp_now_recv_cb_t(EspNowCallback::OnDataRecv));

        // Scan is performed once
        // After its done, platform is back to STANDBY state
        //setState(ManagerState::STANDBY);
        // vTaskDelay(pdMS_TO_TICKS(50)); // Let the core 0 finish http transmission

        setState(ManagerState::UPLOADING); // For now just upload data right after the scan
        delay(200);
        
        EspNowCallback::tx_message.is_scanning=true;
        EspNowCallback::tx_message.status_text="Scanning";
      break;
      
      case ManagerState::UPLOADING:
        Serial.println("Uploading");
        transmitLidarDataToPC();
        
        // After transmitting data to pc, platform switches to STANDBY state
        setState(ManagerState::STANDBY);
        delay(200);

        EspNowCallback::tx_message.is_scanning=false;
        EspNowCallback::tx_message.status_text="Uploading";
      break;

      case ManagerState::STATUS_UPDATE:
        Serial.println("Status update");
        kontrolerSendData();
      delay(200);
      break;

      case ManagerState::IMU_CALIBRATION:
        Serial.println("IMU is in calibration mode, do the dance!");
        imu.resetIMU();
      break;

      default:
        Serial.println("Unknown state");

        EspNowCallback::tx_message.is_scanning=false;
        EspNowCallback::tx_message.status_text="Unknown state";
        delay(200);
      break;
    }
  }

  /// @brief If the message flag is set, processes the message and sets appropriate state (and the state change flag).
  ///        Whole function is protected by the #messageSpinlock. The message and the new message flag should not change during the whole execution.
  void Manager::listenToMessage(){
    taskENTER_CRITICAL(&messageSpinlock);
    if(messageReceived == true){
      // Process the message
      EspNowCallback::Message localCopyMessage = (EspNowCallback::Message)(controllerMessage);

      // If rover is moving and the new state does not allow it to move, stop it
      if(state == ManagerState::MOVING && localCopyMessage.state != ManagerState::MOVING){
        engineController.stop();
      }

      // If imu was in calibration mode and it was switched off, newly learned biases are saved to EEPROM
      if(state == ManagerState::IMU_CALIBRATION && localCopyMessage.state != ManagerState::IMU_CALIBRATION){
        // If biases were succesfully saved on EEPROM, then print message
        if(imu.storeBiases())
          Serial.println("Imu exits calibration mode. Biases stored");
      }

      // Set the state based on message data (buttons status, joystick status, etc)
      state = localCopyMessage.state;

      // In debug mode, show the contents of the received message
      #ifdef DEBUG_SAVE_MSG
        // Message content 
        Serial.println("Message received: ");
        localCopyMessage.printData(Serial, ',');

        // New state
        Serial.print("State set to:");
        Serial.println((int)(state));
      #endif

      // Message has been processed, clear the new message flag.
      messageReceived = false; 
    }
    taskEXIT_CRITICAL(&messageSpinlock);
  }

  /// @brief Invokes routines necessary during every iteration through the main loop.
  ///        1. Odometry position update.
  ///        2. Send odometry update to core 0.
  ///        3. Check if there is update from flask server on pc.
  void Manager::runEveryStep(){
    
    this->updateOdometry();

    // If any message arrived from the PC, check it
    this->checkPCmessages();
  }

  // -------------- Functions Invoked Every Main Loop Step -------------- //

  /// @brief Checks if odometry update happened.
  ///        If it did, the data is sent to the core 0 queue and transmitted to the server.
  void Manager::updateOdometry(){
    // Read the driving direction with regard to north
    ICM_IMU::EulerAngles orientation;
    imu.getEulerAngles(orientation, true);

    Serial.print("IMU readings: ");
    Serial.print(orientation.yaw);
    Serial.print(", ");
    Serial.print(orientation.pitch);
    Serial.print(", ");
    Serial.println(orientation.roll);

    // If the position has been updated, then send the new position to the core 0 which handles odometry updates to flask server
    if(odometer.updatePosition(orientation.yaw) == true){
        // Save the current platform position and HCSR data into the message type handled by the queue
        QueueMessage qMessage;

        qMessage.x = odometer.getXpos();
        qMessage.y = odometer.getYpos();
        qMessage.collision = permanentStop;
        qMessage.angle = orientation.yaw;

        Serial.print("Angle loaded to message: ");
        Serial.println(qMessage.angle);
        Serial.print("Collision status loaded to message: ");
        Serial.println(qMessage.collision);

        // Send the odometry data to the queue
        xQueueSend(Cores::manToHttpQ, (void*)(&qMessage), 0);
        xTaskNotifyGive(Cores::task0Handle);

        #ifdef DEBUG_ODOMETRY_PUTTY
          odometer.writeToCSV(Serial, ';'); // Write data to Serial output -> putty
        #endif
    }
  }

  /// @brief Checks if the #asyncServer has any new messages stored.
  ///        Handled messages are: 
  ///        - Lidar scan requests.
  ///        - Steering commands (simplified).
  ///        Based on server commands modifies #controllerMessage and #messageReceived in order
  ///        to cause similiar behaviours as when steering with a remote contrller. 
  void Manager::checkPCmessages(){
    // Handle the commands received from the server.
    if(asyncServer.isNewSteeringCommand()){
      // Retireve the new command, commnd goes stale after the read
      AsyncServerSpace::SteeringCommand newCommand;
      newCommand = asyncServer.getSteeringCommand();

      // Set the message
      portENTER_CRITICAL(&this->messageSpinlock);

      // Set the flag as message has been received
      this->setMessageReceived(true);

      // Set the joystick values according to the direction of driving
      // If direction is none, stop the platform
      if(newCommand.direction == "none"){
        this->controllerMessage.x = 0;
        this->controllerMessage.y = 0;
        this->controllerMessage.state = ManagerState::STANDBY;
      }
      if(newCommand.direction == "forward"){
        this->controllerMessage.y = 900;
        this->controllerMessage.x = 0;
        this->controllerMessage.state = ManagerState::MOVING;
      }
      else if(newCommand.direction == "backward"){
        this->controllerMessage.y = 100;
        this->controllerMessage.x = 0;
        this->controllerMessage.state = ManagerState::MOVING;
      }
      else if(newCommand.direction == "left"){
        this->controllerMessage.x = 900;
        this->controllerMessage.y = 0;
        this->controllerMessage.state = ManagerState::MOVING;
      }
      else if(newCommand.direction == "right"){
        this->controllerMessage.x = 100;
        this->controllerMessage.y = 0;
        this->controllerMessage.state = ManagerState::MOVING;
      }

      // Clear all other message options
      this->controllerMessage.start = false;
      this->controllerMessage.select = false;
      this->controllerMessage.x_b = false;
      this->controllerMessage.y_b = false;
      this->controllerMessage.b_b = false;
      this->controllerMessage.a_b = false;

      portEXIT_CRITICAL(&this->messageSpinlock);
      
    }
    else if(asyncServer.isNewScanRequest()){
      // Preapre the lidar (set num of rotations and every_nth)

      // Get the request contents
      AsyncServerSpace::ScanRequest scanRequest;
      scanRequest = asyncServer.getScanRequest();

      // Setup LiDAR for the scan
      this->lidarController.setDesiredScansNumber(scanRequest.rotations);
      this->lidarController.setEveryNthPoint(scanRequest.every_nth);

      // Set the approrpiate controllerMessage values
      portENTER_CRITICAL(&this->messageSpinlock);
      this->setMessageReceived(true);

      this->controllerMessage.state = ManagerState::SCANNING;

      this->controllerMessage.x = 0;
      this->controllerMessage.y = 0;

      this->controllerMessage.start = true;
      this->controllerMessage.select = false;
      this->controllerMessage.x_b = false;
      this->controllerMessage.y_b = false;
      this->controllerMessage.b_b = false;
      this->controllerMessage.a_b = false;

      portEXIT_CRITICAL(&this->messageSpinlock);


    }
  }

  // -------------- Rover Operations -------------- //

  void Manager::moveRover(){

    // Should this really be happening during each iteration or rather only when permanentStop == false ?
    engineController.applySteering(controllerMessage.x, controllerMessage.y);

    // Allow movement only if the flag which indicates presence of an obstacle is not raised.
    // Disable movement otherwise.
    if (permanentStop == false){
      //engineController.applySteering(controllerMessage.x, controllerMessage.y);
      engineController.update();
    }
    else{
      engineController.stop();
    }
  }

  /// @brief Updates the information in #odometer about the direction of spinning of the wheels.
  void Manager::updateOdometryDirection(){

    // Set the direction of left wheel
    if(engineController.getLeftSpeed() >= 0){
      odometer.setLeftWheelMotionDirection(Odometry::MotionDirection::FORWARD);
    }
    else{
      odometer.setLeftWheelMotionDirection(Odometry::MotionDirection::BACKWARD);
    }

    // Set the direction of right wheel
    if(engineController.getRightSpeed() >= 0){
      odometer.setRightWheelMotionDirection(Odometry::MotionDirection::FORWARD);
    }
    else{
      odometer.setRightWheelMotionDirection(Odometry::MotionDirection::BACKWARD);
    }

  }

  /// @brief Performs a lidar scan at fixed inclination 90 degrees.
  ///        In total 5 scans are done with default value of every_nth
  void Manager::lidarScan(){

    lidarController.setInclination(90.0f);
    lidarController.scanNtimes(5);
  }

  // -------------- Communication -------------- //

  /// @brief If there is a scan avaialable in #lidarController, uploads it to the PC via http request.
  ///        Lidar data is cleared in the #lidarController after sending it to pc in order to save memory.
  ///        Message content:
  ///        0. Lidar data.
  ///        1. Current position.
  ///        2. Heading (IMU data read).
  void Manager::transmitLidarDataToPC(){
    // Request IMU heading reading
    ICM_IMU::EulerAngles eulerAngles;
    imu.getEulerAngles(eulerAngles);

    std::vector<float>& lidarPoints = lidarController.accessData();

    if(lidarPoints.empty()){
      Serial.println("Lidar scan is empty, no transmission done.");
      return;
    }

    // Format the message into an http request
    String dataJson = httpCommunicator.packLidarDataToJSON(eulerAngles.yaw, odometer.getXpos(), odometer.getYpos(), lidarPoints);

    // DEBUG messages
    Serial.print("Stringified json: ");
    Serial.println(dataJson);

    // Send the http request
    httpCommunicator.sendLidarData(dataJson, getSetting("pc-server-post-endp"));

    // Clear vector with lidar data
    lidarController.clearPoints();
  }

  void Manager::kontrolerSendData(){
    esp_err_t result = esp_now_send(EspNowCallback::kontrolerAddress, (uint8_t *) &EspNowCallback::tx_message, sizeof(EspNowCallback::tx_message));

    if(result == ESP_OK){
      //Serial.print("Sent with success ");
    }else{
      //Serial.print("Error sending the data ");
    }
    //Serial.println("Transmitting gathered data");
    // Call to ESP NOW interface or whatever that is to transmit some data
  }

  // -------------- Getters & Setters -------------- //

  /// @brief Sets the value of #permanentStop.
  /// @param state New value of the #permanentStop.
  void Manager::setPermanentStop(bool state){
    this->permanentStop = state;
  }

  /// @brief Copies themessage received from the controller to #controllerMessage.
  void Manager::setControllerMessage(const EspNowCallback::Message& message){
    taskENTER_CRITICAL(&messageSpinlock);
    controllerMessage = message;
    taskEXIT_CRITICAL(&messageSpinlock);
  }

  /// @brief Returns a copy of the last received message.
  EspNowCallback::Message Manager::getControllerLastMessage(){
    taskENTER_CRITICAL(&messageSpinlock);
    EspNowCallback::Message temp = controllerMessage;
    taskEXIT_CRITICAL(&messageSpinlock);
    return temp;
  }

  /// @brief Returns the address of the #controllerMessage field which stores the message issued by the controller.
  /// @return Address of #controllerMessage.
  volatile EspNowCallback::Message* Manager::getControllerMessageLocation(){
    return &this->controllerMessage;
  }

  /// @brief Sets the status of #messageReceived flag.
  /// @param status true if message had been received and contents of #controllerMessage were changed.
  ///               False otherwise.
  void Manager::setMessageReceived(bool status){
    messageReceived = status;
  }

  /// @brief Sets the state of the main loop.
  /// @param newState State in which platform will be operating now.
  void Manager::setState(ManagerState newState){
    state = newState;
  }

  /// @brief Returns the current state of operation.
  /// @return State of the platform operation.
  ManagerState Manager::getState(){
    return state;
  }

  // /// @brief Accesses the odometry field.
  // /// @return Reference to the #odometer field.
  
  Odometry::Odometer2Wheel& Manager::accessOdometry(){
    return odometer;
  }

}
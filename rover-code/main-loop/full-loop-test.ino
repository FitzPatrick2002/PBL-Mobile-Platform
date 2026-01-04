/// @file mainloop-test.ino
/// @brief Contains the skeleton for main loop of the rover.

#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>

#include "Odometer2Wheel.h"
#include "HTTPCommunicator.h"
#include "LidarController.h"
#include "icm_imu.h"
#include "ArcadeDrive.h"
#include "AsyncServerSpace.h"

#include "Core0Manager.h"

//#include "silnik_mk1_1ax.h"

// Tutorials
// 0. https://www.freertos.org/message_passing_performance
// 1. https://freertos.org/Documentation/02-Kernel/04-API-references/06-Queues/00-QueueManagement
// 2. https://freertos.org/Documentation/02-Kernel/02-Kernel-features/02-Queues-mutexes-and-semaphores/01-Queues
// 3. https://freertos.org/Documentation/02-Kernel/04-API-references/06-Queues/00-QueueManagement

// 4. https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/network/esp_now.html

// Multiprocessing: 
// 5. https://randomnerdtutorials.com/esp32-dual-core-arduino-ide/

// TO DO:

// 0. getControllerLastMessage -> check if it needs to return volatile Message or just Message

//#define DEBUG_ODOMETRY_PUTTY ///< If platform is connected to pc with putty software, odometry data can be printed to serial out and saved by the putty software. 
//#define DEBUG_RECEIVE_MSG ///< When message is received via ESP NOW it is logged through UART. Comment this out to disable
//#define DEBUG_SAVE_MSG ///< When received message is processed (Manager state is updated) in Manager::listenToMessage()

/*
#define LEFT_ENCODER 17  ///< Odometry encoder on left wheel.
#define RIGHT_ENCODER 10 ///< Odmetry encoder on right wheel.
#define HCSR_STOP 1     ///< ISR pin for stopping the platform when obstacle was detected by ultrasonic sensors. 

// Lidar connection pins
#define LIDAR_EN 2
#define LIDAR_RX 7
#define LIDAR_TX 6
#define LIDAR_PWM 21

// Wifi setup
#define SSID "TELPOL-19886"
#define WIFI_PASSWORD "38j8gze9sh"
#define SERVER_NAME "http://192.168.21.17:9000"

// Server running on PC endpoints
#define SERVER_POST_ENDPOINT "/receive_post"

// Platform server endpoints
#define PLATFORM_COMMAND_ENDPOINT "/steering"
#define PLATFORM_SCAN_REQ_ENDPOINT "/lidar"

// I2C setup
#define I2C_SDA 11
#define I2C_SCL 12

// Multiprocessing queue setup
#define MAN_TO_HTTP_CAPACITY 50
#define MAN_TO_HTTP_MESSAGE_SIZE 2*sizeof(float)

// Configuration of L298N 
#define L_IN1 3 //5;   // kierunek silnik A
#define L_IN2 4 //6;   // kierunek silnik A
#define L_ENA 13 //18; // PWM silnik A

#define R_IN1 14 //7;  // kierunek silnik B
#define R_IN2 9 //8;   // kierunek silnik B
#define R_ENB 8 //21;  // PWM silnik B
*/

// Temporarily define this here 
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len);

//dc:06:75:f9:61:ac
uint8_t kontrolerAddress[] = {0xDC, 0x06, 0x75, 0xF9, 0x61, 0xAC}; //kontroler address for two way communication

esp_now_peer_info_t peerInfo; //kontroler board info
/// @brief Defines possible states of operation for the rover
enum ManagerState{
  STANDBY = 0,        ///< Rover is not performing any actions that could break communication.
  MOVING = 1,         ///< Rover is changing position using engines.
  SCANNING = 2,       ///< Rover is gathering environmental data, can't move now.
  UPLOADING = 3,      ///< Rover is uploading data to PC.
  STATUS_UPDATE = 4,  ///< Another state for uploading to controller.
  IMU_CALIBRATION = 5 ///< Rover is basically in idle state but imu is calibration mode.
  //UPLOADING_TO_PC = 5 ///< Platform is sending data to PC.
};

/// @brief Message struct is used to exchange data between rover and controller.
struct Message {
  int x, y = 0; ///< Analog values of joystick potentiometers in range (0, 1023).
  bool start, select, x_b, y_b, b_b, a_b = 0; ///< Status of controllers buttons.
  ManagerState state = ManagerState::STANDBY; ///<  State into which the mobile platform should transitions after receiving the message.

  // -------------------- Constructors -------------------- //

  Message(const volatile Message& mess){
    x = mess.x;
    y = mess.y;
    start = mess.start;
    select = mess.select;
    x_b = mess.x_b;
    y_b = mess.y_b;
    b_b = mess.b_b;
    a_b = mess.a_b;
    state = mess.state;
  }

  Message(int xx, int yy, bool start_, bool select_, bool x_butt, bool y_butt, bool b_butt, bool a_butt, ManagerState newState){
    x = xx;
    y = yy;
    start = start_;
    select = select_;
    x_b = x_butt;
    y_b = y_butt;
    b_b = b_butt;
    a_b = a_butt;
    state = newState;
  }

  Message(){
    x, y = 0;
    start, select, x_b, y_b, b_b, a_b = 0;
    state = ManagerState::STANDBY;
  }

  volatile Message& operator=(const Message& mess) volatile {
    x = mess.x;
    y = mess.y;
    start = mess.start;
    select = mess.select;
    x_b = mess.x_b;
    y_b = mess.y_b;
    b_b = mess.b_b;
    a_b = mess.a_b;
    state = mess.state;

    return *this;
  }

  
  // -------------------- Utility Methods -------------------- //

  /// @brief Returns size of the structure in bytes.
  /// @return Size of the structure in bytes.
  size_t getBytesLength(){
    return 18; //Nah I will make it better later on//sizeof(x) + sizeof(y) + sizeof(start) + sizeof(x) + sizeof(x) + sizeof(x) + sizeof(x) + sizeof(x) + sizeof(x) + sizeof(x) + sizeof(x) + 
    // It's only for testing anyways
  }

  /// @brief Prints the contents of the struct into a stream.
  /// @param stream Refernce to object that implements Stream class.
  /// @param sep Character separator, by default = ';'.
  volatile void printData(Stream &stream = Serial, char sep = ';'){
    stream.print(x);
    stream.print(sep);
    stream.print(y);
    stream.print(sep);

    stream.print(start);
    stream.print(sep);
    stream.print(select);
    stream.print(sep);
    stream.print(x_b);
    stream.print(sep);
    stream.print(y_b);
    stream.print(sep);
    stream.print(b_b);
    stream.print(sep);
    stream.print(a_b);
    stream.print(sep);

    stream.print((int)(state));
    stream.print(sep);
  }

};

struct platforma_message{
  bool is_scanning;
  String status_text;
};

platforma_message tx_message;

// -------------- Http Communication on Core 0 -------------- //

/*
TaskHandle_t task0Handle; ///< Handle to the http handling which runs on core 0.
QueueHandle_t manToHttpQ = NULL; ///< Handle to the queue with which Manager can send stuff to the http operator.

/// @brief Handles slow http requests on core 0 of the esp.
///        Uses 2 queues to communicate with core 1.
void handleHttpOnCore0(void *param){
  // 0. Await indefinitely if there is any data in the queue
  // 1. Read all data from the queue
  // 2. Pack it into a message 
  // 3. Create json
  // 4. Make http request

  // Store the platform position and handle to the queue.
  PlatformPosition position;
  QueueHandle_t queue = (QueueHandle_t)(param);

  // Message structure is as such:
  
  //{
  //  "data-type": "odometry",
  //  "payload": [x0, y0, x1, y1, ...]
  //}
  

  // Main loop of the core 0 process.
  // Handles extraction of data from the manToHttp queue and sending the data to server
  while(true){
    // Await for notification that there is new content in the queue that can be read
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    JsonDocument doc;
    doc["data-type"] = "odometry";
    JsonArray payload = doc["payload"].to<JsonArray>();

    // Read the queue as long as there is content in it
    bool hasData = false;
    while(xQueueReceive(queue, (&position), 0) == pdPASS){
      hasData = true;

      payload.add(position.x);
      payload.add(position.y);
    }

    // If position update has been received, send it via http
    if(hasData){
      
      // Stringify json and send to server
      doc.shrinkToFit();
      String jsonString;
      serializeJson(doc, jsonString);

      // Send the odometry data via POST
      WiFiClient client;
      HTTPClient http;
      if (http.begin(client, String(SERVER_NAME) + String(SERVER_POST_ENDPOINT))){
        http.addHeader("Content-type", "application/json");

        int httpResponseCode = http.POST(jsonString);
        http.end();

        Serial.println("Odometry data POST: Server response: ");
        Serial.println(httpResponseCode);
      }
    }

    // Wait 100 ms to let the system handle other tasks on this core (wifi, etc)
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
*/

class Manager{
private:

  // -------------- WiFi Communication -------------- //
  
  HTTP::HTTPCommunicator httpCommunicator{SSID, WIFI_PASSWORD, SERVER_NAME};       ///< Communicator that is used to make http requests (mainly for lidar scans).
  AsyncServerSpace::ServerHandler asyncServer;

  // -------------- Main Measurement Unit (LiDAR) -------------- //

  Lidar::LidarController& lidarController = Lidar::LidarController::getInstance(); ///< Controles the lidar. Manages the way scans are done, how many rotations or how many points are skipped. Stores the scan results.

  // -------------- Position and Orientation in Space -------------- //

  Odometry::Odometer2Wheel odometer{LEFT_ENCODER, RIGHT_ENCODER, 4, 25, 10, 150}; ///< Calculates the current position based on wheel turns and driving direction given by imu.
  ICM_IMU::IMU imu{Serial}; ///< Controls imu and provides to orientation quaternion. Retireval of orientation might be lengthy if last retrievel happened long ago.
  DoubleEngine engineController{L_IN1, L_IN2, L_ENA, R_IN1, R_IN2, R_ENB};        ///< Controls the speed and direction of rotation of engines.

  // -------------- Controller Messages -------------- //

  volatile Message controllerMessage;         ///< Stores message received from the Controller. 
  volatile bool messageReceived = false;      ///< Specifies if any new message has been received. It will be cleared after the new message has been processed.w

  // -------------- State Handling -------------- //

  ManagerState state = ManagerState::STANDBY; ///< Current state of the rover. 
  bool stateChanged = false;                  ///< Set when data is received from the controller

  // -------------- HCSR Info -------------- //

  volatile bool permanentStop = false;        ///< Informs about the status 

public:

  portMUX_TYPE messageSpinlock = portMUX_INITIALIZER_UNLOCKED; ///< Spinlock secures #controllerMessage and #messageReceived from races.

public:

  // -------------- Constructors & Destructors -------------- //

  Manager() {}
  ~Manager() {}

  // -------------------------------------------- //
  // -------------- Public Methods -------------- //
  // -------------------------------------------- //

  // -------------- Components Initialization -------------- //

  /// @brief Initilizes and start the asynchronous server #asyncServer. 
  ///        Use at the end or after lidar has been initialized.
  void initAsyncServer(){
    // Init the endpoints where requests will be send 
    asyncServer.initCommandEndpoint();
    asyncServer.initScanEndpoint();

    // Start the server
    asyncServer.begin();
  }

  /// @brief Wrapper for engines initilization.
  /// @see #engineController
  void initEngines(){
    engineController.initEngines();
  }

  /// @brief Initilizes lidar.
  void initLidar(){
    lidarController.setPinout(LIDAR_EN, LIDAR_RX, LIDAR_TX, LIDAR_PWM);
    lidarController.setInclination(90.0f); // Inclination in degrees
    lidarController.init();
  }

  /// @brief Initializes imu.
  void initIMU(){
    // Init imu to use dmp, set the last bit of i2c address to 1 and show debug messages
    imu.init(true, 1, true);
  }

  // -------------- Main Loop Actions -------------- //

  /// @brief Main operation loop.
  void mainLoop(){
    // Check if any message has been received
    listenToMessage(); 

    // Perform action based on current state and other flags
    performAction();

    // Some routines need to be run during every execution of the main loop
    // They are run in here
    runEveryStep(); 
  }

  /// @brief Performs action based on the current state.
  void performAction(){
    switch(state){
      case ManagerState::STANDBY:
        // Do nothing
        Serial.println("Standby");
        tx_message.is_scanning=false;
        tx_message.status_text="Standby";
        delay(200);
      break;

      case ManagerState::MOVING:
        moveRover();
        updateOdometryDirection();
        Serial.println("Moving");
        tx_message.is_scanning=false;
        tx_message.status_text="Moving";
        delay(200);
      break;

      case ManagerState::SCANNING:
        Serial.println("Scanning");

        // Disable the ESP NOW for the duration of the scan and re-enable it after the scan
        esp_now_unregister_recv_cb();
        lidarScan();
        esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));

        // Scan is performed once
        // After its done, platform is back to STANDBY state
        //setState(ManagerState::STANDBY);
        // vTaskDelay(pdMS_TO_TICKS(50)); // Let the core 0 finish http transmission

        setState(ManagerState::UPLOADING); // For now just upload data right after the scan
        delay(200);
        
        tx_message.is_scanning=true;
        tx_message.status_text="Scanning";
      break;
      
      case ManagerState::UPLOADING:
        Serial.println("Uploading");
        transmitLidarDataToPC();
        
        // After transmitting data to pc, platform switches to STANDBY state
        setState(ManagerState::STANDBY);
        delay(200);

        tx_message.is_scanning=false;
        tx_message.status_text="Uploading";
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

        tx_message.is_scanning=false;
        tx_message.status_text="Unknown state";
        delay(200);
      break;
    }
  }

  /// @brief If the message flag is set, processes the message and sets appropriate state (and the state change flag).
  ///        Whole function is protected by the #messageSpinlock. The message and the new message flag should not change during the whole execution.
  void listenToMessage(){
    taskENTER_CRITICAL(&messageSpinlock);
    if(messageReceived == true){
      // Process the message
      Message localCopyMessage = (Message)(controllerMessage);

      // If rover is moving and the new state does not allow it to move, stop it
      if(state == ManagerState::MOVING && localCopyMessage.state != ManagerState::MOVING){
        engineController.stop();
      }

      // If imu was in calibration mode and it was switched off, newly learned biases are saved to EEPROM
      if(state == ManagerState::IMU_CALIBRATION && localCopyMessage.state != ManagerStateIMU_CALIBRATION){
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
  void runEveryStep(){
    
    this->updateOdometry();

    // If any message arrived from the PC, check it
    this->checkPCmessages();
  }

  // -------------- Functions Invoked Every Main Loop Step -------------- //

  /// @brief Checks if odometry update happened.
  ///        If it did, the data is sent to the core 0 queue and transmitted to the server.
  void updateOdometry(){
    // Read the driving direction with regard to north
    ICM_IMU::EulerAngles orientation;
    imu.getEulerAngles(orientation, true);

    // If the position has been updated, then send the new position to the core 0 which handles odometry updates to flask server
    if(odometer.updatePosition(orientation.yaw) == true){
        // Save the current platform position into the message type handled by the queue
        PlatformPosition pos;
        pos.x = odometer.getXpos();
        pos.y = odometer.getYpos();

        // Send the odometry data to the queue
        xQueueSend(Cores::manToHttpQ, (void*)(&pos), 0);
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
  void checkPCmessages(){
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

  void moveRover(){

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
  void updateOdometryDirection(){

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
  void lidarScan(){

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
  void transmitLidarDataToPC(){
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
    httpCommunicator.sendLidarData(dataJson, SERVER_POST_ENDPOINT);

    // Clear vector with lidar data
    lidarController.clearPoints();
  }

  void kontrolerSendData(){
    esp_err_t result = esp_now_send(kontrolerAddress, (uint8_t *) &tx_message, sizeof(tx_message));

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
  void setPermanentStop(bool state){
    this->permanentStop = state;
  }

  /// @brief Copies themessage received from the controller to #controllerMessage.
  void setControllerMessage(const Message& message){
    taskENTER_CRITICAL(&messageSpinlock);
    controllerMessage = message;
    taskEXIT_CRITICAL(&messageSpinlock);
  }

  /// @brief Returns a copy of the last received message.
  Message getControllerLastMessage(){
    taskENTER_CRITICAL(&messageSpinlock);
    Message temp = controllerMessage;
    taskEXIT_CRITICAL(&messageSpinlock);
    return temp;
  }

  /// @brief Returns the address of the #controllerMessage field which stores the message issued by the controller.
  /// @return Address of #controllerMessage.
  volatile Message* getControllerMessageLocation(){
    return &this->controllerMessage;
  }

  /// @brief Sets the status of #messageReceived flag.
  /// @param status true if message had been received and contents of #controllerMessage were changed.
  ///               False otherwise.
  void setMessageReceived(bool status){
    messageReceived = status;
  }

  /// @brief Sets the state of the main loop.
  /// @param newState State in which platform will be operating now.
  void setState(ManagerState newState){
    state = newState;
  }

  /// @brief Returns the current state of operation.
  /// @return State of the platform operation.
  ManagerState getState(){
    return state;
  }

  // /// @brief Accesses the odometry field.
  // /// @return Reference to the #odometer field.
  
  Odometry::Odometer2Wheel& accessOdometry(){
    return odometer;
  }
  
};

// -------------- Global Objects -------------- //

// Create the manager class with main loop.
Manager manager;

// -------------- Interrupt Service Routines -------------- //

/// @brief ISR routine which increases number of rotation of the left wheel by 1 tick.
void IRAM_ATTR leftTick(){
  // Increases number of rotation of the left wheel by 1 tick.
  manager.accessOdometry().leftRotation();
}

/// @brief ISR routine which increases number of rotation of the right wheel by 1 tick.
void IRAM_ATTR rightTick(){
  // Increases number of rotation of the right wheel by 1 tick.
  manager.accessOdometry().rightRotation();
}

/// @brief ISR sets / clears the permanent stop flag in the Manager.
///        Invoked when HCSR sensors sense a close obstacle / sense the obstacle has been avoided.
void IRAM_ATTR collisionDetection(){
  // Read the value of the pin, if its down, then there is no imminent danger of collision
  bool status = digitalRead(HCSR_STOP);
  manager.setPermanentStop(status);
}

// -------------- Testing Stuff -------------- //

/// @brief Empties the serial buffer by sequentially reading bytes untill Serial.available() returns false.
void emptySerialBuffer(){
  while(Serial.available()){
    char c = Serial.read();
  }
}

/// @brief Tests if 
/// - Lidar is operational
/// - Imu reads data
/// - esp connects itself with the flask server
/// - http request succeeds
void testCommunicationWithFlask(){

  Serial.println("Changing state to standby");
  manager.setState(ManagerState::STANDBY);
  manager.mainLoop();
  manager.mainLoop();
  manager.mainLoop();
  delay(2000);
  Serial.println("Changing state to scanning");

  manager.setState(ManagerState::SCANNING);
  manager.mainLoop();
  manager.mainLoop(); // State should be back to standby
  manager.mainLoop();
  delay(2000);

  Serial.println("Changing state to uploading");
  manager.setState(ManagerState::UPLOADING);
  manager.mainLoop();
  manager.mainLoop(); // State should be back to STANDBY
  manager.mainLoop();
  delay(10000);
}

// -------------- ESP-NOW Setup -------------- //

/// @brief Callback used when esp receives message via ESP NOW protocol.
///        Copies received message into the manager.
///        In debug mode prints the message via UART.
/// @param mac Mac address of the receiver device.
/// @param incomingData Incoming data bytes.
/// @param len Number of bytes to read.
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  // Copy the received bytes into the appropriate field of the manager
  portENTER_CRITICAL_ISR(&manager.messageSpinlock);
  memcpy ((uint8_t *)(manager.getControllerMessageLocation()), incomingData, len);
  manager.setMessageReceived(true);

  // In debug mode, print the received message to the console
  #ifdef DEBUG_RECEIVE_MSG
    Message rx_message = manager.getControllerLastMessage();

    Serial.print("X: ");
    Serial.println(rx_message.x);
    Serial.print("Y: ");
    Serial.println(rx_message.y);
    Serial.print("Start: ");
    Serial.println(rx_message.start);
    Serial.print("Select: ");
    Serial.println(rx_message.select);
    Serial.print("X button: ");
    Serial.println(rx_message.x_b);
    Serial.print("Y button: ");
    Serial.println(rx_message.y_b);
    Serial.print("A button: ");
    Serial.println(rx_message.a_b);
    Serial.print("B button: ");
    Serial.println(rx_message.b_b);
    Serial.print("State: ");
    Serial.println(rx_message.state);
  #endif

  portEXIT_CRITICAL_ISR(&manager.messageSpinlock);
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t _status){
  manager.setState( ESP_NOW_SEND_SUCCESS ? ManagerState::STANDBY : ManagerState::STATUS_UPDATE);
  //Serial.println(_status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Failure");
}

// --------------Setup && Main Loop -------------- //

void setup() {
  // put your setup code here, to run once:

  // Setup serial communication
  Serial.begin(115200);
  while(!Serial){
    delay(10);
  }
  Serial.println("Serial Communication - OK");

  // Setup the Wire.h library for I2C communication
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(400000);
  Serial.println("Wire I2C - OK");

  delay(250);

  // Initialize IMU
  manager.initIMU();
  Serial.println("IMU - OK");

  delay(250);

  manager.initLidar();
  Serial.println("LiDAR - OK");

  delay(250);

  // Attach odometry interrupts
  attachInterrupt(manager.accessOdometry().getLeftPin(), leftTick, FALLING);
  attachInterrupt(manager.accessOdometry().getRightPin(), rightTick, FALLING);
  Serial.println("Odometry ISRs - OK");

  // Attach collision system detection interrupt
  attachInterrupt(HCSR_STOP, collisionDetection, CHANGE); 
  Serial.println("HCSR ISR - OK");

  delay(250);

  // Init wifi communication
  // ESP NOW requires that esp controller is a wifi station
  // http requests need to be done within a specified network

  // Setup device as wifi station
  WiFi.mode(WIFI_STA);

  WiFi.begin(SSID, WIFI_PASSWORD);
  Serial.println("Connecting to wifi");

  // Await to connect to wifi
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.println(".");
  }

  // Print the address of the esp in local wifi
  Serial.println("");
  Serial.print("Connected to WiFi network with IP addr: ");
  Serial.println(WiFi.localIP());

  Serial.println("WiFi - OK");

  delay(250);

  // Setup esp now protocol

  // In order to use wifi and esp now at the same time we need them to operate on the same channel
  int channel = WiFi.channel();
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  if (esp_now_init() != ESP_OK){
    Serial.println("ESP_NOW - ERROR");
    return;
  }
  else{
    Serial.println("ESP_NOW - OK");
  }

  WiFi.setSleep(false);

  // Register callback that will be called when receiver receives a message
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
  //two way communication - setting up sender
  esp_now_register_send_cb(esp_now_send_cb_t(OnDataSent));

  memcpy(peerInfo.peer_addr, kontrolerAddress,6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if(esp_now_add_peer(&peerInfo)!=ESP_OK){
    Serial.println("failed to add peer");
    return;
  }

  Serial.println("ESP NOW - OK");

  // Init the rover motors
  manager.initEngines();
  Serial.println("Motors - OK");

  // Init asynchronous server
  manager.initAsyncServer();
  Serial.println("Async Server - OK");

  // Initialize the core 0 routine

  // Init queue from manager to core 0
  Cores::initCore0Task(MAN_TO_HTTP_MESSAGE_SIZE, MAN_TO_HTTP_CAPACITY, 4000);
  /*
  manToHttpQ = xQueueCreate(MAN_TO_HTTP_CAPACITY, MAN_TO_HTTP_MESSAGE_SIZE);
  xTaskCreatePinnedToCore(handleHttpOnCore0, "core-0", 3500, manToHttpQ, 0, &task0Handle, 0);
  */
  Serial.println("Core 0 - OK");

  // Reset the odometry in case some interrupts missfired during setup
  delay(1000);
  manager.accessOdometry().reset();

  // Setup went well message
  Serial.println("Setup finished");
}

void loop() {
  // put your main code here, to run repeatedly:

  //testCommunicationWithFlask();

  manager.mainLoop();
}
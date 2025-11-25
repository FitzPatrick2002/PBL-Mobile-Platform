/// @file mainloop-test.ino
/// @brief Contains the skeleton for main loop of the rover.

//#include <esp_now.h>
//#include <WiFi.h>

// Tutorials
// 0. https://www.freertos.org/message_passing_performance
// 1. https://freertos.org/Documentation/02-Kernel/04-API-references/06-Queues/00-QueueManagement
// 2. https://freertos.org/Documentation/02-Kernel/02-Kernel-features/02-Queues-mutexes-and-semaphores/01-Queues
// 3. https://freertos.org/Documentation/02-Kernel/04-API-references/06-Queues/00-QueueManagement

// 4. https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/network/esp_now.html

// TO DO:
// 0. Copy the rest of the setup code from the receiver code from github
// 1. Test it on live esp-s
//    Adjust the code
// 2. Read about interrupts
// 3. Read about queus and parallel access to things
// 4. Process the data received from controller

// N. After testing lidar class incorporate it 
// N + 1. After testing doometry class, incorporate it

// Interrupts driven operations:
// - Data receiving
// - Encoders -> odometry
// - ?

// ^ Data accessed by the interrupt operations need to be protected from races

#define DEBUG_RECEIVE_MSG ///< When message is received via ESP NOW it is logged through UART. COmment this out to disable

/// @brief Defines possible states of operation for the rover
typedef enum ManagerState{
  STANDBY = 0,  ///< Rover is not performing any actions that could break communication
  MOVING = 1,   ///< Rover is changing position using engines.
  SCANNING = 2, ///< Rover is gathering environmental data, can't move now.
  UPLOADING = 3 ///< Rover is uploading data to PC.
  // Another state for uploading to controller
} op_mode;

/// @brief Message struct is used to exchange data between rover and controller.
typedef struct struct_message {
  int x, y; ///< Analog values of joystick potentiometers in range (0, 1023).
  bool start, select, x_button, y_button, b_button, a_button; ///< Status of controllers buttons.
  op_mode state; ///< Some esp32 specific thingie?
} struct_message;

class Manager{
private:
  // Odometry class
  // Lidar class
  // Obstacle detection class (HCSR)
  // Engines Controller class

  struct_message controllerMessage; ///< Stores message received from the Controller. 
  bool messageReceived = false; ///< Specifies if any message has been received.

  ManagerState state = ManagerState::STANDBY; ///< Current state of the rover. 
  bool stateChanged = false; ///< Set when data is received from the controller

public:

  // -------------- Constructors & Destructors -------------- //

  Manager() {}
  ~Manager() {}

  // -------------- Public Methods -------------- //

  /// @brief Main operation loop.
  void mainLoop(){
    listenToMessage(); // Check if any message has been received
    //updateState();     // ?
    performAction();
  }

  /// @brief Performs action based on the current state.
  void performAction(){
    switch(state){
      case ManagerState::STANDBY:
        // Do nothing
        delay(50);
        Serial.println("Standby");

      break;
      case ManagerState::MOVING:
        
        moveRover();
        Serial.println("Moving");
      break;
      case ManagerState::SCANNING:

        lidarScan();
        Serial.println("Scanning");
      break;
      case ManagerState::UPLOADING:
        
        transmitData();
        Serial.println("Uploading");
      break;
      default:
        
        Serial.println("Unknown state");
      break;
    }
  }

  /// @brief If the message flag is set, processes the message and sets appropriate state (and the state change flag)
  void listenToMessage(){
    if(messageReceived == true){
      // Process the message

      // Set the state based on message data (buttons status, joystick status, etc)
      // Mutex 
      //state = (ManagerState)(controllerMessage.newState);
      //state = ManagerState::STANDBY;
      Serial.println("Message is processed.");

      // Set the flag status = false, as message is already processed.
      messageReceived = false; 
    }
  }

/*
  /// @brief ?
  void updateState(){
    if(stateChanged == true){
      stateChanged = false; // Reset the flag

      // Process the received struct
      // e.g. 
      // To move we need:
      //  - Each motor speed, 
      // To scan we need:
      //  - Number of scans to make

      // Set the state
      // Semaphor
    }
  }
*/

  // -------------- Rover Operations -------------- //

  void moveRover(){

    Serial.println("ENgines are spinning: VROOOOM");
    // Call to the engines controller class
  }

  void lidarScan(){

    Serial.println("Scanning environment");
    // Call to the lidar Controller
  }

  // -------------- Communication -------------- //

  void transmitData(){

    Serial.println("Transmitting gathered data");
    // Call to ESP NOW interface or whatever that is to transmit some data
  }

  // -------------- Getters & Setters -------------- //

  /// @brief Copies the controller message to field @controllerMessage.
  void setControllerMessage(struct_message message){
    // Joystick potentiometers analog values
    controllerMessage.x = message.x;
    controllerMessage.y = message.y;

    // State of controller buttons
    controllerMessage.start = message.start; 
    controllerMessage.select = message.select;
    controllerMessage.x_button = message.x_button;
    controllerMessage.y_button = message.y_button;
    controllerMessage.b_button = message.b_button;
    controllerMessage.a_button = message.a_button;

    // Some state thingie
    controllerMessage.state = message.state;
  }

  /// @brief Returns a copy of the last received message.
  struct_message getControllerLastMessage(){
    return controllerMessage;
  }

  /// @brief Returns the address of the @controllerMessage field which stores the message issued by the controller.
  /// @return Address of #controllerMessage.
  struct_message* getControllerMessageLocation(){
    return &this->controllerMessage;
  }

  /// @brief Sets the status of @messageReceived flag.
  void setMessageReceived(bool status){
    messageReceived = status;
  }

  void setState(op_mode newState){
    state = newState;
  }

  ManagerState getState(){
    return state;
  }
};

// Create the manager class with main loop.
Manager manager;

/// @brief Callback used when esp receives message via ESP NOW protocol.
///        Copies received message into the manager.
///        In debug mode prints the message via UART.
/// @param mac Mac address of the receiver device.
/// @param incomingData Incoming data bytes.
/// @param len Number of bytes to read.
/*
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  // Copy the received bytes into the appropriate field of the manager
  memcpy (manager.getControllerMessageLocation(), incomingData, len);
  manager.setMessageReceived(true);

  // In debug mode, print all stuff into the console
  #ifdef DEBUG_RECEIVE_MSG
    struct_message rx_message = manager.getControllerLastMessage();

    Serial.print("X: ");
    Serial.println(rx_message.x);
    Serial.print("Y: ");
    Serial.println(rx_message.y);
    Serial.print("Start: ");
    Serial.println(rx_message.start);
    Serial.print("Select: ");
    Serial.println(rx_message.select);
    Serial.print("X button: ");
    Serial.println(rx_message.x_button);
    Serial.print("Y button: ");
    Serial.println(rx_message.y_button);
    Serial.print("A button: ");
    Serial.println(rx_message.a_button);
    Serial.print("B button: ");
    Serial.println(rx_message.b_button);
    Serial.print("State: ");
    Serial.println(rx_message.state);
  #endif
}
*/

void emptySerialBuffer(){
  while(Serial.available()){
    char c = Serial.read();
  }
}

// -------------- Rover Operations -------------- //

void setup() {
  // put your setup code here, to run once:

  // Setup serial communication
  Serial.begin(115200);
  while(!Serial){
    delay(10);
  }
  Serial.println("Working");

/*
  // Setup device as wifi station
  WiFi.mode(WIFI_STA);

  // Init esp-now
  if (esp_now_init != ESP_OK){
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register callback that will be called when receiver receives a message
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
  */

  // Setup went well message
  Serial.println("All went good");
}

ManagerState seqStates[5] = {ManagerState::STANDBY, ManagerState::SCANNING, ManagerState::MOVING, ManagerState::SCANNING, ManagerState::UPLOADING};
uint8_t counter = 0;

void loop() {
  // put your main code here, to run repeatedly:

  if(Serial.available()){
    char c = Serial.read();
    emptySerialBuffer();

    manager.setMessageReceived(true);
    manager.setState(seqStates[counter]);
    counter = (counter + 1) % 5;
    Serial.println((int)manager.getState());
  }

  //delay(500);
  manager.mainLoop();

}



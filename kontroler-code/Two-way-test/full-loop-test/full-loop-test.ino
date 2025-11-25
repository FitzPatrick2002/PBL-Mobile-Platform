/// @file mainloop-test.ino
/// @brief Contains the skeleton for main loop of the rover.

#include <esp_now.h>
#include <WiFi.h>

//#include "Odometer2Wheel.h"

// Tutorials
// 0. https://www.freertos.org/message_passing_performance
// 1. https://freertos.org/Documentation/02-Kernel/04-API-references/06-Queues/00-QueueManagement
// 2. https://freertos.org/Documentation/02-Kernel/02-Kernel-features/02-Queues-mutexes-and-semaphores/01-Queues
// 3. https://freertos.org/Documentation/02-Kernel/04-API-references/06-Queues/00-QueueManagement

// 4. https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/network/esp_now.html

// TO DO:
// 1. Test it on live esp-s
//    Adjust the code
// 2. Read about interrupts
// 3. Read about queus and parallel access to things
// 4. Process the data received from controller

// N. After testing lidar class incorporate it 
// N + 1. After testing doometry class, incorporate it

#define DEBUG_RECEIVE_MSG ///< When message is received via ESP NOW it is logged through UART. COmment this out to disable

//dc:06:75:f9:61:ac
uint8_t kontrolerAddress[] = {0xDC, 0x06, 0x75, 0xF9, 0x61, 0xAC}; //kontroler address for two way communication

esp_now_peer_info_t peerInfo; //kontroler board info
/// @brief Defines possible states of operation for the rover
enum ManagerState{
  STANDBY = 0,  ///< Rover is not performing any actions that could break communication
  MOVING = 1,   ///< Rover is changing position using engines.
  SCANNING = 2, ///< Rover is gathering environmental data, can't move now.
  UPLOADING = 3, ///< Rover is uploading data to PC.
  STATUS_UPDATE = 4 // Another state for uploading to controller
};

/// @brief Message struct is used to exchange data between rover and controller.
struct Message {
  int x, y = 0; ///< Analog values of joystick potentiometers in range (0, 1023).
  bool start, select, x_b, y_b, b_b, a_b = 0; ///< Status of controllers buttons.
  ManagerState state = ManagerState::STANDBY; ///< Some esp32 specific thingie?

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
// Make the spinlock local in the Odometer2Wheels
// Protext the data insode ISRs as well in that class.
//portMUX_TYPE odometrySpinlock = portMUX_INITIALIZER_UNLOCKED; ///< Protects the coders in #Odometry::Odometer2Wheel.

class Manager{
private:
  //Odometry::Odometer2Wheel odometer{0, 1, 10, 25, 10, 150};
  // Lidar class
  // Obstacle detection class (HCSR)
  // Engines Controller class

  volatile Message controllerMessage; ///< Stores message received from the Controller. 
  volatile bool messageReceived = false; ///< Specifies if any message has been received.

  ManagerState state = ManagerState::STANDBY; ///< Current state of the rover. 
  bool stateChanged = false; ///< Set when data is received from the controller

public:

  portMUX_TYPE messageSpinlock = portMUX_INITIALIZER_UNLOCKED; ///< Spinlock secures #controllerMessage and #messageReceived from races.

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
    //runEveryStep(); // Runs during each iteration through mainLoop()
  }

  /// @brief Performs action based on the current state.
  void performAction(){
    switch(state){
      case ManagerState::STANDBY:
        // Do nothing
        delay(500);
        Serial.println("Standby");
        tx_message.is_scanning=false;
        tx_message.status_text="Standby";
      break;
      case ManagerState::MOVING:
        delay(500);
        moveRover();
        Serial.println("Moving");
        tx_message.is_scanning=false;
        tx_message.status_text="Moving";
      break;
      case ManagerState::SCANNING:
        delay(500);
        lidarScan();
        Serial.println("Scanning");
        tx_message.is_scanning=true;
        tx_message.status_text="Scanning";
      break;
      case ManagerState::UPLOADING:
        delay(500);
        transmitData();
        Serial.println("Uploading");
        tx_message.is_scanning=false;
        tx_message.status_text="Uploading";
      break;
      case ManagerState::STATUS_UPDATE:
        delay(500);
        kontrolerSendData();
        Serial.println("Status Update");
      break;
      default:
        delay(500);
        tx_message.is_scanning=false;
        Serial.println("Unknown state");
        tx_message.status_text="Unknown state";
      break;
    }
  }

  /// @brief If the message flag is set, processes the message and sets appropriate state (and the state change flag).
  ///        Whole function is protected by the #messageSpinlock. The message and the flag should not change during the whole execution.
  void listenToMessage(){
    taskENTER_CRITICAL(&messageSpinlock);
    if(messageReceived == true){
      // Process the message
      Message localCopyMessage = (Message)(controllerMessage);
      Serial.println("Message received: ");
      localCopyMessage.printData(Serial, ',');

      // Set the state based on message data (buttons status, joystick status, etc)
      state = localCopyMessage.state;
      Serial.print("State set to:");
      Serial.println((int)(state));

      // Set the flag status = false, as message is already processed.
      messageReceived = false; 
    }
    taskEXIT_CRITICAL(&messageSpinlock);
  }

  /// @brief Invokes routines necessary during every iteration through the main loop.
  ///        Odometry position update.
  void runEveryStep(){
    //odometer.updatePosition();
  }

  // -------------- Rover Operations -------------- //

  void moveRover(){

    //Serial.println("ENgines are spinning: VROOOOM");
    // Call to the engines controller class
  }

  void lidarScan(){

    //Serial.println("Scanning environment");
    // Call to the lidar Controller
  }

  // -------------- Communication -------------- //

  void transmitData(){

    //Serial.println("Transmitting gathered data");
    // Call to ESP NOW interface or whatever that is to transmit some data
  }

  void kontrolerSendData(){
    
    esp_err_t result = esp_now_send(kontrolerAddress, (uint8_t *) &tx_message, sizeof(tx_message));

    if(result == ESP_OK){
      Serial.print("Sent with success ");
    }else{
      Serial.print("Error sending the data ");
    }
    //Serial.println("Transmitting gathered data");
    // Call to ESP NOW interface or whatever that is to transmit some data
  }

  // -------------- Getters & Setters -------------- //

  /// @brief Copies the controller message to field #controllerMessage.
  void setControllerMessage(const Message& message){
    // Joystick potentiometers analog values
    /*
    controllerMessage.x = message.x;
    controllerMessage.y = message.y;

    // State of controller buttons
    controllerMessage.start = message.start; 
    controllerMessage.select = message.select;
    controllerMessage.x_b = message.x_b;
    controllerMessage.y_b = message.y_b;
    controllerMessage.b_b = message.b_b;
    controllerMessage.a_b = message.a_b;

    // Some state thingie
    controllerMessage.state = message.state;
    */
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

  /// @brief Returns the address of the @controllerMessage field which stores the message issued by the controller.
  /// @return Address of #controllerMessage.
  volatile Message* getControllerMessageLocation(){
    return &this->controllerMessage;
  }

  /// @brief Sets the status of @messageReceived flag.
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
  // Odometry::Odometer2Wheel& accessOdometry(){
  //   return odometer;
  // }

};

// -------------- Global Objects -------------- //

// Create the manager class with main loop.
Manager manager;

/// @brief ISR routine which increases number of rotation of the left wheel by 1 tick.
void IRAM_ATTR leftTick(){
  // Increases number of rotation of the left wheel by 1 tick.
  //manager.accessOdometry().leftRotation();
}

/// @brief ISR routine which increases number of rotation of the right wheel by 1 tick.
void IRAM_ATTR rightTick(){
  // Increases number of rotation of the right wheel by 1 tick.
  //manager.accessOdometry().rightRotation();
}

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
  Serial.println(_status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Failure");
}

// -------------- Testing Stuff -------------- //

/// @brief Empties the serial buffer by sequentially reading bytes untill Serial.available() returns false.
void emptySerialBuffer(){
  while(Serial.available()){
    char c = Serial.read();
  }
}

void setup() {
  // put your setup code here, to run once:

  // Setup serial communication
  Serial.begin(115200);
  while(!Serial){
    delay(10);
  }
  Serial.println("Serial Communication - OK");

  // Attach odometry interrupts
  //attachInterrupt(manager.accessOdometry().getLeftPin(), leftTick, FALLING);
  //attachInterrupt(manager.accessOdometry().getRightPin(), rightTick, FALLING);

  // Setup device as wifi station
  WiFi.mode(WIFI_STA);

  // Init esp-now
  if (esp_now_init() != ESP_OK){
    Serial.println("ESP_NOW - ERROR");
    return;
  }
  else{
    Serial.println("ESP_NOW - OK");
  }

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

  // Reset the odometry in case some interrupts missfired during setup
  delay(1000);
  //manager.accessOdometry().reset();

  // Setup went well message
  Serial.println("Setup finished");
}

void loop() {
  // put your main code here, to run repeatedly:

  manager.mainLoop();
}



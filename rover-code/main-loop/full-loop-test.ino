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
#include "esp-now-callbacks.h"

#include "TheSetuper.h"

#include "ManagerSpace.h"

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



// -------------- Global Objects -------------- //

// Create the manager class with main loop.
ManagerSpace::Manager manager(Lidar::LidarController::getInstance());

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

// --------------Setup && Main Loop -------------- //

void setup() {
  // put your setup code here, to run once:

  // Setup serial communication
  Serial.begin(115200);
  while(!Serial){
    delay(10);
  }
  Serial.println("Serial Communication - OK");

  delay(250);

  // Init TheSetuper
    // Init the communication stream of TheSetuper as Serial (this can be done only once)
  Setup::TheSetuper::getSetuper(&Serial);

  // Run the setup loop
  Setup::TheSetuper::getSetuper()->theSetup();

  // Init wifi communication
  // ESP NOW requires that esp controller is a wifi station
  // http requests need to be done within a specified network

  // Setup device as wifi station
  WiFi.mode(WIFI_STA);

  WiFi.begin(getSetting("wifi-ssid"), getSetting("wifi-password"));
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

  // Once the Serial & WiFi are initalized, setup the needed variables

  // Setup the automatic variables (MAC & IP)
  Setup::TheSetuper::getSetuper()->init();

  // Run the setup loop for the 2nd time
  Setup::TheSetuper::getSetuper()->theSetup();

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
  esp_now_register_recv_cb(esp_now_recv_cb_t(EspNowCallback::OnDataRecv));
  //two way communication - setting up sender
  esp_now_register_send_cb(esp_now_send_cb_t(EspNowCallback::OnDataSent));

  // Read the controllers MAC address
  String macString = getSetting("controller-mac");
  Setup::TheSetuper::getSetuper()->getMACfromString(EspNowCallback::kontrolerAddress, macString);

  memcpy(EspNowCallback::peerInfo.peer_addr, EspNowCallback::kontrolerAddress, 6);
  EspNowCallback::peerInfo.channel = 0;
  EspNowCallback::peerInfo.encrypt = false;

  if(esp_now_add_peer(&EspNowCallback::peerInfo)!=ESP_OK){
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
#ifndef MANAGER_SPACE_H
#define MANAGER_SPACE_H
/// @file ManagerSpace.h
/// @brief Stores some classes, structs and functions used by the Manager.
///        Also contains the main loop for core 0 routine. 
#include "ManagerStates.h"
#include "Odometer2Wheel.h"
#include "HTTPCommunicator.h"
#include "LidarController.h"
#include "icm_imu.h"
#include "ArcadeDrive.h"
#include "AsyncServerSpace.h"
#include <Arduino.h>

#include "Core0Manager.h"
#include "esp-now-callbacks.h"

namespace ManagerSpace{

  class Manager{
  private:

    // -------------- WiFi Communication -------------- //
    
    HTTP::HTTPCommunicator httpCommunicator{MY_WIFI_SSID, WIFI_PASSWORD, SERVER_NAME};      ///< Communicator that is used to make http requests (mainly for lidar scans).
    AsyncServerSpace::ServerHandler asyncServer;

    // -------------- Main Measurement Unit (LiDAR) -------------- //

    Lidar::LidarController& lidarController; ///< Controles the lidar. Manages the way scans are done, how many rotations or how many points are skipped. Stores the scan results.

    // -------------- Position and Orientation in Space -------------- //

    Odometry::Odometer2Wheel odometer{LEFT_ENCODER, RIGHT_ENCODER, 4, 25, 10, 150}; ///< Calculates the current position based on wheel turns and driving direction given by imu.
    ICM_IMU::IMU imu{Serial}; ///< Controls imu and provides to orientation quaternion. Retireval of orientation might be lengthy if last retrievel happened long ago.
    DoubleEngine engineController{L_IN1, L_IN2, L_ENA, R_IN1, R_IN2, R_ENB};        ///< Controls the speed and direction of rotation of engines.

    // -------------- Controller Messages -------------- //

    volatile EspNowCallback::Message controllerMessage;         ///< Stores message received from the Controller. 
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

    Manager(Lidar::LidarController& lidarControllerInstance);
    ~Manager();

    // -------------------------------------------- //
    // -------------- Public Methods -------------- //
    // -------------------------------------------- //

    // -------------- Components Initialization -------------- //

    /// @brief Initilizes and start the asynchronous server #asyncServer. 
    ///        Use at the end or after lidar has been initialized.
    void initAsyncServer();

    /// @brief Wrapper for engines initilization.
    /// @see #engineController
    void initEngines();

    /// @brief Initilizes lidar.
    void initLidar();

    /// @brief Initializes imu.
    void initIMU();

    // -------------- Main Loop Actions -------------- //

    /// @brief Main operation loop.
    void mainLoop();

    void printIMUdata();

    /// @brief Performs action based on the current state.
    void performAction();

    /// @brief If the message flag is set, processes the message and sets appropriate state (and the state change flag).
    ///        Whole function is protected by the #messageSpinlock. The message and the new message flag should not change during the whole execution.
    void listenToMessage();

    /// @brief Invokes routines necessary during every iteration through the main loop.
    ///        1. Odometry position update.
    ///        2. Send odometry update to core 0.
    ///        3. Check if there is update from flask server on pc.
    void runEveryStep();

    // -------------- Functions Invoked Every Main Loop Step -------------- //

    /// @brief Checks if odometry update happened.
    ///        If it did, the data is sent to the core 0 queue and transmitted to the server.
    void updateOdometry();

    /// @brief Checks if the #asyncServer has any new messages stored.
    ///        Handled messages are: 
    ///        - Lidar scan requests.
    ///        - Steering commands (simplified).
    ///        Based on server commands modifies #controllerMessage and #messageReceived in order
    ///        to cause similiar behaviours as when steering with a remote contrller. 
    void checkPCmessages();

    // -------------- Rover Operations -------------- //

    void moveRover();

    /// @brief Updates the information in #odometer about the direction of spinning of the wheels.
    void updateOdometryDirection();

    /// @brief Performs a lidar scan at fixed inclination 90 degrees.
    ///        In total 5 scans are done with default value of every_nth
    void lidarScan();

    // -------------- Communication -------------- //

    /// @brief If there is a scan avaialable in #lidarController, uploads it to the PC via http request.
    ///        Lidar data is cleared in the #lidarController after sending it to pc in order to save memory.
    ///        Message content:
    ///        0. Lidar data.
    ///        1. Current position.
    ///        2. Heading (IMU data read).
    void transmitLidarDataToPC();

    void kontrolerSendData();

    // -------------- Getters & Setters -------------- //

    /// @brief Sets the value of #permanentStop.
    /// @param state New value of the #permanentStop.
    void setPermanentStop(bool state);

    /// @brief Copies themessage received from the controller to #controllerMessage.
    void setControllerMessage(const EspNowCallback::Message& message);

    /// @brief Returns a copy of the last received message.
    EspNowCallback::Message getControllerLastMessage();

    /// @brief Returns the address of the #controllerMessage field which stores the message issued by the controller.
    /// @return Address of #controllerMessage.
    volatile EspNowCallback::Message* getControllerMessageLocation();

    /// @brief Sets the status of #messageReceived flag.
    /// @param status true if message had been received and contents of #controllerMessage were changed.
    ///               False otherwise.
    void setMessageReceived(bool status);

    /// @brief Sets the state of the main loop.
    /// @param newState State in which platform will be operating now.
    void setState(ManagerState newState);

    /// @brief Returns the current state of operation.
    /// @return State of the platform operation.
    ManagerState getState();

    // /// @brief Accesses the odometry field.
    // /// @return Reference to the #odometer field.
    
    Odometry::Odometer2Wheel& accessOdometry();
    
  };

};

#endif
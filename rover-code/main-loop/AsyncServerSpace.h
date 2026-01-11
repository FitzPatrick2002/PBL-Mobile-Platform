/// @file AsyncServerSpace.h
/// @brief Provides an async server which can accept http requests and structs which define type of received data.

#ifndef ASYNC_SERVER_SPACE_H
#define ASYNC_SERVER_SPACE_H

#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <Arduino.h>

#include "TheSetuper.h"

/// @brief Stores server related tasks / classes / structs.
namespace AsyncServerSpace{

  /// @brief Stores data of a steering commands
  struct SteeringCommand{
    String direction = ""; ///< Direction of driving.
    bool isNew = false;    ///< Specifies if the command has not been read yet.
  };

  /// @brief Stores data of a lidar scan request
  struct ScanRequest{
    int rotations = 0;  ///< Number of rotations to perform.
    int every_nth = 0;  ///< Every which point will be included in the scan.
    bool isNew = false; ///< Specifies if the command has not been read yet.
  };

  /// @brief Initializes asynchronous web server.
  ///        Availabale endpoints are /command which is used to receive steering commands from flask
  ///        and /lidar which accpets lidar scanning requests.
  ///        Server port is fixed at 80.
  class ServerHandler{
  private:
    AsyncWebServer server{80};       ///< Main asynchronous webserver

    SteeringCommand steeringCommand; ///< Last received steering command.
    ScanRequest scanRequest;         ///< Last received scan request

    portMUX_TYPE steeringCommandSpinlock = portMUX_INITIALIZER_UNLOCKED; ///< Spinlock which protect the #steeringCommand from being accessed at the same time by the server and some Manager / other process.
    portMUX_TYPE scanRequestSpinlock = portMUX_INITIALIZER_UNLOCKED;     ///< Spinlock which protect the #scanRequest from being accessed at the same time by the server and some Manager / other process.

  public:

    // ------------ Contructor & Destructor ------------ //

    /// @brief Empty.
    ServerHandler();

    /// @brief Empty.
    ~ServerHandler();

    // ------------ Endpoints Initialization ------------ //

    /// @brief Initializes the server endpoint where steering commands are received.
    ///        Accepts POST requests in form {type: "steering", direction: "str"}.
    void initCommandEndpoint();

    /// @brief Initializes the server endpoint where lidar scan requests are received.
    ///        Accepts POST requests in form {type: "scan", rotations: int, every_nth: int}.
    void initScanEndpoint();

    // ------------ Data Retrieval ------------ //

    /// @brief Determine if the steering command is fresh.
    bool isNewSteeringCommand();

    /// @brief Determine if scan request is fresh.
    bool isNewScanRequest();

    /// @brief Returns copy of the last received steering command and sets it as read (isNew = false).
    SteeringCommand getSteeringCommand();

    /// @brief Returns copy of the last received scan request and sets it as read (isNew = false).
    ScanRequest getScanRequest();

    // ------------ Starting the Server ------------ //

    /// @brief Starts the server.
    void begin();

  };
};

#endif
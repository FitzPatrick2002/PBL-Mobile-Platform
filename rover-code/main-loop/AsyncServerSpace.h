#ifndef ASYNC_SERVER_SPACE_H
#define ASYNC_SERVER_SPACE_H

#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <Arduino.h>

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
    ServerHandler() {}

    /// @brief Empty.
    ~ServerHandler() {}

    // ------------ Endpoints Initialization ------------ //

    /// @brief Initializes the server endpoint where steering commands are received.
    ///        Accepts POST requests in form {type: "steering", direction: "str"}.
    void initCommandEndpoint(){
      // Start the server endpoint
      server.on("/command", HTTP_POST, [this](AsyncWebServerRequest *request){

        // If request has correct fields, service it
        if(request->hasParam("type", true) && request->hasParam("direction", true)){
          // read the tupe of the request
          String type = request->getParam("type", true)->value();

          // If it is of type steering update the currently sotred command
          if (type == "steering"){
            String direction = request->getParam("direction", true)->value();

            Serial.println("Steering command received");

            portENTER_CRITICAL_ISR(&steeringCommandSpinlock);
            // Save the command data
            this->steeringCommand.isNew = true;
            this->steeringCommand.direction = direction;
            portEXIT_CRITICAL_ISR(&steeringCommandSpinlock);

            // Send positive response
            request->send(200, "text/plain", "Steering command received - OK");
          }
          else{
            request->send(400, "text/plain", (String("Bad Request: Type should be: steering but was: ") + type).c_str());
          }
        }
        else{
          request->send(400, "text/plain", "Bad Request: Missing Params");
        }
      });
    }

    /// @brief Initializes the server endpoint where lidar scan requests are received.
    ///        Accepts POST requests in form {type: "scan", rotations: int, every_nth: int}.
    void initScanEndpoint(){
      // Start the server endpoint
      server.on("/scan", HTTP_POST, [this](AsyncWebServerRequest *request){

        // If request has appropriate fields, service it
        if(request->hasParam("type", true) && request->hasParam("rotations", true) && request->hasParam("every_nth", true)){
          String type = request->getParam("type", true)->value();

          // If the request type == 'scan', service the request
          if(type == "scan"){
            String rotations = request->getParam("rotations", true)->value();
            String every_nth = request->getParam("every_nth", true)->value();

            Serial.println("Scan command received");

            portENTER_CRITICAL_ISR(&scanRequestSpinlock);
            // Save parameters of the request and mark it as new
            this->scanRequest.every_nth = every_nth.toInt();
            this->scanRequest.rotations = rotations.toInt();
            this->scanRequest.isNew = true;
            portEXIT_CRITICAL_ISR(&scanRequestSpinlock);

            // Send positive response
            request->send(200, "text/plain", "Scan command received - OK");
          }
          else{
            request->send(400, "text/plain", (String("Bad Request: Type should be: lidar but was: ") + type).c_str());
          }
        }
        else{
          request->send(400, "text/plain", "Bad Request: Missing Params");
        }
      });
    }

    // ------------ Data Retrieval ------------ //

    /// @brief Determine if the steering command is fresh.
    bool isNewSteeringCommand(){
      bool isNew;

      portENTER_CRITICAL(&steeringCommandSpinlock);
      isNew = this->steeringCommand.isNew;
      portEXIT_CRITICAL(&steeringCommandSpinlock);

      return isNew;
    }

    /// @brief Determine if scan request is fresh.
    bool isNewScanRequest(){
      bool isNew;

      portENTER_CRITICAL(&scanRequestSpinlock);
      isNew = this->scanRequest.isNew;
      portEXIT_CRITICAL(&scanRequestSpinlock);

      return isNew;
    }

    /// @brief Returns copy of the last received steering command and sets it as read (isNew = false).
    SteeringCommand getSteeringCommand(){
      SteeringCommand comm;

      portENTER_CRITICAL(&steeringCommandSpinlock);
      this->steeringCommand.isNew = false;
      comm = this->steeringCommand;
      portEXIT_CRITICAL(&steeringCommandSpinlock);

      return comm;
    }

    /// @brief Returns copy of the last received scan request and sets it as read (isNew = false).
    ScanRequest getScanRequest(){
      ScanRequest scan;

      portENTER_CRITICAL(&scanRequestSpinlock);
      this->scanRequest.isNew = false;
      scan = this->scanRequest;
      portEXIT_CRITICAL(&scanRequestSpinlock);

      return scan;
    }

    // ------------ Starting the Server ------------ //

    /// @brief Starts the server.
    void begin(){
      server.begin();
    }

  };
};

#endif
/// @file AsyncServerSpace.cpp
/// @brief Implementation of elements from AsyncServerSpace.h.

#include "AsyncServerSpace.h"

namespace AsyncServerSpace{

  // ------------------------------------------------- //
  // ------------- SERVER HANDLER CLASS -------------- //
  // ------------------------------------------------- //

  // ------------ Contructor & Destructor ------------ //

  ServerHandler::ServerHandler() {}

  ServerHandler::~ServerHandler() {}

  // ------------ Endpoints Initialization ------------ //

  void ServerHandler::initCommandEndpoint(){
    // Start the server endpoint
    String endpoint = "/" + getSetting("platform-server-comm-endp");
    server.on(endpoint.c_str() , HTTP_POST, [this](AsyncWebServerRequest *request){

      // If request has correct fields, service it
      if(request->hasParam("type", true) && request->hasParam("direction", true)){
        // read the tupe of the request
        String type = request->getParam("type", true)->value();

        // If it is of type steering update the currently sotred command
        if (type == "steering"){
          String direction = request->getParam("direction", true)->value();

          Serial.println("Steering command received,");
          Serial.print("Direction: ");
          Serial.println(direction);

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

  void ServerHandler::initScanEndpoint(){
    // Start the server endpoint
    String endpoint = "/" + getSetting("platform-server-scan-endp");
    server.on(endpoint, HTTP_POST, [this](AsyncWebServerRequest *request){

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

  bool ServerHandler::isNewSteeringCommand(){
    bool isNew;

    portENTER_CRITICAL(&steeringCommandSpinlock);
    isNew = this->steeringCommand.isNew;
    portEXIT_CRITICAL(&steeringCommandSpinlock);

    return isNew;
  }

  bool ServerHandler::isNewScanRequest(){
    bool isNew;

    portENTER_CRITICAL(&scanRequestSpinlock);
    isNew = this->scanRequest.isNew;
    portEXIT_CRITICAL(&scanRequestSpinlock);

    return isNew;
  }

  SteeringCommand ServerHandler::getSteeringCommand(){
    SteeringCommand comm;

    portENTER_CRITICAL(&steeringCommandSpinlock);
    this->steeringCommand.isNew = false;
    comm = this->steeringCommand;
    portEXIT_CRITICAL(&steeringCommandSpinlock);

    return comm;
  }

  ScanRequest ServerHandler::getScanRequest(){
    ScanRequest scan;

    portENTER_CRITICAL(&scanRequestSpinlock);
    this->scanRequest.isNew = false;
    scan = this->scanRequest;
    portEXIT_CRITICAL(&scanRequestSpinlock);

    return scan;
  }

  // ------------ Starting the Server ------------ //

  void ServerHandler::begin(){
    server.begin();
  }
};
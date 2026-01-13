/// @file  HTTPCommunicator.cpp
/// @brief Implements all functions from HTTPCommunicator.h.

#include "HTTPCommunicator.h"

namespace HTTP{
// --------------- Constructors --------------- //

  HTTPCommunicator::HTTPCommunicator(String ssid, String password, String serverName){
      this->ssid = ssid;
      this->password = password;
      this->serverName = serverName;
  }

  // --------------- Setup --------------- //

  void HTTPCommunicator::resetVariables(String newSsid, String newPassword, String newServerName){
    this->ssid = newSsid;
    this->password = newPassword;
    this->serverName = newServerName;
  }

  void HTTPCommunicator::setupWiFiConnection(Stream& stream){

      // Inits a WiFi connection
      WiFi.begin(ssid.c_str(), password.c_str());
      stream.println("Connecting");

      // Keep on tryin to connect to WiFi till success
      while(WiFi.status() != WL_CONNECTED){
          delay(500);
          stream.print(".");
      }

      // Successful connection. Print the IP assigned to the esp.
      stream.println("");
      stream.print("Connected to WiFi network with IP addr: ");
      stream.println(WiFi.localIP());
  }

  // --------------- Communication --------------- //

  bool HTTPCommunicator::sendLidarData(String& jsonLidar, String dest){

      // If there is connection with the WiFi, try to send data to the server
      if(WiFi.status() == WL_CONNECTED){
          // Create the clients
          WiFiClient client;
          HTTPClient http;

          // Begin the http communication with specified server
          String destinationServer = "http://" +  serverName + "/" + dest;
          
          Serial.println("Making http request (lidar data): ");
          Serial.print(destinationServer);

          http.begin(client, destinationServer);

          // Specify the type of transmitted data and the data itself
          http.addHeader("Content-type", "application/json");
          int httpResponseCode = http.POST(jsonLidar);


          // Print the response code
          Serial.print("LiDAR data POST: Server response: ");
          Serial.println(httpResponseCode);

          // Finish the http request. Why was it gone?
          http.end(); 

          // If error occured, return false
          if (httpResponseCode < 0){
              return false;
          }
      }
      else{
          // If there was no connection just return false
          Serial.println("LiDAR data POST: Connection with server was lost.");
          return false;
      }

      // All went good, return true
      return true;
  }

  // --------------- Data Operations --------------- //

  String HTTPCommunicator::packLidarDataToJSON(float bearing, float scanX, float scanY, std::vector<float>& lidarData) {
      JsonDocument doc;
      
      // Specify type of data
      doc["data-type"] = "lidar";

      // Specify the position at which the scan was taken
      doc["position"].add(scanX);
      doc["position"].add(scanY);

      // Bearing 
      doc["bearing"] = bearing;

      // Add the main payload, that is data from lidar
      JsonArray data = doc["payload"].to<JsonArray>();
      for (float d : lidarData)
          data.add(d);
        
      // Release overallocated memory
      doc.shrinkToFit(); 

      // Serialize json into string
      //char output[4096]; // Half-assed lol
      String output;
      serializeJson(doc, output);

      return String(output);
  }
}
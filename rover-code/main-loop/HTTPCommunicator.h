#ifndef HTTP_COMMUNICATOR_H
#define HTTP_COMMUNICATOR_H

/// @file HTTPCommunicator.h
/// @brief HTTPCommunicator can be used to pack lidar scans data into a json file 
///        and to send it via a POST request to specified server.

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <vector>
#include <Arduino.h>

// TO DO:
// 1. Make the HTTPCommunicator a singleton, there should be no more than a single instance of it.

namespace HTTP{
    
    /// @brief Used to transform measurement data from lidar and odometry into http POST requests 
    ///        and send them to specified server within a given wifi network.
    class HTTPCommunicator{
    private:
        String ssid;       ///< WiFi ssid
        String password;   ///< Password to the WiFi
        String serverName; ///< Name of the server with which communication will be established.

    public:
        // --------------- Constructors --------------- //

        /// @brief Constructs the HTTPCommunicator object. 
        /// @param ssid WiFis ssid code.
        /// @param password Password to the WiFi.
        /// @param serverName Name of the server to which data will be sent.
        HTTPCommunicator(String ssid, String password, String serverName){
            this->ssid = ssid;
            this->password = password;
            this->serverName = serverName;
        }

        // --------------- Setup --------------- //
        
        /// @brief Establishes the connection with WiFi network.
        ///        Use it in void setup(). 
        ///        Prints the connection status to specified stream.
        /// @param stream Stream to which information about info about the connection is printed.
        ///               Stream such as Serial needs to be initilized, there is no checking of that.
        void setupWiFiConnection(Stream& stream){

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

        /// @brief Sends a jsonified data collected by lidar to the server.
        /// @see #packLidarDataToJSON
        /// @param jsonLidar Lidar data jsonified with function packLidarDataToJSON.
        /// @param dest Destination on the server, the uh final site? serverName + dest -> final destination.
        ///             Remember to make it like this: dest = "/some-destination"
        /// @return If transmission was successfull, return true. If not, returns false.
        bool sendLidarData(String& jsonLidar, String dest){

            // If there is connection with the WiFi, try to send data to the server
            if(WiFi.status() == WL_CONNECTED){
                // Create the clients
                WiFiClient client;
                HTTPClient http;

                // Begin the http communication with specified server
                String destinationServer = serverName + dest;
                http.begin(client, destinationServer);

                // Specify the type of transmitted data and the data itself
                http.addHeader("Content-type", "application/json");
                int httpResponseCode = http.POST(jsonLidar);

                // Print the response code
                Serial.print("LiDAR data POST: Server response: ");
                Serial.println(httpResponseCode);

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

        /// @brief Packs a lidar scan and data associated with it into a json file. JSON format:
        ///             "data-type" : "lidar"
        ///             "position"  : [scanX, scanY]
        ///             "bearing"   : bearing value
        ///             "payload"   : [r0, phi0, theta0, r1, phi1, theta1, ...] <- lidarData
        /// @param bearing Bearing of the mobile platform at the moment of making the scan.
        /// @param scanX x-axis postion of the platform when the scan was being made.
        /// @param scanY y-axis position of the platform when the scan was being made.
        /// @param lidarData Data collected from lidar. 
        /// @return Returns a jsonified version of the provided parameters
        String packLidarDataToJSON(float bearing, float scanX, float scanY, std::vector<float>& lidarData) {
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
    };
}

#endif
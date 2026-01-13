/// @file  HTTPCommunicator.h
/// @brief HTTPCommunicator can be used to pack lidar scans data into a json file 
///        and to send it via a POST request to specified server.

#ifndef HTTP_COMMUNICATOR_H
#define HTTP_COMMUNICATOR_H

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <vector>
#include <Arduino.h>

#include "TheSetuper.h"

/// @brief Contains the @ref HTTPCommunicator class which is responsible for sending lidar scans via http to server.
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
        HTTPCommunicator(String ssid = "", String password = "", String serverName = "");

        // --------------- Setup --------------- //

        /// @brief If default contrstructor has been used, the fields are not initialized.
        ///        This method can be used to set them.
        /// @param newSsid Sets the @ref ssid field.
        /// @param newPassword Sets the @ref password field.
        /// @param newServerName Sets the @ref serverName field.
        void resetVariables(String newSsid, String newPassword, String newServerName);
        
        /// @brief Establishes the connection with WiFi network.
        ///        Use it in void setup(). 
        ///        Prints the connection status to specified stream.
        /// @param stream Stream to which information about info about the connection is printed.
        ///               Stream such as Serial needs to be initilized, there is no checking of that.
        void setupWiFiConnection(Stream& stream);

        // --------------- Communication --------------- //

        /// @brief Sends a jsonified data collected by lidar to the server.
        /// @see #packLidarDataToJSON
        /// @param jsonLidar Lidar data jsonified with function packLidarDataToJSON.
        /// @param dest Destination on the server, the uh final site? serverName + dest -> final destination.
        ///             Remember to make it like this: dest = "/some-destination"
        /// @return If transmission was successfull, return true. If not, returns false.
        bool sendLidarData(String& jsonLidar, String dest);

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
        String packLidarDataToJSON(float bearing, float scanX, float scanY, std::vector<float>& lidarData);
    };
}

#endif
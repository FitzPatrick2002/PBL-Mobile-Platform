/// @file  Core0Manager.h
/// @brief Contains namespace which includes the class which manages operation of core 0 of the esp.
///        It includes the routine which sends odometry updates to the flask server, queue which transports data from core 1 to core 0 and something else.

#ifndef CORE_0_MANAGER_H
#define CORE_0_MANAGER_H

#include <Arduino.h>
#include "Defines.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>

#include "TheSetuper.h"

// All function used to be inline

/// @brief Stores viariables necessary.
namespace Cores{
    extern TaskHandle_t task0Handle; ///< Handle to the http handling which runs on core 0.
    extern QueueHandle_t manToHttpQ; ///< Handle to the queue with which Manager can send stuff to the http operator.

    /// @brief Handles slow http requests on core 0 of the esp.
    ///        Uses 2 queues to communicate with core 1.
    void handleHttpOnCore0(void *param);
    
    /// @brief Pins the #handleHttpOnCore0 to core 0 and contructs the queue with which core 1 can send data to core 0.
    /// @param messageSize   Size of the objects (in bytes) which will be put in the queue.
    /// @param queueCapacity Maximal number of objects which can be put in the queue.
    /// @param stackSize     Size of stack assigned to core 0, expressed in words (word == 2 bytes).
    void initCore0Task(int messageSize, int queueCapacity, int stackSize = 3500);
};

#endif
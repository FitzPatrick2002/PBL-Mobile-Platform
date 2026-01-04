/// @file Core0Manager.h
/// @brief Contains namespace which includes the class which manages operation of core 0 of the esp.
///        It includes the routine which sends odometry updates to the flask server, queue which transports data from core 1 to core 0 and something else.

#include "Defines.h"

struct PlatformPosition{
  float x, y = 0.0;
};

/// @brief Stores viariables necessary 
namespace Cores{
    TaskHandle_t task0Handle;        ///< Handle to the http handling which runs on core 0.
    QueueHandle_t manToHttpQ = NULL; ///< Handle to the queue with which Manager can send stuff to the http operator.

    /// @brief Handles slow http requests on core 0 of the esp.
    ///        Uses 2 queues to communicate with core 1.
    void handleHttpOnCore0(void *param){
        // 0. Await indefinitely if there is any data in the queue
        // 1. Read all data from the queue
        // 2. Pack it into a message 
        // 3. Create json
        // 4. Make http request

        // Store the platform position and handle to the queue.
        PlatformPosition position;
        QueueHandle_t queue = (QueueHandle_t)(param);

        // Message structure is as such:
        /*
        {
            "data-type": "odometry",
            "payload": [x0, y0, x1, y1, ...]
        }
        */

        // Main loop of the core 0 process.
        // Handles extraction of data from the manToHttp queue and sending the data to server
        while(true){
            // Await for notification that there is new content in the queue that can be read
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

            JsonDocument doc;
            doc["data-type"] = "odometry";
            JsonArray payload = doc["payload"].to<JsonArray>();

            // Read the queue as long as there is content in it
            bool hasData = false;
            while(xQueueReceive(queue, (&position), 0) == pdPASS){
            hasData = true;

            payload.add(position.x);
            payload.add(position.y);
            }

            // If position update has been received, send it via http
            if(hasData){
            
            // Stringify json and send to server
            doc.shrinkToFit();
            String jsonString;
            serializeJson(doc, jsonString);

            // Send the odometry data via POST
            WiFiClient client;
            HTTPClient http;
            if (http.begin(client, String(SERVER_NAME) + String(SERVER_POST_ENDPOINT))){
                http.addHeader("Content-type", "application/json");

                int httpResponseCode = http.POST(jsonString);
                http.end();

                Serial.println("Odometry data POST: Server response: ");
                Serial.println(httpResponseCode);
            }
            }

            // Wait 100 ms to let the system handle other tasks on this core (wifi, etc)
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    };
    
    /// @brief Pins the #handleHttpOnCore0 to core 0 and contructs the queue with which core 1 can send data to core 0.
    /// @param messageSize   Size of the objects (in bytes) which will be put in the queue.
    /// @param queueCapacity Maximal number of objects which can be put in the queue.
    /// @param stackSize     Size of stack assigned to core 0, expressed in words (word == 2 bytes).
    void initCore0Task(int messageSize, int queueCapacity, int stackSize = 3500){
        manToHttpQ = xQueueCreate(queueCapacity, messageSize);
        xTaskCreatePinnedToCore(handleHttpOnCore0, "core-0", stackSize, manToHttpQ, 0, &task0Handle, 0);
    }
};
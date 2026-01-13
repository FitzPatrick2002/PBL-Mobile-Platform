/// @file  Core0Manager.cpp
/// @brief Provides implementation for the routine which runs on core 0 of the esp.
/// @see Core0Manager.cpp

#include "Core0Manager.h"

namespace Cores{
   TaskHandle_t task0Handle;       
   QueueHandle_t manToHttpQ = NULL; 

   void handleHttpOnCore0(void *param){
      // 0. Await indefinitely if there is any data in the queue
      // 1. Read all data from the queue
      // 2. Pack it into a message 
      // 3. Create json
      // 4. Make http request

      // Store the platform position and handle to the queue.
      QueueMessage qData;
      QueueHandle_t queue = (QueueHandle_t)(param);

      // Message structure is as such:
      /*
      {
         "data-type": "odometry",
         "payload": [x0, y0, x1, y1, ...]
         "angle" : imu.yaw
         "collision" : collision status (true / false)
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
         while(xQueueReceive(queue, (&qData), 0) == pdPASS){
               hasData = true;

               // Pack positions from all queue messages into the json
               payload.add(qData.x);
               payload.add(qData.y);
         }

         // If position update has been received, send it via http
         if(hasData){

               // Include only the most recent collision status (HCSR data) and driving direction
               doc["collision"] = (bool)qData.collision;
               doc["angle"] = (float)qData.angle;

               Serial.print("Position loaded into the message: ");
               Serial.print(qData.x);
               Serial.print(", ");
               Serial.println(qData.y);

               Serial.print("Angle loaded to message: ");
               Serial.println(qData.angle);

               Serial.print("Collision status loaded to message: ");
               Serial.println(qData.collision);
               
               // Stringify json and send to server
               doc.shrinkToFit();
               String jsonString;
               serializeJson(doc, jsonString);

               // Send the odometry data via POST
               WiFiClient client;
               HTTPClient http;
               String destination = "http://" + getSetting("pc-server-name") + "/" + getSetting("pc-server-post-endp");
               if (http.begin(client, destination)){
                  Serial.print("Destination of telemetry post: ");
                  Serial.println(destination);
                  Serial.println("Http request (telemetry) done");
                  
                  http.addHeader("Content-type", "application/json");
                  int httpResponseCode = http.POST(jsonString);

                  Serial.println("Odometry data POST: Server response: ");
                  Serial.println(httpResponseCode);

                  http.end();

                  Serial.println("Odometry data POST: Server response: ");
                  Serial.println(httpResponseCode);
               }
         }

         // Wait 100 ms to let the system handle other tasks on this core (wifi, etc)
         vTaskDelay(pdMS_TO_TICKS(100));
      }
   }

   void initCore0Task(int messageSize, int queueCapacity, int stackSize){
      manToHttpQ = xQueueCreate(queueCapacity, messageSize);
      xTaskCreatePinnedToCore(handleHttpOnCore0, "core-0", stackSize, manToHttpQ, 0, &task0Handle, 0);
   }
};
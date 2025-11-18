#include <esp_now.h>
#include <WiFi.h>

// enum value containing the operation state of the platform
enum manager_state {
  STANDBY = 0,
  MOVING = 1, 
  SCANNING = 2,
  UPLOADING = 3 //data to the PC
}

// Must match the sender structure
typedef struct struct_message {
  int x, y;
  bool start, select, x_button, y_button, b_button, a_button;
  op_mode state;
} struct_message;

struct_message rx_message;

// callback function that will be executed when data is received
// modify this however you need
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpye rx_message, incomingData, len);

  Serial.print("X: ");
  Serial.printle rx_message.x);
  Serial.print("Y: ");
  Serial.printle rx_message.y);
  Serial.print("Start: ");
  Serial.printle rx_message.start);
  Serial.print("Select: ");
  Serial.printle rx_message.select);
  Serial.print("X button: ");
  Serial.printle rx_message.x_button);
  Serial.print("Y button: ");
  Serial.printle rx_message.y_button);
  Serial.print("A button: ");
  Serial.printle rx_message.a_button);
  Serial.print("B button: ");
  Serial.printle rx_message.b_button);
  Serial.print("State: ");
  Serial.printle rx_message.state);
}
 
void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // registering the callback function to OnDataRecv
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));

  // the setup went well
  Serial.println("The receiver board has been initiated.");
}
 
void loop() {
  // does not require code
}

   
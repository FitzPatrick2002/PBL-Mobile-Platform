#include <esp_now.h>
#include <WiFi.h>

enum manager_state{
  STANDBY = 0,
  MOVING = 1,
  SCANNING =2,
  UPLOADING = 3
};

struct message{
  int x,y;
  bool start, select, x_b, y_b, b_b, a_b;
  manager_state state;
};

message rx_message;

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len){
  memcpy(&rx_message, incomingData,len);
  Serial.println(rx_message.x);
  Serial.println(rx_message.y);
  Serial.println(rx_message.state);
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(100);
  Serial.println("dd");
  WiFi.mode(WIFI_STA);

  if(esp_now_init() != ESP_OK){
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
  Serial.println("The receiver has been initiated");
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(100);

}

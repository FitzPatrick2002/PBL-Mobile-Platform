/// @file esp-no-callbacks.cpp
/// @brief Defines the callbacks necessary to communicate with the remote controller.

#include "esp-now-callbacks.h"
#include "ManagerSpace.h"

// The main manager of the program (from ManagerSpace::) is defined in full-loop-test.ino
extern ManagerSpace::Manager manager; ///< Main manager instance. It should be created in the same file as void setup().

namespace EspNowCallback{

  uint8_t kontrolerAddress[6] = {0xDC, 0x06, 0x75, 0xF9, 0x61, 0xAC}; 

  esp_now_peer_info_t peerInfo; 

  platforma_message tx_message;

  // -------------------------------------------------------- //
  // -------------------- STRUCT MESSAGE -------------------- //
  // -------------------------------------------------------- //

  // -------------------- Constructors -------------------- //

  Message::Message(const volatile Message& mess){
    x = mess.x;
    y = mess.y;
    start = mess.start;
    select = mess.select;
    x_b = mess.x_b;
    y_b = mess.y_b;
    b_b = mess.b_b;
    a_b = mess.a_b;
    state = mess.state;
  }

  Message::Message(int xx, int yy, bool start_, bool select_, bool x_butt, bool y_butt, bool b_butt, bool a_butt, ManagerState newState){
    x = xx;
    y = yy;
    start = start_;
    select = select_;
    x_b = x_butt;
    y_b = y_butt;
    b_b = b_butt;
    a_b = a_butt;
    state = newState;
  }

  Message::Message(){
    x, y = 0;
    start, select, x_b, y_b, b_b, a_b = 0;
    state = ManagerState::STANDBY;
  }

  volatile Message& Message::operator=(const Message& mess) volatile {
    x = mess.x;
    y = mess.y;
    start = mess.start;
    select = mess.select;
    x_b = mess.x_b;
    y_b = mess.y_b;
    b_b = mess.b_b;
    a_b = mess.a_b;
    state = mess.state;

    return *this;
  }

  // -------------------- Utility Methods -------------------- //

  size_t Message::getBytesLength(){
    return 18; //Nah I will make it better later on//sizeof(x) + sizeof(y) + sizeof(start) + sizeof(x) + sizeof(x) + sizeof(x) + sizeof(x) + sizeof(x) + sizeof(x) + sizeof(x) + sizeof(x) + 
    // It's only for testing anyways
  }

  volatile void Message::printData(Stream &stream, char sep){
    stream.print(x);
    stream.print(sep);
    stream.print(y);
    stream.print(sep);

    stream.print(start);
    stream.print(sep);
    stream.print(select);
    stream.print(sep);
    stream.print(x_b);
    stream.print(sep);
    stream.print(y_b);
    stream.print(sep);
    stream.print(b_b);
    stream.print(sep);
    stream.print(a_b);
    stream.print(sep);

    stream.print((int)(state));
    stream.print(sep);
  }

  // ----------------------------------------------------------- //
  // -------------------- ESP NOW CALLBACKS -------------------- //
  // ----------------------------------------------------------- //

  void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
    // Copy the received bytes into the appropriate field of the manager
    portENTER_CRITICAL_ISR(&manager.messageSpinlock);
    memcpy ((uint8_t *)(manager.getControllerMessageLocation()), incomingData, len);
    manager.setMessageReceived(true);

    // In debug mode, print the received message to the console
    #ifdef DEBUG_RECEIVE_MSG
      Message rx_message = manager.getControllerLastMessage();

      Serial.print("X: ");
      Serial.println(rx_message.x);
      Serial.print("Y: ");
      Serial.println(rx_message.y);
      Serial.print("Start: ");
      Serial.println(rx_message.start);
      Serial.print("Select: ");
      Serial.println(rx_message.select);
      Serial.print("X button: ");
      Serial.println(rx_message.x_b);
      Serial.print("Y button: ");
      Serial.println(rx_message.y_b);
      Serial.print("A button: ");
      Serial.println(rx_message.a_b);
      Serial.print("B button: ");
      Serial.println(rx_message.b_b);
      Serial.print("State: ");
      Serial.println(rx_message.state);
    #endif

    portEXIT_CRITICAL_ISR(&manager.messageSpinlock);
  }

  void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t _status){
    manager.setState( ESP_NOW_SEND_SUCCESS ? ManagerState::STANDBY : ManagerState::STATUS_UPDATE);
    //Serial.println(_status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Failure");
  }

};
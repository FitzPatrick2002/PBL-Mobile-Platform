#ifndef ESP_NOW_CALLBACK
#define ESP_NOW_CALLBACK

#include <Arduino.h>
#include <esp_now.h>

#include "ManagerStates.h"

// Foreward declaration so that callbacks know it actually exists
namespace ManagerSpace{
  class Manager;
};

/// @brief Specifies the communication callbacks for the esp now protocol.
namespace EspNowCallback{

  //dc:06:75:f9:61:ac
  extern uint8_t kontrolerAddress[6]; //kontroler address for two way communication

  extern esp_now_peer_info_t peerInfo; //kontroler board info

  /// @brief Message struct is used to exchange data between rover and controller.
  struct Message {
    int x, y = 0; ///< Analog values of joystick potentiometers in range (0, 1023).
    bool start, select, x_b, y_b, b_b, a_b = 0; ///< Status of controllers buttons.
    ManagerState state = ManagerState::STANDBY; ///<  State into which the mobile platform should transitions after receiving the message.

    // -------------------- Constructors -------------------- //

    Message(const volatile Message& mess);

    Message(int xx, int yy, bool start_, bool select_, bool x_butt, bool y_butt, bool b_butt, bool a_butt, ManagerState newState);

    Message();

    volatile Message& operator=(const Message& mess) volatile;
    
    // -------------------- Utility Methods -------------------- //

    /// @brief Returns size of the structure in bytes.
    /// @return Size of the structure in bytes.
    size_t getBytesLength();

    /// @brief Prints the contents of the struct into a stream.
    /// @param stream Refernce to object that implements Stream class.
    /// @param sep Character separator, by default = ';'.
    volatile void printData(Stream &stream = Serial, char sep = ';');

  };

  struct platforma_message{
    bool is_scanning;
    String status_text;
  };

  extern platforma_message tx_message;

  // --------------- ESP NOW CALLBACKS --------------- //

  /// @brief Callback used when esp receives message via ESP NOW protocol.
  ///        Copies received message into the manager.
  ///        In debug mode prints the message via UART.
  /// @param mac Mac address of the receiver device.
  /// @param incomingData Incoming data bytes.
  /// @param len Number of bytes to read.
  void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len);

  /// @brief ESP NOW callback used when platform is sending a respons to the contrller via esp now.
  ///        If response was successfull state is set to STANDBY if not platform remains in STATUS_UPDATE
  ///        and attempts to re-send the response.
  void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t _status);
};

#endif 
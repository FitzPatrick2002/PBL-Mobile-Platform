/// @file esp-no-callbacks.h
/// @brief Defines the message struct used to communicate with the remote controller
///        and callbacks which define the way of communiating with the remote controller..
        
#ifndef ESP_NOW_CALLBACK
#define ESP_NOW_CALLBACK

#include <Arduino.h>
#include <esp_now.h>

#include "ManagerStates.h"
#include "TheSetuper.h"

// Foreward declaration so that callbacks know it actually exists
namespace ManagerSpace{
  class Manager;
};

/// @brief Specifies the communication callbacks for the esp now protocol.
namespace EspNowCallback{

  //dc:06:75:f9:61:ac
  extern uint8_t kontrolerAddress[6];           ///< MAC address of the remote controler board. Initially its hardcoded however it can be changed using the TheSetuper class.

  extern esp_now_peer_info_t peerInfo;          ///< Kontroler board info.

  /// @brief Message struct is used to exchange data between rover and controller.
  ///        It carries information from the controller to the mobile platform.
  ///        Information contained in it determines the state and operation of mobile platform.
  /// @note  Usually in code objects of type Message are volatile due to them being handled by the ESP NOW stack. 
  struct Message {
    int x, y = 0;                               ///< Analog values of joystick potentiometers in range (0, 1023).
    bool start, select, x_b, y_b, b_b, a_b = 0; ///< Status of controllers buttons.
    ManagerState state = ManagerState::STANDBY; ///<  State into which the mobile platform should transitions after receiving the message.

    // -------------------- Constructors -------------------- //

    /// @brief Copy constructor.
    ///        Copies all values from the mess to the created object.
    /// @param mess The message xchanged with ESP NOW.
    Message(const volatile Message& mess);

    /// @brief Constructs the Message object.
    /// @param xx Horizontal value of the joystick.
    /// @param yy Vertical value of the joystick.
    /// @param start_ 
    /// @param select_ 
    /// @param x_butt 
    /// @param y_butt 
    /// @param a_butt
    /// @param newState The state into the platform will transition after receiving the message. 
    Message(int xx, int yy, bool start_, bool select_, bool x_butt, bool y_butt, bool b_butt, bool a_butt, ManagerState newState);

    /// @brief Default constructor.
    ///        Sets all values to 0 / false.
    ///        State is set to @ref ManagerState::STANDBY
    Message();

    /// @brief Copy assingment.
    ///        Allows to copy volatile objects.
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

  /// @brief Response sent from mobile platform to the remote controller.
  struct platforma_message{ 
    bool is_scanning;   ///< Determines if the mobile platform is in ManagerState::SCANNING state.
    String status_text; ///< Status string specifies the current ManagerState type in which platform is currently operating.
  };

  extern platforma_message tx_message; ///< Holds the actual message sent by platform to the remote controller.

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
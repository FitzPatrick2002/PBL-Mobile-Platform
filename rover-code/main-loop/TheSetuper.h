/// @file  TheSetuper.h
/// @brief Contains the namespace which stores the class responsible for the setup of the mobile platform.
///        In order to use the Setup::TheSetuper class mobile platform needs to be connected with PC via UART.

#ifndef THE_SETUPER_H
#define THE_SETUPER_H

#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <map>
#include <string>

/// @brief Contains TheSetuper class which allows the user to set some 'global' values via a communication stream (like Serial).
namespace Setup{

  /// @brief Manages the setup of the mobile platform via UART.
  ///        Stores variables needed for initialization and operation of other elements of the mobile platform
  ///        and provides a way to setup them via a communication Stream (like Serial).
  ///        Available commands:
  ///         - set [varaible-name] [value] : Sets the value of a variable.
  ///         - show [variable-name]        : Shows the value of the variable.
  ///         - help, man                   : Shows how to issue commands.
  ///        Supported varaible names:
  ///         - **wifi-ssid**
  ///           Specifies the names of the wifi network to which the mobile platform will connect to.
  ///         - **wifi-password**
  ///           Password to wifi. Visible to everyone :3.
  ///         - **pc-server-name**
  ///           Server url (e.g. http://192.168.21.17:9000)
  ///         - **controller-mac**
  ///           - MAC address of the remote controller (e.g. DC:06:75:F9:61:AC). 
  ///           - Use only capital letters.
  ///           - Separate hexadecimal values by ':'.
  ///         - **platform-mac**
  ///           - Automatically read MAC address of the mobile platform. 
  ///           - Don't modify it, you will have te restart the platform to regenerate it.
  ///         - **pc-server-post-endp**
  ///           - Server endpoint which accepts the POST requests 
  ///           - All POST messages (lidar & odometry data) will be send there.
  ///         - **platform-server-name**
  ///           Name of the server which is running on the mobile platform.
  ///         - **platform-server-comm-endp**
  ///           Endpoint of the platform server, which accepts POST requests which specify steering commands.
  ///         - **platform-server-scan-endp**
  ///           Enpoint on the platform server, which accepts POST requests which request a new lidar scan and specify its details.
  /// 
  /// @note This class implements a singleton desing pattern (pointer singleton).
  /// @note If the Serial communication is cluncky take a look at interrupts being disabled with portENTER_CRITICAL()
  ///       Serial needs interrupts and this macro disbles them.
  /// Example Usage:
  /// 
  /// @code
  /// Setup::TheSetuper::getSetuper(&Serial); // Init the communication stream of TheSetuper as Serial (this can be done only once)
  /// Setup::TheSetuper::getSetuper()->theSetup(); // Setup the automatic variables (MAC & IP)
  /// // *Init Serial & WiFi here* //
  /// Setup::TheSetuper::getSetuper()->init(); // Run the setup loop again so that the server name of the mobile platform can be read
  /// Setup::TheSetuper::getSetuper()->theSetup(); // Setup the automatic variables (MAC & IP)
  /// @endcode
  class TheSetuper{
  private:

    // -------------- Serial Communication -------------- //

    Stream& commStream; ///< Communication stream used to interact with the setuper.

    // -------------- Variables Map -------------- //

    std::map<String, String> variables = {
      {"wifi-ssid", "Buahaha"},
      {"wifi-password", "Garfield1978"},
      {"pc-server-name", "10.214.168.134:9000"},

      {"controller-mac", "DC:06:75:F9:61:AC"}, 
      {"platform-mac", ""},

      {"pc-server-post-endp", "receive_post"},

      {"platform-server-name", "192.168.21.30"},
      {"platform-server-comm-endp", "command"},
      {"platform-server-scan-endp", "scan"}
    }; ///< Maps names of setup variables to their values.

    // -------------- Operation -------------- //

    bool commandServiced = true; ///< Specifies if the currently stored command has already been serviced.
    bool setterActive = true;    ///< When setter is active user can issue command via #commStream. Once its deactivated #commandLoop terminates.

    // -------------- Singleton Pointer -------------- //

    static TheSetuper* instance; ///< The only instance of TheSetuper is allocated on heap once.

    // -------------- Mutexes -------------- //

    static portMUX_TYPE variablesSpinlock;          ///< Spinlock protects the #variables map from concurrency.

    static portMUX_TYPE theSetuperInstanceSpinlock; ///< Protects the instance of TheSetuper from concurrent access.

  private:

    // -------------- Constructor & Copy Assignment -------------- //

    /// @brief initializes the reference to the communication stream #commStream.
    TheSetuper(Stream& s);

    /// @brief Disabled.
    TheSetuper& operator=(const TheSetuper& s) = delete;

    /// @brief the copy assignment is disabled.
    TheSetuper(const TheSetuper& s) = delete;

  public:

    // -------------- Destructor -------------- //

    /// @brief Empty.
    ~TheSetuper();

    // -------------- Singleton! -------------- //

    /// @brief Access the singleton instance.
    ///        Instance of TheSetuper @ref instance is protected with the spinlock @ref theSetuperInstanceSpinlock.
    /// @returns Pointer to the only instance of the TheSetuper.
    static TheSetuper* getSetuper(Stream *s = nullptr){
      if(instance == nullptr){

        portENTER_CRITICAL(&theSetuperInstanceSpinlock);
        if (instance == nullptr){
          if(s != nullptr){
            instance = new TheSetuper(*s);
          }
        }
        portEXIT_CRITICAL(&theSetuperInstanceSpinlock);
      }

      return instance;
    }

    // -------------- Initialization & Operation -------------- //

    /// @brief Initializes the varaibles which should be constant such as platoform-mac platform-server-name.
    ///        Other fields are set to their defaults.
    /// @note  WiFi needs to be initialized before using this. 
    void init();

    /// @brief Main loop of the TheSetuper, user can issue commands and set the variables. 
    ///        Available commands:
    ///        set [varaible-name] [value]
    ///        show [variable-name]
    /// @param initialMessage Initial message which will be printed before the setuper enters the command loop.
    ///                       By default its "".
    void theSetup(String initialMessage = "");

    // -------------- Utility -------------- //

    /// @brief Converts a stringified MAC address into the uint8_t array of length 6.
    /// @param dest Destination array where the MAC address value will be stored. It will take 6 bytes.
    /// @param src Source string, which should look like this: 'AA:BB:CC:DD:EE:FF'.
    ///            ':' can be replaced with '-', ',' or ' '. 
    void getMACfromString(uint8_t dest[], String src);

    // -------------- Accessing Variables -------------- //

    /// @brief Returns a value of specified variable.
    ///        If name is misspelled or such varaible does not exist, return "".
    /// @param name Name of a variable.
    /// @returns Value of a specified variable.
    String getVariable(String name);

  private:

    // -------------- Reading Commands -------------- //

    /// @brief Reads a single line from the #commStream if any is avaialable and returns it.
    /// @returns String read from the #commStream.
    String readCommand();

    /// @brief Recognises the issued command based on the first part of the string.
    ///        Passes the rest of the string to one of the runXXX functions.
    /// @param input User input read from the #commStream by #readCommand.
    void handleCommand(String input);

    // -------------- Commands -------------- //

    /// @brief Prints the manual how to use the program via #commStream.
    void runHelp();

    /// @brief Shows the value of passed variable.
    /// @param content Should be the name of one of the variables or 'all'.
    void runShow(String content);

    /// @brief Sets the variable based on specified content.
    /// @param content Chould contain the name of the variable and the value, separated by ' '.
    void runSet(String content);

  };
};

/// @brief Wrapper which works the same as getVariable() form TheSetuper class.
///        Its equvalent to the code below.
/// @code return Setup::TheSetuper::getSetuper()->getVariable(name);
/// @param name Name of the setup variable which we want to retreive.
///             To see the full list of varaibles, refer to documentation of TheSetuper.
/// @returns outcomes of the getVariable() function from TheSetuper class.
static inline String getSetting(String name){
  return Setup::TheSetuper::getSetuper()->getVariable(name);
}

#endif
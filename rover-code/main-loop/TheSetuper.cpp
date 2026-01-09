#include "TheSetuper.h"

namespace Setup{

  // -------------------------------------------------------- //
  // ------------------- TheSetuper Class ------------------- //
  // -------------------------------------------------------- //

  TheSetuper* TheSetuper::instance = nullptr;

  // -------------- Constructor & Destructor -------------- //

  /// @brief initializes the reference to the communication stream #commStream.
  TheSetuper::TheSetuper(Stream& s) : commStream(s) {}

  /// @brief Empty.
  TheSetuper::~TheSetuper() {}

  // -------------- Initialization & Operation -------------- //

  /// @brief Initializes the varaibles which should be constant such as platoform-mac platform-server-name.
  ///        Other fields are set to their defaults.
  /// @note  WiFi needs to be initialized before using this. 
  void TheSetuper::init(){
    // Setup the platform IP address
    this->variables["platform-server-name"] = WiFi.localIP().toString();

    // Setup the platform MAC
    uint8_t platformMACArray[6];
    esp_err_t result = esp_wifi_get_mac(WIFI_IF_STA, platformMACArray);

    // Load the MAC into a string
    char macString[18];
    if (result == ESP_OK){
      sprintf(macString, "%02X:%02X:%02X:%02X:%02X:%02X", platformMACArray[0], platformMACArray[1],
                                                          platformMACArray[2], platformMACArray[3],
                                                          platformMACArray[4], platformMACArray[5]);
      this->variables["platform-mac"] = String(macString);
    }
    else{
      commStream.println("Could not read the platform MAC.");
      commStream.println("If you want to use a remote controller, that will fail. Reset the platform.");
    }

    // Show the current setup
    this->runShow("all");
  }

  /// @brief Main loop of the TheSetuper, user can issue commands and set the variables. 
  ///        Available commands:
  ///        set [varaible-name] [value]
  ///        show [variable-name]
  void TheSetuper::theSetup(){
    String command;
    while (this->setterActive){
      command = this->readCommand();
      
      if(commandServiced == false){
        this->handleCommand(command);
      }
    }
  }

  // -------------- Utility -------------- //

  /// @brief Converts a stringified MAC address into the uint8_t array of length 6.
  /// @param dest Destination array where the MAC address value will be stored. It will take 6 bytes.
  /// @param src Source string, which should look like this: 'AA:BB:CC:DD:EE:FF'.
  ///            ':' can be replaced with '-', ',' or ' '. 
  void TheSetuper::getMACfromString(uint8_t dest[], String src){
    int numsRead = sscanf(src.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &dest[0], &dest[1], &dest[2],
                                                                        &dest[3], &dest[4], &dest[5]);

    // If there is didffernt delimieter in use, check other options
    if(numsRead < 6){
      numsRead = sscanf(src.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &dest[0], &dest[1], &dest[2],
                                                                      &dest[3], &dest[4], &dest[5]);
    }
    if(numsRead < 6){
      numsRead = sscanf(src.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &dest[0], &dest[1], &dest[2],
                                                                      &dest[3], &dest[4], &dest[5]);
    }
    if(numsRead < 6){
      numsRead = sscanf(src.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &dest[0], &dest[1], &dest[2],
                                                                      &dest[3], &dest[4], &dest[5]);
    }
  }

  // -------------- Reading Commands -------------- //

  /// @brief Reads a single line from the #commStream if any is avaialable and returns it.
  /// @returns String read from the #commStream.
  String TheSetuper::readCommand(){
    String command = "";

    // If any data is avaialable, read it
    if(commStream.available()){
      command = commStream.readStringUntil('\n');
      commandServiced = false;
    }

    return command;
  }

  /// @brief Recognises the issued command based on the first part of the string.
  ///        Passes the rest of the string to one of the runXXX functions.
  /// @param input User input read from the #commStream by #readCommand.
  void TheSetuper::handleCommand(String input){
    // Initially remove any trailing / leading spaces
    input.trim();

    // Recognize command which chould not take any params
    if(input == "help" || input == "man"){
      // Show the manual
      this->runHelp();
      this->commandServiced = true;
      return;
    }
    else if (input == "exit"){
      // Disable the main loop of the setuper and exit
      this->setterActive = false;
      this->commandServiced = true;
      return;
    }

    // Input should contain spaces for it to be valid at all
    int spaceIdx = input.indexOf(' ');
    if(spaceIdx == -1){
      commStream.println("Invalid command syntax. Use spcaes to spearate command, variable and value.");
      this->commandServiced = true;
      return;
    }

    // Split the string to command type and command content
    // Command is the part of the string before first ' ', content is the rest
    String commandName = input.substring(0, spaceIdx); 
    String content = input.substring(spaceIdx + 1); // Omit the ' ' character

    // Recognize command which takes params
    if(commandName == "show"){
      this->runShow(content);
    }
    else if (commandName == "set"){
      this->runSet(content);
    }

    this->commandServiced = true;
  } 

  // -------------- Commands -------------- //

  /// @brief Prints the manual how to use the program via #commStream.
  void TheSetuper::runHelp(){
    commStream.println("Setup Manual");
    commStream.println("You can use the following commands. Please omit the [] in your input.");
    commStream.println("To show values of all variables type in \'show all\'");
    
    commStream.println();

    commStream.println("set [variable-name] [value]");
    commStream.println("Sets a variable to the specified value.");
    
    commStream.println();

    commStream.println("show [variable-name]");
    commStream.println("Shows value of a variable.");

    commStream.println();

    commStream.println("help");
    commStream.println("man");
    commStream.println("Both help and man print the this manual.");

    commStream.println();
    
    commStream.println("exit");
    commStream.println("Exits the setup phase");

    commStream.println();

    commStream.println("Notes:");
    commStream.println("- Write MAC with \':\' as delimieter.");
    commStream.println("- Write MAC using capital letters.");

  }

  /// @brief Shows the value of passed variable.
  /// @param content Should be the name of one of the variables or 'all'.
  void TheSetuper::runShow(String content){
    // Remove trailing / leading white spaces from the content 
    content.trim();

    if(content.length() == 0){
      commStream.println("\'show\' takes at least one variable name or \'all\' as input.");
      return;
    }

    // Print all variables
    if(content == "all"){
      commStream.println("All variables:");

      for(const auto& elem : variables){
        commStream.print(elem.first);
        commStream.print(": ");
        commStream.println(elem.second);
      }

      return;
    }

    // Print the specified variable
    if(variables.find(content) != variables.end()){
      commStream.print(content);
      commStream.print(": ");
      commStream.println(variables[content]);
    }
    else{
      commStream.print("Variable: ");
      commStream.print(content);
      commStream.println(" not found");
    }

  }

  /// @brief Sets the variable based on specified content.
  /// @param content Chould contain the name of the variable and the value, separated by ' '.
  void TheSetuper::runSet(String content){
    // Split the content into variable and value assigned to the variable
    content.trim();

    // Check if there is any content and if it contains at least two elements
    int spaceIdx = content.indexOf(' ');

    if(spaceIdx == -1){
      commStream.println("\'set\' command takes 2 inputs separated by space.");
      return;
    }

    String setVariable = content.substring(0, spaceIdx);
    String setValue = content.substring(spaceIdx + 1);

    setVariable.trim();
    setValue.trim();

    if(setValue.length() == 0){
      commStream.println("\'set\' command takes 2 inputs separated by space.");
      return;
    }

    // If variable exists, 
    if(variables.find(setVariable) != variables.end()){
      variables[setVariable] = setValue;
      commStream.println("Variable value set!");
    }
    else{
      commStream.print("Variable: ");
      commStream.print(setVariable);
      commStream.println(" not found");
    }
  }

  // -------------- Accessing Variables -------------- //

  /// @brief Returns a value of specified variable.
  ///        If name is misspelled or such varaible does not exist, return "".
  /// @param name Name of a variable.
  /// @returns Value of a specified variable.
  String TheSetuper::getVariable(String name){
    return (variables.count(name) ? variables[name] : "");
  }
}


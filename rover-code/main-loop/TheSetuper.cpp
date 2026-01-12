/// @file  TheSetuper.cpp
/// @brief Provides the definintion of TheSetuper class from TheSetuper.h.

#include "TheSetuper.h"

namespace Setup{

  // -------------------------------------------------------- //
  // ------------------- TheSetuper Class ------------------- //
  // -------------------------------------------------------- //

  TheSetuper* TheSetuper::instance = nullptr;

  // -------------- Constructor & Destructor -------------- //

  TheSetuper::TheSetuper(Stream& s) : commStream(s) {}

  TheSetuper::~TheSetuper() {}

  // -------------- Initialization & Operation -------------- //

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

  void TheSetuper::theSetup(String initialMessage){
    commStream.println(initialMessage);

    String command;
    while (this->setterActive){
      command = this->readCommand();
      
      if(commandServiced == false){
        this->handleCommand(command);
      }
    }
  }

  // -------------- Utility -------------- //

  void TheSetuper::getMACfromString(uint8_t dest[], String src){
    int numsRead = sscanf(src.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &dest[0], &dest[1], &dest[2],
                                                                        &dest[3], &dest[4], &dest[5]);

    // If there is different delimeter in use, check other options
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

  String TheSetuper::readCommand(){
    String command = "";

    // If any data is avaialable, read it
    if(commStream.available()){
      command = commStream.readStringUntil('\n');
      commandServiced = false;
    }

    return command;
  }

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

  String TheSetuper::getVariable(String name){
    return (variables.count(name) ? variables[name] : "");
  }
}


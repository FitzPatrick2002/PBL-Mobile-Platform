/// @file sender.ino
/// @brief Contains the entire mobile controller code for steering the platform

#include <Wire.h>
#include "Adafruit_seesaw.h"

#include <esp_now.h>
#include <WiFi.h>

#include "bitmaps.h"

// Custom e-ink display setup
// Install GxEPD2 library
#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold9pt7b.h>

// E-Ink pins
#define EPD_CS    7
#define EPD_DC    1
#define EPD_RST   2
#define EPD_BUSY  3

// Message sender communicator
#define blue_led 21

// Controller buttons
#define BUTTON_X         6
#define BUTTON_Y         2
#define BUTTON_A         5
#define BUTTON_B         1
#define BUTTON_SELECT    0
#define BUTTON_START    16

// Own constructor for the E-Ink display
GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT>
display(GxEPD2_154_D67(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

// -------------- Global Objects -------------- //

/// @brief Seesaw object for the Mini I2C Gamepad board
Adafruit_seesaw ss;

/// @brief Button mask for the Mini I2C Gamepad
uint32_t button_mask = (1UL << BUTTON_X) | (1UL << BUTTON_Y) | (1UL << BUTTON_START) |
                       (1UL << BUTTON_A) | (1UL << BUTTON_B) | (1UL << BUTTON_SELECT);

// TO DO CHANGE
/// @brief MAC address of the platform
uint8_t receiverAddress[] = {0x48, 0xCA, 0x43, 0x2f, 0x6C, 0xA0};

/// @brief ESP-NOW peer info of the connected platform
esp_now_peer_info_t peerInfo;

/// @brief Message states to be chosen with controller buttons, sent to the platform
enum manager_state{
  STANDBY = 0,
  MOVING = 1,
  SCANNING = 2,
  UPLOADING = 3,
  STATUS_UPDATE = 4,
  IMU_CALIBRATION = 5,
  ROVER_INIT = 6
};

/// @brief The structure of the transmitter message to the platform
struct message{
  int x,y;
  bool start, select, x_b, y_b, b_b, a_b;
  manager_state state;
};

/// @brief Transmitter message, message to be sent to the platform
message tx_message;

/// @brief The structure of the receiver message, sent from the platform
struct platforma_message{
  bool is_scanning;
  String status_text;
};

/// @brief Receiver message, message sent from the platform to the controller
platforma_message rx_message;

/// @brief Last read values from the Mini I2C Gamepad
int last_x = 0, last_y =0;
manager_state last_man_state = STANDBY;

// --------------E-Ink Display methods -------------- //

/// @brief Draws the bitmap from bitmaps.h, specified by the parameter choice, on the E-Ink display
///         Used for the initial logo screen
void drawBitmaps(int choice)
{
  display.setRotation(1);
  display.setFullWindow();
  if(choice > 1 || choice < 0)
    return;

  display.firstPage();
  do{
    display.fillScreen(GxEPD_WHITE);
    display.drawInvertedBitmap(0, 0, bitmaps[choice], 200, 200, GxEPD_BLACK);
  }while(display.nextPage());
}

/// @brief Clears the E-Ink display
void cleanDisplay()
{
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
  } while (display.nextPage());
}

/// @brief Displays the frame bitmap and the selected state text on the E-Ink display
void displayMessage(const char message[])
{
  display.setRotation(1);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);

  String text = message;
  int index = text.indexOf('\n');
  if(index != -1) // The message contains two lines of text
  {
    // Seperate the two lines
    String second_line = text.substring(index+1);
    String first_line = text.substring(0, index-1);

    // Calculate the text positions
    int16_t tbx, tby; uint16_t tbw, tbh;
    display.getTextBounds(first_line.c_str(), 0, 0, &tbx, &tby, &tbw, &tbh);
    // Center the bounding box by transposition of the origin:
    uint16_t x = ((display.width() - tbw) / 2) - tbx;
    uint16_t y = ((display.height() - tbh) / 2) - tby-20;

    display.getTextBounds(second_line.c_str(), 0, 0, &tbx, &tby, &tbw, &tbh);
    uint16_t x_ = ((display.width() - tbw) / 2) - tbx;
    uint16_t y_ = ((display.height() - tbh) / 2) - tby+20;

    display.setFullWindow();
    display.firstPage();
    do
    {
      // Draw the frame bitmap
      display.drawInvertedBitmap(0, 0, bitmaps[0], 200, 200, GxEPD_BLACK);

      display.setCursor(x, y);
      display.print(first_line);
      display.setCursor(x_, y_);
      display.print(second_line);
    }
    while (display.nextPage());
  }else
  {
    int16_t tbx, tby; uint16_t tbw, tbh;
    display.getTextBounds(message, 0, 0, &tbx, &tby, &tbw, &tbh);
    // center the bounding box by transposition of the origin:
    uint16_t x = ((display.width() - tbw) / 2) - tbx;
    uint16_t y = ((display.height() - tbh) / 2) - tby;
    display.setFullWindow();
    display.firstPage();
    do
    {
      display.setCursor(x, y);
      display.print(message);
    }
    while (display.nextPage());
  }
  
}

// --------------ESP-NOW message creation -------------- //

/// @brief Converts the manager_state state to a string value for printing
const char* stateToString(manager_state state)
{
  switch (state)
  {
    case STANDBY:       return "STANDBY";
    case MOVING:        return "MOVING";
    case SCANNING:      return "SCANNING";
    case UPLOADING:     return "UPLOADING";
    case STATUS_UPDATE: return "STATUS UPDATE";
    case IMU_CALIBRATION: return "IMU CALIBRATION";
    case ROVER_INIT:          return "PLATFORMA INIT";
    default:            return "UNKNOWN";
  }
}

/// @brief Constructs new message based on the Mini I2C Gamepad input values and the previous message
void constructMessage(message& new_message)
{
  int x = 1023 - ss.analogRead(14);
  int y = 1023 - ss.analogRead(15); //fixed the pin reading
  // Reset the message values
  new_message.a_b = false;
  new_message.b_b = false;
  new_message.x_b = false;
  new_message.y_b = false;
  new_message.start = false;
  new_message.select = false;
  new_message.state = last_man_state;

  if(new_message.state!= SCANNING || new_message.state!= UPLOADING)
  {
    if((abs(x-last_x)>3) || (abs(y-last_y)>3)){
      last_x = x; last_y = y;
      new_message.x = x; new_message.y = y;
      new_message.state = MOVING;

      send_message();
      Serial.println("Sending a message");
    }
  }

  uint32_t buttons = ss.digitalReadBulk(button_mask);

  if(!(buttons & (1UL << BUTTON_A))){
    new_message.a_b = true;
    if(new_message.state != UPLOADING || new_message.state != ROVER_INIT || new_message.state != IMU_CALIBRATION)
    {
      new_message.state = SCANNING;
    }
  }
  if(!(buttons & (1UL << BUTTON_B))){
    new_message.b_b = true;
    if(new_message.state != SCANNING || new_message.state != ROVER_INIT || new_message.state != IMU_CALIBRATION)
    {
      new_message.state = UPLOADING;
    }
  }
  if(!(buttons & (1UL << BUTTON_X)))
  {
    new_message.y_b = true;
    new_message.state = STANDBY; // CANCEL FOR ALL, NO MATTER WHAT WAS PREVIOUSLY
  }

  if(!(buttons & (1UL << BUTTON_Y)))
  {
    if(new_message.state!= SCANNING || new_message.state!= UPLOADING || last_man_state != STATUS_UPDATE || new_message.state != ROVER_INIT || new_message.state != IMU_CALIBRATION)
    {
      new_message.state = STATUS_UPDATE;
    }
    new_message.x_b = true;
  }
  if(!(buttons & (1UL << BUTTON_SELECT))) //for IMU_CALIBRATION
  { 
    if(new_message.state!= SCANNING || new_message.state!= UPLOADING || new_message.state != ROVER_INIT)
    {
      new_message.state = IMU_CALIBRATION;
    }
    new_message.select = true;
  }
  if(!(buttons & (1UL << BUTTON_START)))
  {
    new_message.start = true;
    send_message();
    Serial.println("Sending a message");
  }
  if(!(buttons & (1UL << BUTTON_X)) && !(buttons & (1UL << BUTTON_Y))) //combo for init
  {
    new_message.state = ROVER_INIT;
  }

  last_man_state = new_message.state;

  // Printing data
  Serial.println("*|==========================================================|*");
  Serial.print("Values: X: "); Serial.print(new_message.x);Serial.print(" | Y: "); Serial.print(new_message.y); 
  Serial.print(" |\nButtons: A: "); Serial.print(new_message.a_b); Serial.print(" | B: "); Serial.print(new_message.b_b);
  Serial.print(" | X: "); Serial.print(new_message.x_b); Serial.print(" | Y: "); Serial.print(new_message.y_b);
  Serial.print(" | Start: "); Serial.print(new_message.start); Serial.print(" | Select: "); Serial.println(new_message.select);
  Serial.print("State: "); Serial.print(stateToString(new_message.state));
  
}

/// @brief Sends the message through ESP-NOW protocol to the platforma
void send_message()
{
  esp_err_t result = esp_now_send(receiverAddress, (uint8_t *) &tx_message, sizeof(tx_message));

  if(result == ESP_OK){
    Serial.print("Sent with success ");
  }else{
    Serial.print("Error sending the data ");
  }
  if(tx_message.state != MOVING)
  {
    String text = "Sent message:\n";
    text += stateToString(tx_message.state);
    displayMessage(text.c_str());
  }
}

// --------------ESP-NOW callback methods -------------- //

/// @brief Callback method called when the message is sent to the platforma
///         Lights up the blue diode
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status){
  digitalWrite(blue_led, HIGH);
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Failure");
  delay(50);
  digitalWrite(blue_led, LOW);
}

/// @brief Callback method called when the status update is received from the platforma
///         Prints out the state on the E-Ink display
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len){
  memcpy(&rx_message, incomingData,len);
  Serial.println(rx_message.status_text);
  String text = "Received state:\n";
  text += rx_message.status_text;
  displayMessage(text.c_str());
}

// --------------Setup & Main Loop -------------- //

void setup() {

  Serial.begin(115200);

  Wire.begin(8,9); //SDA SCL

  pinMode(blue_led, OUTPUT);
  delay(50);

  if(!ss.begin(0x50)){
    Serial.println("Error seesaw not found");
    while(1) delay(1);
  }

  uint32_t version = ((ss.getVersion() >> 16) & 0xFFFF);
  if (version != 5743) {
    Serial.print("Wrong firmware loaded? ");
    Serial.println(version);
    while(1) delay(10);
  }
  
  Serial.println("Found Product 5743");
  
  // Set up the mask for the Mini I2C Gamepad
  ss.pinModeBulk(button_mask, INPUT_PULLUP);
  ss.setGPIOInterrupts(button_mask, 1);

  WiFi.mode(WIFI_STA);
  if(esp_now_init()!= ESP_OK){
    Serial.println("error initializing ESP-NOW");
    return;
  }

  esp_now_register_send_cb(esp_now_send_cb_t(OnDataSent));

  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));

  memcpy(peerInfo.peer_addr, receiverAddress,6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if(esp_now_add_peer(&peerInfo)!=ESP_OK){
    Serial.println("failed to add peer");
    return;
  }

  // E-Ink display initialization
  display.init(115200, true, 2, false); //2ms reset pulse

  // Initial logo screen
  drawBitmaps(1);
  display.hibernate();
}

void loop() {
    constructMessage(tx_message);
    delay(200);
}

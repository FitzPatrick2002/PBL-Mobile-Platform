#include <Wire.h>
#include "Adafruit_seesaw.h"
Adafruit_seesaw ss;

#include <esp_now.h>
#include <WiFi.h>

//custom e-ink display setup
#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold9pt7b.h>
//#include "GxEPD2_display_selection_new_style.h"
#define EPD_CS    7
#define EPD_DC    1
#define EPD_RST   2
#define EPD_BUSY  3

//own constructor
GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT>
display(GxEPD2_154_D67(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

#include "bitmaps.h"

#define blue_led 21

#define BUTTON_X         6
#define BUTTON_Y         2
#define BUTTON_A         5
#define BUTTON_B         1
#define BUTTON_SELECT    0
#define BUTTON_START    16
uint32_t button_mask = (1UL << BUTTON_X) | (1UL << BUTTON_Y) | (1UL << BUTTON_START) |
                       (1UL << BUTTON_A) | (1UL << BUTTON_B) | (1UL << BUTTON_SELECT);

uint8_t receiverAddress[] = {0xE4, 0xB0, 0x63, 0xAF, 0x36, 0xBC};

esp_now_peer_info_t peerInfo;

enum manager_state{
  STANDBY = 0,
  MOVING = 1,
  SCANNING = 2,
  UPLOADING = 3,
  STATUS_UPDATE = 4
};

struct message{
  int x,y;
  bool start, select, x_b, y_b, b_b, a_b;
  manager_state state;
};

message tx_message;

struct platforma_message{
  bool is_scanning;
  String status_text;
};

platforma_message rx_message;

int last_x = 0, last_y =0;
manager_state last_man_state = STANDBY;

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

void cleanDisplay()
{
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
  } while (display.nextPage());
}

const char HelloWorld[] = "Hello World!";

void displayMessage(const char message[])
{
  //cleanDisplay();
  display.setRotation(1);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);
  String text = message;
  int index = text.indexOf('\n');
  if(index != -1)
  {
    String second_line = text.substring(index+1);
    String first_line = text.substring(0, index-1);
    int16_t tbx, tby; uint16_t tbw, tbh;
    display.getTextBounds(first_line.c_str(), 0, 0, &tbx, &tby, &tbw, &tbh);
    // center the bounding box by transposition of the origin:
    uint16_t x = ((display.width() - tbw) / 2) - tbx;
    uint16_t y = ((display.height() - tbh) / 2) - tby-20;

    display.getTextBounds(second_line.c_str(), 0, 0, &tbx, &tby, &tbw, &tbh);
    uint16_t x_ = ((display.width() - tbw) / 2) - tbx;
    uint16_t y_ = ((display.height() - tbh) / 2) - tby+20;
    display.setFullWindow();
    display.firstPage();
    do
    {
      display.drawInvertedBitmap(0, 0, bitmaps[0], 200, 200, GxEPD_BLACK);
      //display.fillScreen(GxEPD_WHITE);
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
      //display.fillScreen(GxEPD_WHITE);
      display.setCursor(x, y);
      display.print(message);
    }
    while (display.nextPage());
  }
  
}



void constructMessage(message& new_message)
{
  int x = 1023 - ss.analogRead(14);
  int y = 1023 - ss.analogRead(15); //fixed the pin reading
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
    if(new_message.state != UPLOADING)
    {
      new_message.state = SCANNING;
    }
  }
  if(!(buttons & (1UL << BUTTON_B))){
    new_message.b_b = true;
    if(new_message.state != SCANNING)
    {
      new_message.state = UPLOADING;
    }
  }
  if(!(buttons & (1UL << BUTTON_Y)))
  {
    new_message.y_b = true;
    if(new_message.state == SCANNING || new_message.state == UPLOADING || new_message.state == STATUS_UPDATE)
    {
      new_message.state = STANDBY;
    }
  }

  if(!(buttons & (1UL << BUTTON_X)))
  {
    if(new_message.state == MOVING)
    {
      new_message.state = STANDBY;
    }
    new_message.x_b = true;
  }
  if(!(buttons & (1UL << BUTTON_SELECT)))
  { 
    if(new_message.state!= SCANNING || new_message.state!= UPLOADING || last_man_state != STATUS_UPDATE)
    {
      new_message.state = STATUS_UPDATE;
    }
    new_message.select = true;
  }
  if(!(buttons & (1UL << BUTTON_START)))
  {
    new_message.start = true;
    send_message();
    Serial.println("Sending a message");
  }

  last_man_state = new_message.state;

  //printing data
  Serial.println("*|==========================================================|*");
  Serial.print("Values: X: "); Serial.print(new_message.x);Serial.print(" | Y: "); Serial.print(new_message.y); 
  Serial.print(" |\nButtons: A: "); Serial.print(new_message.a_b); Serial.print(" | B: "); Serial.print(new_message.b_b);
  Serial.print(" | X: "); Serial.print(new_message.x_b); Serial.print(" | Y: "); Serial.print(new_message.y_b);
  Serial.print(" | Start: "); Serial.print(new_message.start); Serial.print(" | Select: "); Serial.println(new_message.select);
  Serial.print("State: ");  
  String state;
  switch(new_message.state)
  {
    case STANDBY:
      state = "STANDBY";
      break;
    case MOVING:
      state = "MOVING";
      break;
    case SCANNING:
      state = "SCANNING";
      break;
    case UPLOADING:
      state = "UPLOADING";
      break;
    case STATUS_UPDATE:
      state = "requesting update";
      break;
  }
  
}

const char* stateToString(manager_state state)
{
  switch (state)
  {
    case STANDBY:       return "STANDBY";
    case MOVING:        return "MOVING";
    case SCANNING:      return "SCANNING";
    case UPLOADING:     return "UPLOADING";
    case STATUS_UPDATE: return "STATUS UPDATE";
    default:            return "UNKNOWN";
  }
}

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

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status){
  digitalWrite(blue_led, HIGH);
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Failure");
  delay(50);
  digitalWrite(blue_led, LOW);
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len){
  memcpy(&rx_message, incomingData,len);
  //last_man_state = STANDBY;
  Serial.println(rx_message.status_text);
  String text = "Received state:\n";
  text += rx_message.status_text;
  displayMessage(text.c_str());
}

void setup() {
  // put your setup code here, to run once:
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
  
  //set up the mask for the i2c minicontroller
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

  //e-ink display
  display.init(115200, true, 2, false); // USE THIS for Waveshare boards with "clever" reset circuit, 2ms reset pulse
  //displayMessage("Starting program");
  drawBitmaps(1);
  display.hibernate();
}

void loop() {
    constructMessage(tx_message);
    delay(200);
}

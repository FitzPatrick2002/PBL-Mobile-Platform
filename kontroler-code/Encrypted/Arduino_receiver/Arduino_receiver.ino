#include <esp_now.h>
#include <WiFi.h>
#include <LittleFS.h>
#include "FS.h"

#define FORMAT_LITTLEFS_IF_FAILED true

// enum value containing the operation state of the platform
enum manager_state {
  STANDBY = 0,
  MOVING = 1, 
  SCANNING = 2,
  UPLOADING = 3 //data to the PC
};

// Must match the sender structure
typedef struct struct_message {
  int x, y;
  bool start, select, x_button, y_button, b_button, a_button;
  manager_state state;
};

struct_message rx_message;

//for encryption purposes
uint8_t masterMacAddress[6];

// encryption codes
// It can be made of numbers and letters and the keys are 16 bytes
static char PMK_KEY_STR[17]; //sender's
static char LMK_KEY_STR[17]; //local relationship between rx and tx

void rtrim(char* s) {
  int len = strlen(s);
  while (len > 0 && (s[len-1] == '\n' || s[len-1] == '\r' || s[len-1] == ' ')) {
    s[len-1] = '\0';
    len--;
  }
}

void readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);

  
  char line[64];
  char pmkKey[17] = {0};
  char lmkKey[17] = {0};
  char mac[32] = {0};
  File file = fs.open(path);
  if(!file || file.isDirectory()){
      Serial.println("- failed to open file for reading");
      return;
  }

  while(file.available()){
    

    int len = file.readBytesUntil('\n', line, sizeof(line) - 1);
    line[len] = '\0';
    rtrim(line); // <- usuwa końcowe \n i \r

    if (strncmp(line, "PMK:", 4) == 0) {
      strncpy(pmkKey, line + 4, 16);
      rtrim(pmkKey); // usuwa końcówki
    } else if (strncmp(line, "LMK:", 4) == 0) {
      strncpy(lmkKey, line + 4, 16);
      rtrim(lmkKey);
    } else if (strncmp(line, "RX_MAC:", 7) == 0) {
      strncpy(mac, line + 7, sizeof(mac) - 1);
      rtrim(mac);
    }
  }
  
  
  file.close();
  Serial.println("Keys loaded:");
  Serial.print("PMK: ");
  Serial.println(pmkKey);
  Serial.print("LMK: ");
  Serial.println(lmkKey);
  Serial.print("Mac: ");
  Serial.println(mac);

  //setting the mac address
  int values[6];
  if (sscanf(mac, "%x:%x:%x:%x:%x:%x",
             &values[0], &values[1], &values[2],
             &values[3], &values[4], &values[5]) == 6) {
    for (int i = 0; i < 6; ++i) masterMacAddress[i] = (uint8_t) values[i]; //assigning the MAC address of the rx
  } else {
    Serial.println("Invalid MAC format!");
    return;
  }

  strncpy(PMK_KEY_STR, pmkKey, sizeof(PMK_KEY_STR));
  strncpy(LMK_KEY_STR, lmkKey, sizeof(LMK_KEY_STR));
  Serial.print("PMK_KEY_STR bytes: ");

  for(int i=0;i<16;i++){
      Serial.print((uint8_t)PMK_KEY_STR[i], HEX);
      Serial.print(" ");
  }
  Serial.println();

  Serial.print("LMK_KEY_STR bytes: ");
  for(int i=0;i<16;i++){
      Serial.print((uint8_t)LMK_KEY_STR[i], HEX);
      Serial.print(" ");
  }
  Serial.println();

  
  // PMK_KEY_STR = pmkKey;
  // LMK_KEY_STR = lmkKey;
}

// callback function that will be executed when data is received
// modify this however you need
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&rx_message, incomingData, len);

  Serial.print("X: ");
  Serial.println( rx_message.x);
  Serial.print("Y: ");
  Serial.println( rx_message.y);
  Serial.print("Start: ");
  Serial.println( rx_message.start);
  Serial.print("Select: ");
  Serial.println( rx_message.select);
  Serial.print("X button: ");
  Serial.println( rx_message.x_button);
  Serial.print("Y button: ");
  Serial.println( rx_message.y_button);
  Serial.print("A button: ");
  Serial.println( rx_message.a_button);
  Serial.print("B button: ");
  Serial.println( rx_message.b_button);
  Serial.print("State: ");
  Serial.println( rx_message.state);
}
 
void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS mount failed!");
  } else {
    Serial.println("LittleFS mounted OK");
    if (LittleFS.exists("/keys.txt")) {
        Serial.println("keys.txt exists");
    } else {
        Serial.println("keys.txt NOT found");
    }
  }

  //read the keys and MAC
  readFile(LittleFS,"/keys.txt");

  // set the PMK key for encryption
  esp_now_set_pmk((uint8_t *)PMK_KEY_STR);

  // Register the master as peer
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, masterMacAddress, 6);
  peerInfo.channel = 1;
  // Setting the master device LMK key aka the local key of the relationship
  for (uint8_t i = 0; i < 16; i++) {
    peerInfo.lmk[i] = LMK_KEY_STR[i];
  }
  peerInfo.encrypt = false; //change to true later, it doesnt work for now :CCCC
  
  esp_now_del_peer(masterMacAddress);

  // Add master as peer       
  if (esp_now_add_peer(&peerInfo) == ESP_OK) {
    Serial.println("Peer added successfully");
  } else {
    Serial.println("Failed to add peer");
  }


  // registering the callback function to OnDataRecv
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));

  // the setup went well
  Serial.println("The receiver board has been initiated.");
}
 
void loop() {
  // does not require code
}

   
#include <WiFi.h>
#include <esp_wifi.h>

//This code will help you obtain the MAC address of the board you want to be the receiver in ESP-NOW communication
//Simply plug in your board, start the code and it should print out its MAC address

//if we need to set the MAC address
//esp_err_t esp_wifi_set_mac(wifi_interface_t ifx, const uint8_t mac[6])

//this function can ONLY be called before the interface is enabled.
//The bit 0 of the first byte of MAC address can not be 1.
void setMacAddress(uint8_t mac[6])
{
  esp_err_t ret = esp_wifi_set_mac(WIFI_IF_STA, mac); //station interface

  if (ret == ESP_OK) { //succeeded
    Serial.printf("%02x:%02x:%02x:%02x:%02x:%02x\n",
                  mac[0], mac[1], mac[2],
                  mac[3], mac[4], mac[5]); //printing the MAC address to be copied into the sender's code
  } else if (ret == ESP_ERR_WIFI_NOT_INIT){
    Serial.println("WiFi is not initialized by esp_wifi_init.");
  } else if (ret == ESP_ERR_INVALID_ARG){
    Serial.println("Invalid argument.");
  }else if (ret == ESP_ERR_WIFI_IF){
    Serial.println("Invalid interface. Is not STATION.");
  }else if(ret == ESP_ERR_WIFI_MAC){
    Serial.println("Invalid mac address. Remember that the bit 0 of the first byte of MAC address can not be 1.");
  }else if(ret == ESP_ERR_WIFI_MODE){
    Serial.println("WiFi mode is wrong.");
  }

}

void readMacAddress(){

  uint8_t baseMac[6]; //will contain the MAC address of the board
  esp_err_t ret = esp_wifi_get_mac(WIFI_IF_STA, baseMac); //providing an interface, it will fill the baseMac address and return the status of the action

  if (ret == ESP_OK) { //succeeded
    Serial.printf("%02x:%02x:%02x:%02x:%02x:%02x\n",
                  baseMac[0], baseMac[1], baseMac[2],
                  baseMac[3], baseMac[4], baseMac[5]); //printing the MAC address to be copied into the sender's code
  } else if (ret == ESP_ERR_WIFI_NOT_INIT){
    Serial.println("WiFi is not initialized by esp_wifi_init.");
  } else if (ret == ESP_ERR_INVALID_ARG){
    Serial.println("Invalid argument.");
  }else if (ret == ESP_ERR_WIFI_IF){
    Serial.println("Invalid interface. Is not STATION.");
  }

}

void setup(){
  Serial.begin(115200);
  while (!Serial) { delay(10); } //for arduino nano esp32 because there's a delay

  Serial.println("Reading the MAC address:");

  WiFi.mode(WIFI_STA); //Station mode (client)

  //uncomment to set a new mac address

  // uint8_t newMac[6] = {0xB4, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  // setMacAddress(newMac);

  WiFi.STA.begin(); //start the wi-fi

  Serial.print("ESP32 Board MAC Address: ");
  readMacAddress();
}
 
void loop(){

}
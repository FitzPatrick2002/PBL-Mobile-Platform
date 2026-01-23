/// @file Distance_Alarm.ino
/// @brief Controlls two ultrasonic sensors and an LED, setting up the trigger pins via .S code.
#define echo_uno 1
#define trigger_uno 0
#define echo_dos 2 
#define trigger_dos 3
#define led_uno 6 // changed from 5

// -------------- Global Objects -------------- //

/// @brief Initiating variables to calculate and read for ultrasonic sensors.
float duration_uno, distance_uno;
float duration_dos, distance_dos;

/// @brief Constant, alert the LED when the calculated distance is less than this value.
float ALARM_DISTANCE = 10.0;

/// @brief Externing functions from the Distance_Alarm.S file.
extern "C"{
    void set_output_enable();
    void trigger_pin();
    void reset_pin();
    void trigger_pin_dos();
    void reset_pin_dos();
    void led_high();
    void led_low();
    void set_output_enable_led();
} 

// --------------Setup & Main Loop -------------- //

void setup() {

  pinMode(echo_uno, INPUT);
  pinMode(echo_dos, INPUT);

  set_output_enable();
  pinMode(led_uno, OUTPUT); //unable to set pin 5, no known reason. documentation checked

  Serial.println("Starting to measure the distance: ");
  Serial.begin(115200);
  delay(400);
}

void loop() {

  trigger_pin(); 
  //instead of what is below
  // digitalWrite(trigger_uno,HIGH);
  // delayMicroseconds(10);
  // digitalWrite(trigger_uno,LOW);

  duration_uno = pulseIn(echo_uno, HIGH);
  distance_uno = 0.0343 * duration_uno / 2;

  Serial.print("Distance Uno: "); Serial.println(distance_uno);
  
  if(distance_uno <= ALARM_DISTANCE)
  {
    led_high();
    delay(100);
  }else
  {
    led_low();
    delayMicroseconds(35);
  }

  trigger_pin_dos();

  duration_dos = pulseIn(echo_dos, HIGH);
  distance_dos = 0.0343 * duration_dos / 2;
  Serial.print("Distance Dos: "); Serial.println(distance_dos);
  if(distance_dos <= ALARM_DISTANCE)
  {
    led_high();
    delay(200);
  }else
  {
    led_low();
    delayMicroseconds(35);
  }

}

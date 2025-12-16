#define echo_uno 0 //3 0
#define trigger_uno 1 //0 1
#define echo_dos 2 //4 2
#define trigger_dos 3 //1 3
#define led_uno 5 //2 4

float duration_uno, distance_uno;
float duration_dos, distance_dos;
float alarm_distance = 15.0;

extern "C"{
    void set_output_enable();
    void trigger_pin();
    void reset_pin();
    void trigger_pin_dos();
    void reset_pin_dos();
    void led_high();
    void led_low();
} 

void setup() {

  pinMode(echo_uno, INPUT);
  pinMode(echo_dos, INPUT);

  set_output_enable();
  pinMode(led_uno, OUTPUT); //will need to check the assembly for why it is not setting properly

  Serial.println("Starting to measure the distance: ");
  Serial.begin(115200);
  delay(400);
}

void loop() {

  trigger_pin(); //instead of what is below
  // digitalWrite(trigger_uno,HIGH);
  // delayMicroseconds(10);
  // digitalWrite(trigger_uno,LOW);

  duration_uno = pulseIn(echo_uno, HIGH);
  distance_uno = 0.0343 * duration_uno / 2; //this will stay in c++ due to 

  Serial.print("Distance Uno: "); Serial.println(distance_uno);
  
  if(distance_uno <= alarm_distance)
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
  distance_dos = 0.0343 * duration_dos / 2; //this will stay in c++ due to 
  Serial.print("Distance Dos: "); Serial.println(distance_dos);
  if(distance_dos <= alarm_distance)
  {
    led_high();
    delay(200);
  }else
  {
    led_low();
    delayMicroseconds(35);
  }

}

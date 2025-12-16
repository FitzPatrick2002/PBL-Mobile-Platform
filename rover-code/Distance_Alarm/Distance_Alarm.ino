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
  // put your setup code here, to run once:
  // pinMode(trigger_uno, OUTPUT);
  // pinMode(trigger_dos, OUTPUT);
  pinMode(echo_uno, INPUT);
  pinMode(echo_dos, INPUT);

  //pinMode(led_uno, OUTPUT);

  Serial.begin(115200);
  delay(400);
  Serial.println("Starting to measure the distance: ");
  volatile uint32_t* enable_reg = (volatile uint32_t*)0x60004020;
  Serial.print("PRZED set_output_enable: 0x");
  Serial.println(*enable_reg, BIN);
  
  //pinMode(led_uno, OUTPUT);
  set_output_enable();
  pinMode(led_uno, OUTPUT);
  // Sprawdź PO
  Serial.print("PO set_output_enable: 0x");
  Serial.println(*enable_reg, BIN);
  //set_output_enable();

  
}

void loop() {

  
  //digitalWrite(led_uno, LOW);
  //digitalWrite(trigger_uno,LOW); //just in case of any delays, can later do it in assembly
  delayMicroseconds(5);

  trigger_pin(); //instead of what is below
  // digitalWrite(trigger_uno,HIGH);
  // delayMicroseconds(10);
  // digitalWrite(trigger_uno,LOW);

  duration_uno = pulseIn(echo_uno, HIGH);
  distance_uno = 0.0343 * duration_uno / 2; //this will stay in c++ due to 

  //distance = calculate_distance(0.034f, duration);

  Serial.print("Distance Uno: "); Serial.println(distance_uno);
  
  if(distance_uno <= alarm_distance)
  {
    //digitalWrite(led_uno, HIGH);
    led_high();
    delay(100);
  }else
  {
    //digitalWrite(led_uno, LOW);
    led_low();
    delayMicroseconds(35);

  }

 trigger_pin_dos();
  // digitalWrite(trigger_dos, LOW);
  // delayMicroseconds(5);
  // digitalWrite(trigger_dos,HIGH);
  // delayMicroseconds(10);
  // digitalWrite(trigger_dos,LOW);

  duration_dos = pulseIn(echo_dos, HIGH); //to do in .S - read analog value from pin number 0
  distance_dos = 0.0343 * duration_dos / 2; //this will stay in c++ due to 
  Serial.print("Distance Dos: "); Serial.println(distance_dos);
  if(distance_dos <= alarm_distance)
  {
    //digitalWrite(led_uno, HIGH);
    led_high();
    delay(200);
  }else
  {
    //digitalWrite(led_uno, LOW);
    led_low();
    delayMicroseconds(35);
  }
  
  //delay(200);
}

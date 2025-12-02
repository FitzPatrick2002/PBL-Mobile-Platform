#define echo_uno 0
#define trigger_uno 1
#define echo_dos 2
#define trigger_dos 3
#define led_uno 4
#define led_dos 5

float duration_uno;
float distance_uno;
float duration_dos;
float distance_dos;

extern "C" void set_output_enable();
extern "C" void trigger_pin();
extern "C" void reset_pin();

void setup() {
  // put your setup code here, to run once:
  pinMode(trigger_uno, OUTPUT);
  pinMode(trigger_dos, OUTPUT);
  //set_output_enable();
  pinMode(echo_uno, INPUT);
  pinMode(echo_dos, INPUT);

  pinMode(led_uno, OUTPUT);
  Serial.begin(115200);
  delay(400);
  Serial.println("Starting to measure the distance: ");
}

void loop() {

  //digitalWrite(led_uno, LOW);
  digitalWrite(trigger_uno,LOW); //just in case of any delays, can later do it in assembly
  delayMicroseconds(5);

  trigger_pin(); //instead of what is below
  // digitalWrite(trigger_uno,HIGH);
  // delayMicroseconds(10);
  // digitalWrite(trigger_uno,LOW);

  duration_uno = pulseIn(echo_uno, HIGH); //to do in .S - read analog value from pin number 0
  distance_uno = 0.0343 * duration_uno / 2; //this will stay in c++ due to 

  //distance = calculate_distance(0.034f, duration);

  Serial.print("Distance Uno: "); Serial.println(distance_uno);
  
  delayMicroseconds(35);

  digitalWrite(trigger_dos, LOW);
  delayMicroseconds(5);
  digitalWrite(trigger_dos,HIGH);
  delayMicroseconds(10);
  digitalWrite(trigger_dos,LOW);

  duration_dos = pulseIn(echo_dos, HIGH); //to do in .S - read analog value from pin number 0
  distance_dos = 0.0343 * duration_dos / 2; //this will stay in c++ due to 
  Serial.print("Distance Dos: "); Serial.println(distance_dos);
  if(distance_uno <= 10.0 || distance_dos <= 10.0)
  {
    digitalWrite(led_uno, HIGH);
    delay(200);
  }else
  {
    digitalWrite(led_uno, LOW);
  }
  delayMicroseconds(35);
  //delay(200);
}

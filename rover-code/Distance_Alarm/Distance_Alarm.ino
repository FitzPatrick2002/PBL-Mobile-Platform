#define echo 0
#define trigger 1

float duration;
float distance;

extern "C" void set_output_enable();
extern "C" void trigger_pin();
extern "C" void reset_pin();

void setup() {
  // put your setup code here, to run once:
  //pinMode(trigger, OUTPUT);
  set_output_enable();
  pinMode(echo, INPUT);

  Serial.begin(115200);
  delay(400);
  Serial.println("Starting to measure the distance: ");
}

void loop() {

  digitalWrite(trigger,LOW); //just in case of any delays, can later do it in assembly
  delayMicroseconds(5);

  trigger_pin(); //instead of what is below
  // digitalWrite(trigger,HIGH);
  // delayMicroseconds(10);
  // digitalWrite(trigger,LOW);

  duration = pulseIn(echo, HIGH); //to do in .S - read analog value from pin number 0
  distance = 0.0343 * duration / 2; //this will stay in c++ due to 

  //distance = calculate_distance(0.034f, duration);

  Serial.print("Distance: "); Serial.println(distance);

  delay(100);
}

/*

*/

#define CRVENA 16
#define ZELENA 17

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(CRVENA, OUTPUT);
  pinMode(ZELENA, OUTPUT);
}

// the loop function runs over and over again forever
void loop() { 
  digitalWrite(CRVENA, HIGH);   // turn the LED off by making the voltage LOW
  delay(2000);                      // wait for a second
  digitalWrite(CRVENA, LOW);   // turn the LED off by making the voltage LOW
  delay(1000);                      // wait for a second
  digitalWrite(ZELENA, HIGH);  // turn the LED on (HIGH is the voltage level)
  delay(2000);                      // wait for a second
  digitalWrite(ZELENA, LOW);  // turn the LED on (HIGH is the voltage level)
  delay(2000); 
}

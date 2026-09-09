/*
Filename: esp32_keypad_input.ino
Description: Reads input from a 4x4 keypad and prints pressed keys to the Serial Monitor
Author: www.oceanlabz.in
Modification: 1/4/2025
*/
 
 
#include <Keypad.h>
 
const byte ROWS = 4;
const byte COLS = 4;
 
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
 
byte rowPins[ROWS] = {13, 12, 14, 27}; // Connect to R1-R4
byte colPins[COLS] = {26, 25, 33, 32}; // Connect to C1-C4
 
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
 
void setup() {
  Serial.begin(115200);
  keypad.setHoldTime(200); 		// minimalno vrijeme pritiska tipke
  keypad.setDebounceTime(50); 	// vrijeme eliminiranja treperenja

}
 
void loop() {
  char key = keypad.getKey();
 
  if (key) {
    Serial.print("Key Pressed: ");
    Serial.println(key);
  }
  delay(100);
}

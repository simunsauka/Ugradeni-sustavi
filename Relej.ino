

#define REL 15



void setup() {
  
  pinMode(REL, OUTPUT);
}


void loop() { 

  digitalWrite(REL, LOW);   
  delay(30000);                      
  digitalWrite(REL, HIGH);  
  delay(2000);                      

}



#define CRVENA 16
#define ZELENA 17


void setup() {
  
  pinMode(CRVENA, OUTPUT);
  pinMode(ZELENA, OUTPUT);
}


void loop() { 
  digitalWrite(CRVENA, HIGH);   
  delay(2000);                     
  digitalWrite(CRVENA, LOW);   
  delay(1000);                     
  digitalWrite(ZELENA, HIGH);  
  delay(2000);                      
  digitalWrite(ZELENA, LOW);  
  delay(2000); 
}

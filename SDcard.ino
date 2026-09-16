

#include <SPI.h>
#include <SD.h>

File myFile;
const int CS = 5;

void WriteFile(const char * path, const char * message){
  
  myFile = SD.open(path, FILE_WRITE);
  
  if (myFile) {
    Serial.printf("Writing to %s ", path);
    myFile.println(message);
    myFile.close(); // close the file:
    Serial.println("completed.");
  } 
  
  else {
    Serial.println("error opening file ");
    Serial.println(path);
  }
}


void ReadFile(const char * path){
  
  myFile = SD.open(path);
  if (myFile) {
     Serial.printf("Reading file from %s\n", path);
     
    while (myFile.available()) {
      Serial.write(myFile.read());
    }
    myFile.close(); // close the file:
  } 
  else {
    
    Serial.println("error opening test.txt");
  }
}

void setup() {
  Serial.begin(9600);    
  delay(500);
  while (!Serial) { ; }  
  Serial.println("Initializing SD card...");
  if (!SD.begin(CS)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");

  WriteFile("/test.txt", "ElectronicWings.com");
  ReadFile("/test02.txt");
}

void loop() {
  
}

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

int sati = 16;
int minute = 54;
int sekunde = 0;

unsigned long previousMillis = 0;

void setup() {
  Serial.begin(115200);
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("SSD1306 inicijalizacija neuspješna"));
    for(;;);
  }
  
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
}

void loop() {
  unsigned long currentMillis = millis();
  
  if(currentMillis - previousMillis >= 1000) {
    previousMillis = currentMillis;
    
    sekunde++;
    if(sekunde >= 60) {
      sekunde = 0;
      minute++;
      if(minute >= 60) {
        minute = 0;
        sati++;
        if(sati >= 24) {
          sati = 0;
        }
      }
    }
    
    display.clearDisplay();
    
    // GORNJI ŽUTI DIO (0-16 piksela)
    display.setTextSize(2);
    
    char tekst[] = "ZATVORENO";
    display.setCursor(10, 0);
    display.println(tekst);
    
    // DONJI PLAVI DIO (17-63 piksela)
    display.setTextSize(3);

    display.setCursor(0, 30);
    display.print(sati);
    display.setCursor(32, 30);
    display.print(":");
    display.setCursor(46, 30);
    display.print(minute);
    display.setCursor(78, 30);
    display.print(":");
    display.setCursor(92, 30);
    display.print(sekunde);
    
    display.display();
  }
}
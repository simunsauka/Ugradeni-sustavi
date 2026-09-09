// Pin na koji je spojen buzzer
#define BUZZER_PIN 2

// Definiranje nota (frekvencije u Hz)
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_C5  523

int razmak = 500; // milisekunde

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
  Serial.begin(115200);
  Serial.println("ESP32 Buzzer program pokrenut");
}

void loop() {
  // Sviranje melodije
  playMelody();
  
  delay(2000); // Pauza između ponavljanja
}

void playMelody() {
    tone(BUZZER_PIN, NOTE_C4, 500);
    delay(razmak);
    noTone(BUZZER_PIN);

    tone(BUZZER_PIN, NOTE_D4, 500);
    delay(razmak);
    noTone(BUZZER_PIN);

    tone(BUZZER_PIN, NOTE_E4, 500);
    delay(razmak);
    noTone(BUZZER_PIN);

    tone(BUZZER_PIN, NOTE_C4, 500);
    delay(razmak);
    noTone(BUZZER_PIN);

    tone(BUZZER_PIN, NOTE_E4, 500);
    delay(razmak);
    noTone(BUZZER_PIN);

    tone(BUZZER_PIN, NOTE_F4, 500);
    delay(razmak);
    noTone(BUZZER_PIN);

    tone(BUZZER_PIN, NOTE_G4, 500);
    delay(razmak);
    noTone(BUZZER_PIN);

    tone(BUZZER_PIN, NOTE_C5, 500);
    delay(razmak);
    noTone(BUZZER_PIN);
}

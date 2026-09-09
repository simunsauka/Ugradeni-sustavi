/*
 * ============================================================
 *  SUSTAV KONTROLE PRISTUPA – DOIT ESP32 DEVKIT V1
 * ============================================================
 *  Hardver:
 *    Relej          → GPIO15
 *    Buzzer         → GPIO2
 *    Crvena LED     → GPIO16
 *    Zelena LED     → GPIO17
 *    RTC DS1307     → SDA=GPIO21, SCL=GPIO22
 *    OLED 0.96"     → SDA=GPIO21, SCL=GPIO22  (adresa 0x3C)
 *    SD kartica     → CS=GPIO5, SCK=GPIO18, MISO=GPIO19, MOSI=GPIO23
 *    Tipkovnica 4x4 → R1=13,R2=12,R3=14,R4=27 | C1=26,C2=25,C3=33,C4=32
 *
 *  Potrebne knjižnice (Library Manager):
 *    - RTClib          (Adafruit)
 *    - Adafruit SSD1306
 *    - Adafruit GFX Library
 *    - Keypad          (Mark Stanley, Alexander Brevig)
 *    - SD              (ugrađena u Arduino/ESP32 core)
 *    - WiFi            (ugrađena u ESP32 core)
 *    - WebServer       (ugrađena u ESP32 core)
 * ============================================================
 */

#include <Wire.h>
#include <RTClib.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Keypad.h>
#include <SD.h>
#include <SPI.h>
#include <WiFi.h>
#include <WebServer.h>

// ─── WiFi AP postavke ────────────────────────────────────────
// ESP32 radi kao Access Point – nije potrebna vanjska mreža
// Spoji se na "KontrolaPristupa" i otvori http://192.168.4.1
const char* AP_SSID     = "KontrolaPristupa";
const char* AP_PASSWORD = "12345678";

// ─── PIN-ovi korisnika ────────────────────────────────────────
// Format: { "KorisnickoIme", "PIN" }
// Dodajte/izmijenite po potrebi
struct User {
  const char* name;
  const char* pin;
};

const User USERS[] = {
  { "Admin",    "1234" },
  { "Korisnik1","1111" },
  { "Korisnik2","2222" },
  { "Korisnik3","3333" },
  { "Korisnik4","4444" }
};
const int USER_COUNT = sizeof(USERS) / sizeof(USERS[0]);

// ─── Maksimalni broj neuspjelih pokušaja ─────────────────────
const int MAX_FAILED = 3;
const unsigned long LOCKOUT_MS = 30000; // 30 sekundi blokiranja

// ─── GPIO definicije ─────────────────────────────────────────
#define PIN_RELAY     15
#define PIN_BUZZER    2
#define PIN_LED_RED   16
#define PIN_LED_GREEN 17
#define PIN_SD_CS     5

// ─── OLED display ─────────────────────────────────────────────
#define OLED_WIDTH  128
#define OLED_HEIGHT 64
#define OLED_ADDR   0x3C
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

// ─── RTC ─────────────────────────────────────────────────────
RTC_DS1307 rtc;

// ─── Tipkovnica 4x4 ──────────────────────────────────────────
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {13, 12, 14, 27};
byte colPins[COLS] = {26, 25, 33, 32};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ─── Web server ───────────────────────────────────────────────
WebServer server(80);

// ─── Log evidencija (zadnjih N zapisa u RAM-u za web prikaz) ──
#define LOG_BUFFER_SIZE 20
struct LogEntry {
  char timestamp[20];
  char user[20];
  bool success;
  char note[30];
};
LogEntry logBuffer[LOG_BUFFER_SIZE];
int logHead = 0;
int logCount = 0;

// ─── Stanje sustava ──────────────────────────────────────────
String inputPin = "";
int failedAttempts = 0;
bool isLockedOut = false;
unsigned long lockoutStart = 0;
bool doorOpen = false;
unsigned long doorOpenTime = 0;
const unsigned long DOOR_OPEN_MS = 5000; // vrata otvorena 5 sekundi

// ─── Globalni status string za OLED/web ──────────────────────
String systemStatus = "Spreman";

// ============================================================
//  POMOĆNE FUNKCIJE
// ============================================================

String getTimestamp() {
  if (!rtc.isrunning()) return "0000-00-00 00:00:00";
  DateTime now = rtc.now();
  char buf[20];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
           now.year(), now.month(), now.day(),
           now.hour(), now.minute(), now.second());
  return String(buf);
}

void addLog(const char* user, bool success, const char* note) {
  String ts = getTimestamp();
  LogEntry& e = logBuffer[logHead];
  strncpy(e.timestamp, ts.c_str(), sizeof(e.timestamp)-1);
  strncpy(e.user, user, sizeof(e.user)-1);
  e.success = success;
  strncpy(e.note, note, sizeof(e.note)-1);
  logHead = (logHead + 1) % LOG_BUFFER_SIZE;
  if (logCount < LOG_BUFFER_SIZE) logCount++;

  // Zapis na SD karticu
  File f = SD.open("/log.csv", FILE_APPEND);
  if (f) {
    f.printf("%s,%s,%s,%s\n",
             e.timestamp,
             e.user,
             success ? "USPJEH" : "NEUSPJEH",
             e.note);
    f.close();
  }
}

// ─── OLED ────────────────────────────────────────────────────
// Ispiši tekst horizontalno centriran na zadanom retku (y koordinata)
void oledPrintCentered(const String& text, int y, uint8_t textSize = 1) {
  display.setTextSize(textSize);
  int16_t charW = 6 * textSize; // SSD1306 font: 6px po znaku pri size=1
  int16_t textW = text.length() * charW;
  int16_t x = (OLED_WIDTH - textW) / 2;
  if (x < 0) x = 0;
  display.setCursor(x, y);
  display.print(text);
}

void oledClear() {
  display.clearDisplay();
  display.setCursor(0, 0);
}

// Prikazuje 4 retka; prvi redak (naslov) uvijek je centriran
void oledShow(const String& line1,
              const String& line2 = "",
              const String& line3 = "",
              const String& line4 = "") {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // Redak 1 – centriran naslov
  oledPrintCentered(line1, 0);

  // Redak 2‑4 – lijevo poravnanje
  display.setTextSize(1);
  display.setCursor(0, 16); display.println(line2);
  display.setCursor(0, 32); display.println(line3);
  display.setCursor(0, 48); display.println(line4);
  display.display();
}

void showReadyScreen() {
  String ts = getTimestamp();
  oledShow("=== PRISTUP ===",
           "Unesite PIN:",
           inputPin,
           ts.substring(0,16));
}

// ─── Buzzer ──────────────────────────────────────────────────
void beepOK() {
  tone(PIN_BUZZER, 1000, 200);
  delay(250);
  tone(PIN_BUZZER, 1500, 200);
  delay(250);
  noTone(PIN_BUZZER);
}

void beepFail() {
  tone(PIN_BUZZER, 400, 500);
  delay(600);
  noTone(PIN_BUZZER);
}

void beepKeypress() {
  tone(PIN_BUZZER, 1200, 80);
  delay(90);
  noTone(PIN_BUZZER);
}

// ─── LED ─────────────────────────────────────────────────────
void ledGreen(bool on) { digitalWrite(PIN_LED_GREEN, on ? HIGH : LOW); }
void ledRed(bool on)   { digitalWrite(PIN_LED_RED,   on ? HIGH : LOW); }

// ─── Relej (brava) ────────────────────────────────────────────
void unlockDoor() {
  digitalWrite(PIN_RELAY, HIGH); // relej aktiviran = brava otvorena
  doorOpen = true;
  doorOpenTime = millis();
  ledRed(false);   // vrata otvorena → crvena UGAŠENA
  ledGreen(true);  // zelena UPALJENA
}

void lockDoor() {
  digitalWrite(PIN_RELAY, LOW);
  doorOpen = false;
  ledGreen(false); // zelena UGAŠENA
  ledRed(true);    // vrata zaključana → crvena UPALJENA
}

// ─── Provjera PIN-a ──────────────────────────────────────────
int checkPin(const String& pin) {
  for (int i = 0; i < USER_COUNT; i++) {
    if (pin == String(USERS[i].pin)) return i;
  }
  return -1;
}

// ─── Autentikacija ────────────────────────────────────────────
void handleAccessGranted(int userIdx) {
  failedAttempts = 0;
  systemStatus = String("OK: ") + USERS[userIdx].name;

  beepOK();
  unlockDoor(); // brine za LED-ove

  oledShow(">>> PRISTUP OK <<<",
           String("Dobrodosli,"),
           String(USERS[userIdx].name),
           "Vrata otvorena...");

  addLog(USERS[userIdx].name, true, "Pristup odobren");
  Serial.printf("[%s] PRISTUP ODOBREN: %s\n",
                getTimestamp().c_str(), USERS[userIdx].name);
}

void handleAccessDenied(const String& triedPin) {
  failedAttempts++;
  systemStatus = "ODBIJEN";

  beepFail(); // crvena već stalno svijetli – nema promjene LED-a

  char note[30];
  snprintf(note, sizeof(note), "Neuspjeh %d/%d", failedAttempts, MAX_FAILED);
  addLog("???", false, note);
  Serial.printf("[%s] PRISTUP ODBIJEN (pokusaj %d)\n",
                getTimestamp().c_str(), failedAttempts);

  if (failedAttempts >= MAX_FAILED) {
    isLockedOut = true;
    lockoutStart = millis();
    systemStatus = "BLOKIRAN";
    oledShow("!!! BLOKIRAN !!!",
             "Previse pogresnih",
             "PIN-ova!",
             "Cekajte 30s...");
    addLog("SUSTAV", false, "Zakljucano 30s");
    Serial.println("[UPOZORENJE] Sustav blokiran 30 sekundi!");
    delay(3000);
  } else {
    char buf[20];
    snprintf(buf, sizeof(buf), "Pokusaj %d/%d", failedAttempts, MAX_FAILED);
    oledShow("!!! POGRESNO !!!",
             "Neispravni PIN",
             String(buf),
             "Pokusajte ponovo");
    delay(2000);
  }

  ledRed(false);
}

// ─── Web server handleri ──────────────────────────────────────
void handleWebRoot() {
  String ts = getTimestamp();

  String html = R"rawhtml(
<!DOCTYPE html>
<html lang='hr'>
<head>
<meta charset='UTF-8'>
<meta http-equiv='refresh' content='10'>
<meta name='viewport' content='width=device-width, initial-scale=1'>
<title>Kontrola Pristupa</title>
<style>
  body{font-family:Arial,sans-serif;background:#1a1a2e;color:#eee;margin:0;padding:20px}
  h1{color:#00d4ff;text-align:center;border-bottom:2px solid #00d4ff;padding-bottom:10px}
  .card{background:#16213e;border-radius:10px;padding:15px;margin:15px 0;border:1px solid #0f3460}
  .status{font-size:1.4em;font-weight:bold;text-align:center;padding:10px}
  .ok{color:#00ff88}
  .fail{color:#ff4444}
  .warn{color:#ffaa00}
  table{width:100%;border-collapse:collapse}
  th{background:#0f3460;padding:8px;text-align:left}
  td{padding:6px 8px;border-bottom:1px solid #0f3460}
  tr:hover{background:#0f3460}
  .badge-ok{background:#00ff88;color:#000;padding:2px 8px;border-radius:4px;font-size:.85em}
  .badge-fail{background:#ff4444;color:#fff;padding:2px 8px;border-radius:4px;font-size:.85em}
  footer{text-align:center;color:#666;font-size:.8em;margin-top:20px}
</style>
</head>
<body>
<h1>🔐 Kontrola Pristupa</h1>
)rawhtml";

  // Status kartica
  String statusClass = "ok";
  if (systemStatus.startsWith("BLOK") || systemStatus.startsWith("ODBIJEN"))
    statusClass = "fail";
  else if (doorOpen)
    statusClass = "warn";

  html += "<div class='card'>";
  html += "<div class='status " + statusClass + "'>Status: " + systemStatus + "</div>";
  html += "<p>Brava: <strong>" + String(doorOpen ? "OTVORENA 🔓" : "ZATVORENA 🔒") + "</strong></p>";
  html += "<p>Blokiranje: <strong>" + String(isLockedOut ? "DA ⚠️" : "NE") + "</strong></p>";
  html += "<p>Neuspjeli pokušaji: <strong>" + String(failedAttempts) + "/" + String(MAX_FAILED) + "</strong></p>";
  html += "<p>Vrijeme: <strong>" + ts + "</strong></p>";
  html += "<p>IP adresa: <strong>" + WiFi.softAPIP().toString() + "</strong></p>";
  html += "</div>";

  // Tablica događaja
  html += "<div class='card'><h2>📋 Zadnji događaji</h2>";
  if (logCount == 0) {
    html += "<p style='text-align:center;color:#888'>Nema zapisanih događaja.</p>";
  } else {
    html += "<table><tr><th>Datum i Vrijeme</th><th>Korisnik</th><th>Status</th><th>Bilješka</th></tr>";

    // Prikaži od najnovijeg prema starijem
    for (int i = logCount - 1; i >= 0; i--) {
      int idx = (logHead - 1 - i + LOG_BUFFER_SIZE) % LOG_BUFFER_SIZE;
      LogEntry& e = logBuffer[idx];
      html += "<tr>";
      html += "<td>" + String(e.timestamp) + "</td>";
      html += "<td>" + String(e.user) + "</td>";
      html += "<td>";
      if (e.success)
        html += "<span class='badge-ok'>USPJEH</span>";
      else
        html += "<span class='badge-fail'>NEUSPJEH</span>";
      html += "</td>";
      html += "<td>" + String(e.note) + "</td>";
      html += "</tr>";
    }
    html += "</table>";
  }
  html += "</div>";

  html += "<footer>ESP32 Kontrola Pristupa | Stranica se osvježava svakih 10s</footer>";
  html += "</body></html>";

  server.send(200, "text/html; charset=UTF-8", html);
}

void handleWebLog() {
  // Vraća CSV log sa SD kartice
  File f = SD.open("/log.csv", FILE_READ);
  if (!f) {
    server.send(404, "text/plain", "Log datoteka nije pronađena.");
    return;
  }
  String content = "timestamp,korisnik,status,biljeska\n";
  while (f.available()) {
    content += (char)f.read();
  }
  f.close();
  server.send(200, "text/csv; charset=UTF-8", content);
}

void handleWebNotFound() {
  server.send(404, "text/plain", "404 – Stranica nije pronađena.");
}

// ============================================================
//  SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  Serial.println("\n=== ESP32 Kontrola Pristupa – START ===");

  // GPIO
  pinMode(PIN_RELAY,     OUTPUT); digitalWrite(PIN_RELAY, LOW);
  pinMode(PIN_LED_RED,   OUTPUT); digitalWrite(PIN_LED_RED, LOW);
  pinMode(PIN_LED_GREEN, OUTPUT); digitalWrite(PIN_LED_GREEN, LOW);
  pinMode(PIN_BUZZER,    OUTPUT); noTone(PIN_BUZZER);

  // I2C (RTC + OLED)
  Wire.begin(21, 22);

  // OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("[UPOZORENJE] OLED nije pronađen!");
  } else {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.display();
    Serial.println("[OK] OLED inicijaliziran.");
  }

  oledShow("Inicijalizacija...", "", "", "");

  // RTC
  if (!rtc.begin()) {
    Serial.println("[UPOZORENJE] RTC nije pronađen! Koristim dummy timestamp.");
    oledShow("Inicijalizacija...", "RTC: NIJE NADJEN!", "", "");
    delay(2000);
  } else {
    if (!rtc.isrunning()) {
      // Postavi na vrijeme kompajliranja samo ako RTC ne radi
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
      Serial.println("[INFO] RTC postavljen na compile-time.");
    }
    Serial.println("[OK] RTC inicijaliziran: " + getTimestamp());
  }

  // SD kartica
  SPI.begin(18, 19, 23, 5); // SCK, MISO, MOSI, CS
  if (!SD.begin(PIN_SD_CS)) {
    Serial.println("[UPOZORENJE] SD kartica nije pronađena! Logiranje onemogućeno.");
    oledShow("Inicijalizacija...", "SD: NIJE NADJENA!", "", "");
    delay(2000);
  } else {
    Serial.println("[OK] SD kartica inicijalizirana.");
    // Kreiraj zaglavlje ako datoteka ne postoji
    if (!SD.exists("/log.csv")) {
      File f = SD.open("/log.csv", FILE_WRITE);
      if (f) {
        f.println("timestamp,korisnik,status,biljeska");
        f.close();
      }
    }
  }

  // WiFi – Access Point mod
  oledShow("Pokretanje AP...", AP_SSID, "", "");
  Serial.printf("[INFO] Pokretanje AP: %s\n", AP_SSID);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  delay(500); // kratka pauza da AP stabilizira IP

  IPAddress apIP = WiFi.softAPIP();
  Serial.printf("[OK] AP aktivan. IP: %s\n", apIP.toString().c_str());
  oledShow("AP aktivan!", AP_SSID, "IP: " + apIP.toString(), "pw: " + String(AP_PASSWORD));
  delay(3000);

  // Web server rute
  server.on("/",       HTTP_GET, handleWebRoot);
  server.on("/log",    HTTP_GET, handleWebLog);
  server.onNotFound(handleWebNotFound);
  server.begin();
  Serial.println("[OK] Web server pokrenut.");

  // Gotovo – kratki signal (zelena trepne jednom, zatim se uključi crvena)
  ledGreen(true);
  tone(PIN_BUZZER, 1000, 100); delay(150);
  tone(PIN_BUZZER, 1500, 100); delay(150);
  tone(PIN_BUZZER, 2000, 100); delay(200);
  noTone(PIN_BUZZER);
  ledGreen(false);
  ledRed(true); // sustav spreman, vrata zaključana → crvena upaljena

  addLog("SUSTAV", true, "Sustav pokrenut");
  showReadyScreen();
  Serial.println("[OK] Sustav spreman.");
}

// ============================================================
//  LOOP
// ============================================================
void loop() {
  server.handleClient();

  // ── Provjera isteka blokade ──────────────────────────────
  if (isLockedOut) {
    unsigned long elapsed = millis() - lockoutStart;
    if (elapsed >= LOCKOUT_MS) {
      isLockedOut = false;
      failedAttempts = 0;
      systemStatus = "Spreman";
      inputPin = "";
      ledRed(true);  // vrata i dalje zaključana → crvena stalno upaljena
      Serial.println("[INFO] Blokada istekla. Sustav spreman.");
      addLog("SUSTAV", true, "Blokada istekla");
      showReadyScreen();
    } else {
      // Prikaži odbrojavanje
      int remaining = (LOCKOUT_MS - elapsed) / 1000 + 1;
      ledRed((millis() / 500) % 2); // trepćuća crvena
      oledShow("!!! BLOKIRAN !!!",
               "Cekajte jos:",
               String(remaining) + " sekundi",
               "");
      return; // Ne obrađuj tipkovnicu dok je blokada aktivna
    }
  }

  // ── Automatsko zatvaranje brave ───────────────────────────
  if (doorOpen && (millis() - doorOpenTime >= DOOR_OPEN_MS)) {
    lockDoor(); // uključuje crvenu, gasi zelenu
    systemStatus = "Spreman";
    inputPin = "";
    Serial.println("[INFO] Brava zatvorena (timeout).");
    addLog("SUSTAV", true, "Brava zatvorena");
    showReadyScreen();
  }

  // ── Čitanje tipkovnice ───────────────────────────────────
  if (doorOpen) return; // Dok su vrata otvorena ne primamo input

  char key = keypad.getKey();
  if (!key) return;

  beepKeypress();
  Serial.printf("[TIPKA] '%c'\n", key);

  // '#' = potvrda PIN-a
  if (key == '#') {
    if (inputPin.length() == 0) {
      showReadyScreen();
      return;
    }

    int userIdx = checkPin(inputPin);
    if (userIdx >= 0) {
      handleAccessGranted(userIdx);
    } else {
      handleAccessDenied(inputPin);
    }
    inputPin = "";
    if (!doorOpen && !isLockedOut) showReadyScreen();

  // '*' = brisanje unosa
  } else if (key == '*') {
    inputPin = "";
    systemStatus = "Spreman";
    Serial.println("[INFO] Unos obrisan.");
    showReadyScreen();

  // Brojevi / slova = dodaj u PIN buffer
  } else {
    if (inputPin.length() < 8) { // max 8 znakova
      inputPin += key;
    }
    // Prikaži unesene znakove
    oledShow("=== PRISTUP ===",
             "Unesite PIN:",
             inputPin,
             getTimestamp().substring(0,16));
  }
}

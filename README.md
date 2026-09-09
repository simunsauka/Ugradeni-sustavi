# ESP32 sustav kontrole pristupa

Projekt predstavlja sustav kontrole pristupa temeljen na **DOIT ESP32 DEVKIT V1** razvojnoj pločici. Korisnik unosi PIN putem 4×4 tipkovnice, a ESP32 provjerava PIN, upravlja relejem električne brave, daje zvučnu i svjetlosnu povratnu informaciju te zapisuje događaje na SD karticu. Status sustava i evidencija pristupa dostupni su i kroz ugrađeno web sučelje.

Projekt sadrži dvije verzije programa:

- **AP (Access Point) način rada** – ESP32 stvara vlastitu Wi‑Fi mrežu.
- **STA (Station) način rada** – ESP32 se spaja na postojeću Wi‑Fi mrežu.

## Funkcionalnosti

- unos PIN-a preko 4×4 membranske tipkovnice
- `#` potvrđuje uneseni PIN
- `*` briše trenutni unos
- podrška za više korisnika definiranih u programu
- PIN može sadržavati najviše 8 znakova
- prikaz statusa i vremena na OLED zaslonu
- automatsko otključavanje vrata nakon ispravnog PIN-a
- automatsko ponovno zaključavanje nakon 5 sekundi
- blokada sustava na 30 sekundi nakon 3 uzastopna pogrešna pokušaja
- crvena i zelena LED indikacija stanja
- zvučna signalizacija buzzerom
- RTC DS1307 za datum i vrijeme događaja
- zapis događaja u `/log.csv` na SD kartici
- web sučelje sa statusom sustava i zadnjih 20 događaja
- CSV evidencija dostupna putem rute `/log`

## Potreban hardver

| Komponenta | ESP32 priključak |
|---|---|
| Relej | GPIO15 |
| Buzzer | GPIO2 |
| Crvena LED | GPIO16 |
| Zelena LED | GPIO17 |
| RTC DS1307 SDA | GPIO21 |
| RTC DS1307 SCL | GPIO22 |
| OLED SDA | GPIO21 |
| OLED SCL | GPIO22 |
| SD CS | GPIO5 |
| SD SCK | GPIO18 |
| SD MISO | GPIO19 |
| SD MOSI | GPIO23 |
| Tipkovnica R1–R4 | GPIO13, GPIO12, GPIO14, GPIO27 |
| Tipkovnica C1–C4 | GPIO26, GPIO25, GPIO33, GPIO32 |

OLED koristi I2C adresu `0x3C`. RTC i OLED dijele istu I2C sabirnicu.

Shema spajanja nalazi se u datoteci **`Spajanje.jpg`**.

## Potrebne Arduino knjižnice

Prije kompajliranja instalirajte sljedeće knjižnice kroz Arduino IDE Library Manager:

- `RTClib` – Adafruit
- `Adafruit SSD1306`
- `Adafruit GFX Library`
- `Keypad` – Mark Stanley / Alexander Brevig

Sljedeće knjižnice dolaze uz ESP32 Arduino core:

- `WiFi`
- `WebServer`
- `SD`
- `SPI`
- `Wire`

## Struktura projekta

```text
Sauka/
├── access_control_AP_mod/
│   └── access_control_AP_mod.ino
├── access_control_STA_mod/
│   └── access_control_STA_mod.ino
├── Spajanje.jpg
└── Opis programa.docx
```

## Pokretanje projekta

1. Spojite komponente prema shemi `Spajanje.jpg`.
2. U Arduino IDE-u instalirajte podršku za ESP32 i potrebne knjižnice.
3. Otvorite željenu `.ino` verziju programa.
4. Odaberite odgovarajuću ESP32 pločicu i serijski port.
5. Ako koristite **STA verziju**, u kodu postavite SSID i lozinku svoje Wi‑Fi mreže.
6. Učitajte program na ESP32.
7. Po želji otvorite Serial Monitor na `115200 baud` radi praćenja inicijalizacije i događaja.

> **Sigurnosna napomena:** Wi‑Fi lozinke i stvarne korisničke PIN-ove nemojte objavljivati u javnom repozitoriju. Prije objave projekta zamijenite ih primjerima ili ih izdvojite u lokalnu konfiguracijsku datoteku.

## AP način rada

Datoteka:

```text
access_control_AP_mod/access_control_AP_mod.ino
```

ESP32 radi kao vlastita pristupna točka. Nakon učitavanja programa:

1. spojite računalo ili mobitel na Wi‑Fi mrežu koju stvara ESP32
2. otvorite adresu `http://192.168.4.1`
3. prikazat će se web sučelje sustava kontrole pristupa

Naziv mreže i lozinka mogu se promijeniti u varijablama `AP_SSID` i `AP_PASSWORD`.

## STA način rada

Datoteka:

```text
access_control_STA_mod/access_control_STA_mod.ino
```

U ovoj verziji ESP32 se spaja na postojeći Wi‑Fi router. Prije učitavanja programa potrebno je postaviti:

```cpp
const char* WIFI_SSID = "NAZIV_MREZE";
const char* WIFI_PASSWORD = "LOZINKA_MREZE";
```

Nakon povezivanja, IP adresu ESP32 uređaja provjerite u Serial Monitoru ili na OLED zaslonu te je otvorite u web pregledniku.

## Korištenje tipkovnice

Unesite PIN brojčanim tipkama.

- `#` – potvrda PIN-a
- `*` – brisanje unosa
- maksimalna duljina PIN-a: 8 znakova

Korisnici i PIN-ovi definirani su u polju `USERS` unutar programa. Primjer:

```cpp
const User USERS[] = {
  { "Admin",     "1234" },
  { "Korisnik1", "1111" }
};
```

Za stvarnu uporabu preporučuje se promijeniti zadane PIN-ove.

## Ponašanje sustava

### Ispravan PIN

Kod ispravnog PIN-a sustav:

1. poništava broj neuspjelih pokušaja
2. reproducira pozitivan zvučni signal
3. aktivira relej
4. pali zelenu i gasi crvenu LED
5. prikazuje korisnika na OLED-u
6. zapisuje uspješan pristup u log
7. nakon 5 sekundi ponovno zaključava vrata

### Pogrešan PIN

Kod pogrešnog PIN-a:

- povećava se brojač neuspjelih pokušaja
- reproducira se negativni zvučni signal
- događaj se zapisuje u log
- nakon 3 uzastopna pogrešna pokušaja aktivira se blokada od 30 sekundi

Tijekom blokade crvena LED trepće, a OLED prikazuje preostalo vrijeme. Nakon isteka blokade sustav se automatski vraća u normalan način rada.

## LED i zvučna signalizacija

| Stanje | Indikacija |
|---|---|
| Vrata zaključana | crvena LED |
| Vrata otvorena | zelena LED |
| Uspješan PIN | dvotonski zvučni signal |
| Pogrešan PIN | dugi niski ton |
| Pritisak tipke | kratki beep |
| Sustav blokiran | trepćuća crvena LED |

## Evidencija na SD kartici

Događaji se spremaju u:

```text
/log.csv
```

CSV sadrži:

```text
timestamp,korisnik,status,biljeska
```

Primjer zapisa:

```text
2026-09-09 12:30:15,Admin,USPJEH,Pristup odobren
2026-09-09 12:31:02,???,NEUSPJEH,Neuspjeh 1/3
```

Ako `log.csv` ne postoji, program ga automatski kreira sa zaglavljem.

## Web sučelje

Web stranica automatski se osvježava svakih 10 sekundi i prikazuje:

- trenutni status sustava
- stanje brave
- informaciju je li sustav blokiran
- broj neuspjelih pokušaja
- trenutno RTC vrijeme
- IP adresu uređaja
- zadnjih 20 događaja pohranjenih u RAM-u

Dostupne rute:

| Ruta | Opis |
|---|---|
| `/` | glavno web sučelje |
| `/log` | kompletan CSV log sa SD kartice |

## RTC

DS1307 služi za dodavanje točnog datuma i vremena zapisima. Ako RTC radi, ali nije pokrenut, program ga postavlja na datum i vrijeme kompajliranja.

Ako RTC nije pronađen, sustav može nastaviti raditi, ali timestamp neće biti valjan.

## Napomene

- Provjerite odgovara li logika vašeg relejnog modula programu (`HIGH` = aktivan relej u postojećem kodu).
- SD kartica mora biti pravilno formatirana i spojena na SPI pinove.
- OLED i RTC koriste zajedničke SDA/SCL vodove.
- Za stvarni sustav kontrole pristupa preporučuje se dodatno zaštititi korisničke PIN-ove i mrežne podatke.
- Zadnjih 20 događaja web stranica čita iz RAM-a; potpuna povijest ostaje u `log.csv` na SD kartici.

## Autor / projekt

ESP32 sustav kontrole pristupa s PIN autentikacijom, OLED prikazom, RTC satom, SD evidencijom i web nadzorom.

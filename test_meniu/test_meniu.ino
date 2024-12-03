#include <Keypad.h>
#include <Wire.h>
#include <U8g2lib.h>

#define ENC_A 18
#define ENC_B 19
#define BTN_BACK 'B'
#define BTN_SELECT 'A'

U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0);

const byte ROWS = 4;
const byte COLS = 4;
char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {53, 51, 49, 47};
byte colPins[COLS] = {45, 43, 41, 39};
Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

volatile int menuIndex = 0;
volatile int currentMenu = 0;
volatile int addPasswordStep = 0;
volatile int scrollIndex = 0;

String domain = "";
String password = "";

#define EEPROM_ADDRESS 0x50 // Adresa I2C a memoriei EEPROM (presupunând că A0, A1, A2 sunt la GND)
#define EEPROM_SIZE 32768   // Dimensiunea memoriei EEPROM (32KB pentru AT24C256)
#define ENTRY_SIZE 32       // Lungimea maximă pentru fiecare înregistrare (domeniu + parolă)

// Protocoale funcții
void eepromWriteByte(unsigned int eeaddress, byte data);
byte eepromReadByte(unsigned int eeaddress);
void writeEEPROM(unsigned int eeaddress, byte *data, int length);
void readEEPROM(unsigned int eeaddress, byte *data, int length);

void setup() {
  // Setează pinii encoderului și atașează întreruperile
  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENC_A), read_encoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_B), read_encoder, CHANGE);

  // Initializează afișajul OLED
  u8g2.begin();
  u8g2.clearBuffer();

  // Pornește monitorul serial
  Serial.begin(115200);

  // Inițializează comunicația I2C
  Wire.begin();
}

void loop() {
  char customKey = customKeypad.getKey();
  if (customKey) {
    handleKeypadInput(customKey);
  }

  displayMenu();
}

void read_encoder() {
  static uint8_t old_AB = 3;
  static int8_t encval = 0;
  static const int8_t enc_states[] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};

  old_AB <<= 2;
  if (digitalRead(ENC_A)) old_AB |= 0x02;
  if (digitalRead(ENC_B)) old_AB |= 0x01;
  encval += enc_states[(old_AB & 0x0f)];

  if (encval >= 8) { // Prag crescut de sensibilitate
    if (currentMenu == 2 || currentMenu == 3) {
      scrollIndex++;
    } else {
      menuIndex++;
    }
    encval = 0;
    Serial.print("Encoder rotit la dreapta, menuIndex: ");
    Serial.println(menuIndex);
  } else if (encval <= -8) {
    if (currentMenu == 2 || currentMenu == 3) {
      scrollIndex--;
    } else {
      menuIndex--;
    }
    encval = 0;
    Serial.print("Encoder rotit la stânga, menuIndex: ");
    Serial.println(menuIndex);
  }

  // Corectează indexul meniului pentru a rămâne în limite
  if (menuIndex < 0) {
    menuIndex = 2;
  } else if (menuIndex > 2) {
    menuIndex = 0;
  }

  // Corectează scrollIndex pentru a rămâne în limite
  int totalEntries = EEPROM_SIZE / ENTRY_SIZE;
  if (scrollIndex < 0) {
    scrollIndex = totalEntries - 1;
  } else if (scrollIndex >= totalEntries) {
    scrollIndex = 0;
  }
}

void displayMenu() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_4x6_tr); // Font mai mic pentru spațiere mai bună

  if (currentMenu == 0) {
    switch (menuIndex) {
      case 0:
        u8g2.drawStr(0, 10, "> Adaugare Parola");
        u8g2.drawStr(0, 20, "  Vizualizare Parole");
        u8g2.drawStr(0, 30, "  Stergere Parola");
        break;
      case 1:
        u8g2.drawStr(0, 10, "  Adaugare Parola");
        u8g2.drawStr(0, 20, "> Vizualizare Parole");
        u8g2.drawStr(0, 30, "  Stergere Parola");
        break;
      case 2:
        u8g2.drawStr(0, 10, "  Adaugare Parola");
        u8g2.drawStr(0, 20, "  Vizualizare Parole");
        u8g2.drawStr(0, 30, "> Stergere Parola");
        break;
    }
  } else if (currentMenu == 1) {
    addPassword();
  } else if (currentMenu == 2) {
    displayPasswords();
  } else if (currentMenu == 3) {
    deletePassword();
  }

  u8g2.sendBuffer();
}

void handleKeypadInput(char key) {
  if (key == BTN_BACK) {
    // Revenire la meniul principal
    if (currentMenu != 0) {
      currentMenu = 0;
      menuIndex = 0;
      addPasswordStep = 0;
      domain = "";
      password = "";
      Serial.println("Butonul Înapoi apăsat, revenire la meniul principal");
    }
  } else if (key == BTN_SELECT) {
    // Gestionare logică buton Select pentru a intra în submeniu
    if (currentMenu == 0) {
      Serial.print("Opțiune selectată: ");
      Serial.println(menuIndex);
      switch (menuIndex) {
        case 0:
          currentMenu = 1;
          addPasswordStep = 0;
          break;
        case 1:
          currentMenu = 2;
          scrollIndex = 0;
          break;
        case 2:
          currentMenu = 3;
          scrollIndex = 0;
          break;
      }
    } else if (currentMenu == 1) {
      if (addPasswordStep == 0) {
        addPasswordStep = 1;
      } else if (addPasswordStep == 1) {
        Serial.print("Domeniu: ");
        Serial.println(domain);
        Serial.print("Parolă: ");
        Serial.println(password);
        // Salvează domeniul și parola în memoria EEPROM externă
        saveToMemory(domain, password);
        domain = "";
        password = "";
        addPasswordStep = 2;
      }
    } else if (currentMenu == 2) {
      // Trimite parola la serial când vizualizezi parolele
      String entry = readFromMemory(scrollIndex);
      Serial.print("Trimitere către PC: ");
      Serial.println(entry);
    } else if (currentMenu == 3) {
      // Șterge parola selectată
      deleteFromMemory(scrollIndex);
      Serial.print("Parolă ștearsă la indexul: ");
      Serial.println(scrollIndex);
    }
  } else {
    // Gestionare intrare alfanumerică pentru domeniu și parolă în meniul de adăugare parolă
    if (currentMenu == 1) {
      if (addPasswordStep == 0) {
        if (key != '#' && domain.length() < 16) {
          domain += key;
        }
      } else if (addPasswordStep == 1) {
        if (key != '#' && password.length() < 16) {
          password += key;
        }
      }
    }
  }
}

void addPassword() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_4x6_tr);
  if (addPasswordStep == 0) {
    u8g2.drawStr(0, 10, "Adaugare Domeniu:");
    u8g2.drawStr(0, 20, domain.c_str());
  } else if (addPasswordStep == 1) {
    u8g2.drawStr(0, 10, "Introdu Parola:");
    u8g2.drawStr(0, 20, password.c_str());
  } else if (addPasswordStep == 2) {
    u8g2.drawStr(0, 10, "Parola salvata cu succes!");
  }
  u8g2.sendBuffer();
}

void displayPasswords() {
  // Logică pentru afișarea parolelor din memoria EEPROM
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_4x6_tr);
  u8g2.drawStr(0, 10, "Parole Salvate:");

  for (int i = 0; i < 3; i++) {
    int totalEntries = EEPROM_SIZE / ENTRY_SIZE;
    int entryIndex = (scrollIndex + i) % totalEntries;
    String entry = readFromMemory(entryIndex);
    if (i == 0) {
      u8g2.drawStr(0, 20 + (i * 10), "> ");
    }
    u8g2.drawStr(10, 20 + (i * 10), entry.c_str());
  }

  u8g2.sendBuffer();
}

void deletePassword() {
  // Logică pentru ștergerea parolei din memoria EEPROM
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_4x6_tr);
  u8g2.drawStr(0, 10, "Sterge Parola:");

  String entry = readFromMemory(scrollIndex);
  u8g2.drawStr(0, 20, entry.c_str());

  u8g2.sendBuffer();
}

void saveToMemory(String domain, String password) {
  int address = findFreeSlot();
  if (address == -1) {
    Serial.println("Nu există slot liber în memoria EEPROM.");
    return;
  }

  String entry = domain + ":" + password;
  int length = entry.length();
  if (length > ENTRY_SIZE) {
    length = ENTRY_SIZE; // Trunchiază dacă este necesar
  }
  byte data[ENTRY_SIZE];
  for (int i = 0; i < ENTRY_SIZE; i++) {
    if (i < length) {
      data[i] = entry[i];
    } else {
      data[i] = 0;
    }
  }

  writeEEPROM(address, data, ENTRY_SIZE);

  Serial.print("Salvare în memorie: ");
  Serial.println(entry);
}

String readFromMemory(int index) {
  int address = index * ENTRY_SIZE;
  byte data[ENTRY_SIZE];
  readEEPROM(address, data, ENTRY_SIZE);

  String entry = "";
  for (int i = 0; i < ENTRY_SIZE; i++) {
    if (data[i] == 0 || data[i] == 0xFF) break;
    entry += (char)data[i];
  }
  return entry;
}

void deleteFromMemory(int index) {
  int address = index * ENTRY_SIZE;
  byte data[ENTRY_SIZE];
  for (int i = 0; i < ENTRY_SIZE; i++) {
    data[i] = 0xFF; // Șterge prin scrierea valorii 0xFF
  }
  writeEEPROM(address, data, ENTRY_SIZE);
}

int findFreeSlot() {
  int totalEntries = EEPROM_SIZE / ENTRY_SIZE;
  byte data[ENTRY_SIZE];
  for (int i = 0; i < totalEntries; i++) {
    int address = i * ENTRY_SIZE;
    readEEPROM(address, data, ENTRY_SIZE);
    bool isEmpty = true;
    for (int j = 0; j < ENTRY_SIZE; j++) {
      if (data[j] != 0xFF && data[j] != 0) {
        isEmpty = false;
        break;
      }
    }
    if (isEmpty) {
      return address;
    }
  }
  return -1;
}

// Funcție pentru scrierea unui byte în EEPROM
void eepromWriteByte(unsigned int eeaddress, byte data) {
  Wire.beginTransmission(EEPROM_ADDRESS);
  Wire.write((int)(eeaddress >> 8));   // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.write(data);
  Wire.endTransmission();
  delay(5);
}

// Funcție pentru citirea unui byte din EEPROM
byte eepromReadByte(unsigned int eeaddress) {
  byte rdata = 0xFF;
  Wire.beginTransmission(EEPROM_ADDRESS);
  Wire.write((int)(eeaddress >> 8));   // MSB
  Wire.write((int)(eeaddress & 0xFF)); // LSB
  Wire.endTransmission();
  Wire.requestFrom(EEPROM_ADDRESS, 1);
  if (Wire.available()) rdata = Wire.read();
  return rdata;
}

// Funcție pentru scrierea mai multor bytes în EEPROM
void writeEEPROM(unsigned int eeaddress, byte *data, int length) {
  int remaining = length;
  int dataIndex = 0;
  while (remaining > 0) {
    // Determină câți bytes pot fi scriși în pagina curentă
    unsigned int page_offset = eeaddress % 64;
    unsigned int space_in_page = 64 - page_offset;

    int wrlen = min(remaining, space_in_page);
    wrlen = min(wrlen, 30); // Maxim 32 bytes per transmisie, minus 2 bytes pentru adrese

    Wire.beginTransmission(EEPROM_ADDRESS);
    Wire.write((int)(eeaddress >> 8));   // MSB
    Wire.write((int)(eeaddress & 0xFF)); // LSB

    for (int i = 0; i < wrlen; i++) {
      Wire.write(data[dataIndex++]);
    }
    Wire.endTransmission();
    delay(5);
    remaining -= wrlen;
    eeaddress += wrlen;
  }
}

// Funcție pentru citirea mai multor bytes din EEPROM
void readEEPROM(unsigned int eeaddress, byte *data, int length) {
  int remaining = length;
  int dataIndex = 0;
  while (remaining > 0) {
    int rdlen = min(32, remaining); // Maxim 32 bytes per cerere

    Wire.beginTransmission(EEPROM_ADDRESS);
    Wire.write((int)(eeaddress >> 8));   // MSB
    Wire.write((int)(eeaddress & 0xFF)); // LSB
    Wire.endTransmission();

    Wire.requestFrom(EEPROM_ADDRESS, rdlen);

    for (int i = 0; i < rdlen; i++) {
      if (Wire.available()) {
        data[dataIndex++] = Wire.read();
      }
    }
    remaining -= rdlen;
    eeaddress += rdlen;
  }
}
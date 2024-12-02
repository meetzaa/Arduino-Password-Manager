#include <Keypad.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <EEPROM.h>

#define ENC_A 19
#define ENC_B 21
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

const int EEPROM_SIZE = 512;
const int ENTRY_SIZE = 32; // Max domain + password length per entry

void setup() {
  // Set encoder pins and attach interrupts
  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENC_A), read_encoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_B), read_encoder, CHANGE);

  // Initialize OLED display
  u8g2.begin();
  u8g2.clearBuffer();

  // Start the serial monitor
  Serial.begin(115200);
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

  if (encval >= 8) { // Increased sensitivity threshold
    if (currentMenu == 2 || currentMenu == 3) {
      scrollIndex++;
    } else {
      menuIndex++;
    }
    encval = 0;
    Serial.print("Encoder turned right, menuIndex: ");
    Serial.println(menuIndex);
  } else if (encval <= -8) { // Increased sensitivity threshold
    if (currentMenu == 2 || currentMenu == 3) {
      scrollIndex--;
    } else {
      menuIndex--;
    }
    encval = 0;
    Serial.print("Encoder turned left, menuIndex: ");
    Serial.println(menuIndex);
  }

  // Correct the menu index to stay within bounds
  if (menuIndex < 0) {
    menuIndex = 2;
  } else if (menuIndex > 2) {
    menuIndex = 0;
  }

  // Correct scroll index to stay within bounds
  int totalEntries = EEPROM_SIZE / ENTRY_SIZE;
  if (scrollIndex < 0) {
    scrollIndex = totalEntries - 1;
  } else if (scrollIndex >= totalEntries) {
    scrollIndex = 0;
  }
}

void displayMenu() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_4x6_tr); // Smaller font for better spacing

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
    // Go back to the main menu
    if (currentMenu != 0) {
      currentMenu = 0;
      menuIndex = 0;
      addPasswordStep = 0;
      domain = "";
      password = "";
      Serial.println("Back pressed, returning to main menu");
    }
  } else if (key == BTN_SELECT) {
    // Handle select button logic to enter submenu
    if (currentMenu == 0) {
      Serial.print("Selected option: ");
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
        Serial.print("Domain: ");
        Serial.println(domain);
        Serial.print("Password: ");
        Serial.println(password);
        // Save domain and password to EEPROM
        saveToMemory(domain, password);
        domain = "";
        password = "";
        addPasswordStep = 2;
      }
    } else if (currentMenu == 2) {
      // Send password to serial when viewing passwords
      String entry = readFromMemory(scrollIndex);
      Serial.print("Sending to PC: ");
      Serial.println(entry);
    } else if (currentMenu == 3) {
      // Delete selected password
      deleteFromMemory(scrollIndex);
      Serial.print("Deleted password at index: ");
      Serial.println(scrollIndex);
    }
  } else {
    // Handle alphanumeric input for domain and password in add password menu
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
  u8g2.setFont(u8g2_font_4x6_tr); // Smaller font for better spacing
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
  // Logic to display passwords from EEPROM
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_4x6_tr); // Smaller font for better spacing
  u8g2.drawStr(0, 10, "Parole Salvate:");

  for (int i = 0; i < 3; i++) {
    int entryIndex = (scrollIndex + i) % (EEPROM_SIZE / ENTRY_SIZE);
    String entry = readFromMemory(entryIndex);
    if (i == 0) {
      u8g2.drawStr(0, 20 + (i * 10), "> ");
    }
    u8g2.drawStr(10, 20 + (i * 10), entry.c_str());
  }

  u8g2.sendBuffer();
}

void deletePassword() {
  // Logic to delete password from EEPROM
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_4x6_tr); // Smaller font for better spacing
  u8g2.drawStr(0, 10, "Sterge Parola:");

  String entry = readFromMemory(scrollIndex);
  u8g2.drawStr(0, 20, entry.c_str());

  u8g2.sendBuffer();
}

void saveToMemory(String domain, String password) {
  int address = findFreeSlot();
  if (address == -1) {
    Serial.println("No free slot available in EEPROM.");
    return;
  }

  String entry = domain + ":" + password;
  for (int i = 0; i < ENTRY_SIZE; i++) {
    if (i < entry.length()) {
      EEPROM.update(address + i, entry[i]);
    } else {
      EEPROM.write(address + i, 0);
    }
  }

  Serial.print("Saving to memory: ");
  Serial.println(entry);
}

String readFromMemory(int index) {
  int address = index * ENTRY_SIZE;
  String entry = "";
  for (int i = 0; i < ENTRY_SIZE; i++) {
    char c = EEPROM.read(address + i);
    if (c == 0) break;
    entry += c;
  }
  return entry;
}

void deleteFromMemory(int index) {
  int address = index * ENTRY_SIZE;
  for (int i = 0; i < ENTRY_SIZE; i++) {
    EEPROM.write(address + i, 0);
  }
}

int findFreeSlot() {
  for (int i = 0; i < EEPROM_SIZE; i += ENTRY_SIZE) {
    bool isEmpty = true;
    for (int j = 0; j < ENTRY_SIZE; j++) {
      if (EEPROM.read(i + j) != 0) {
        isEmpty = false;
        break;
      }
    }
    if (isEmpty) {
      return i;
    }
  }
  return -1;
}
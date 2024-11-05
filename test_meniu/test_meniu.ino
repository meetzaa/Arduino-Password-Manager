#include <Arduino.h>
#include <U8g2lib.h>
#include <Keypad.h>

#ifdef U8X8HAVE_HW_SPI
#include <SPI.h>
#endif

#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

const byte ROWS = 4; 
const byte COLS = 4; 

char hexaKeys[ROWS][COLS] = {
  {'D', 'C', 'B', 'A'},
  {'#', '9', '6', '3'},
  {'0', '8', '5', '2'},
  {'*', '7', '4', '1'}
};

byte rowPins[ROWS] = {53, 51, 49, 47}; 
byte colPins[COLS] = {45, 43, 41, 39}; 

Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 

U8G2_SSD1306_128X32_UNIVISION_F_SW_I2C u8g2(U8G2_R0, SCL, SDA, U8X8_PIN_NONE);

const char* passwords[] = {
  "parola123",
  "parolalungapentruafisare",
  "securizata!",
};
const int total_passwords = sizeof(passwords) / sizeof(passwords[0]);

uint8_t current_password_index = 0;
float scroll_offset = 0;
#define SCROLL_SPEED 10  // Viteza de derulare

void setup() {
  u8g2.begin();
  u8g2.setFont(u8g2_font_logisoso28_tr);
}

void loop() {
  char customKey = customKeypad.getKey();
  if (customKey) {  // Apăsare buton
    delay(200);  // Anti-debounce
    current_password_index = (current_password_index + 1) % total_passwords;
    scroll_offset = -128;  // Resetăm derularea pentru noua parolă
  }

  const char* current_password = passwords[current_password_index];
  int text_width = u8g2.getStrWidth(current_password);

  u8g2.clearBuffer();

  if (text_width > 128) {
    // Derulăm textul ciclic
    scroll_offset += SCROLL_SPEED;
    if (scroll_offset > text_width) {
      scroll_offset = -128;  // Resetăm pentru a relua derularea din dreapta
    }
    u8g2.drawStr(-scroll_offset, 35, current_password);  // Derulăm parola pe ecran
  } else {
    u8g2.drawStr(0, 35, current_password);  // Afișăm textul fix dacă încape complet
  }

  u8g2.sendBuffer();
}
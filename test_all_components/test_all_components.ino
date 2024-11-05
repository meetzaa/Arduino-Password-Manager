//OLED
#include <Arduino.h>
#include <U8g2lib.h>
#include <SPI.h>
#include <Wire.h>

//Tastaturi matriciale
#include <Keypad.h>

//ENCODER ROTATIV
#define ENC_A 18
#define ENC_B 19

//Tastaturi matriciale
const byte ROWS = 4; 
const byte COLS = 4; 

char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte rowPins1[ROWS] = {53, 51, 49, 47}; 
byte colPins1[COLS] = {45, 43, 41, 39}; 

byte rowPins2[ROWS] = {37, 35, 33, 31}; 
byte colPins2[COLS] = {29, 27, 25, 23}; 

//ENCODER ROTATIV
unsigned long _lastIncReadTime = micros(); 
unsigned long _lastDecReadTime = micros(); 
int _pauseLength = 25000;//25000;
int _fastIncrement = 10;
volatile int counter = 0;
unsigned long before_display;

//OLED
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0); 

//Tastaturi matriciale
Keypad customKeypad1 = Keypad(makeKeymap(hexaKeys), rowPins1, colPins1, ROWS, COLS); 
Keypad customKeypad2 = Keypad(makeKeymap(hexaKeys), rowPins2, colPins2, ROWS, COLS);

void setup() {
  //OLED
  u8g2.begin();
  //ENCODER ROTATIV
  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENC_A), read_encoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_B), read_encoder, CHANGE);

  Serial.begin(115200);
  Serial.println(counter);
}

void loop() {
  static int lastCounter = 0;

  if(counter != lastCounter){
    Serial.println(counter);
    lastCounter = counter;
  }

  u8g2.clearBuffer();					// clear the internal memory
   u8g2.setFont(u8g2_font_logisoso28_tr);  // choose a suitable font at https://github.com/olikraus/u8g2/wiki/fntlistall
   before_display = micros();
   u8g2.drawStr(8,29,"Proiect");	// write something to the internal memory
   u8g2.sendBuffer();					// transfer internal memory to the display
  //Serial.println(micros()-before_display);
  delay(100);

  char customKey1 = customKeypad1.getKey();
  char customKey2 = customKeypad2.getKey();
  
  if (customKey1){
    Serial.println(customKey1);
  }

  if (customKey2){
    Serial.println(customKey2);
  }
}

void read_encoder() {
 
  static uint8_t old_AB = 3;  
  static int8_t encval = 0;   
  static const int8_t enc_states[]  = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};
  old_AB <<=2;  

  if (digitalRead(ENC_A)) old_AB |= 0x02;
  if (digitalRead(ENC_B)) old_AB |= 0x01; 
  
  encval += enc_states[( old_AB & 0x0f )];


  if( encval > 3 ) {     
    int changevalue = 1;
    if((micros() - _lastIncReadTime) < _pauseLength) {
      changevalue = _fastIncrement * changevalue; 
    }
    _lastIncReadTime = micros();
    counter = counter + changevalue;            
    encval = 0;
  }
  else if( encval < -3 ) {        
    int changevalue = -1;
    if((micros() - _lastDecReadTime) < _pauseLength) {
      changevalue = _fastIncrement * changevalue; 
    }
    _lastDecReadTime = micros();
    counter = counter + changevalue;         
    encval = 0;
  }


  
   //delay(100);
/*
   u8g2.clearBuffer();         // clear the internal memory
   u8g2.setFont(u8g2_font_logisoso28_tr);  // choose a suitable font at https://github.com/olikraus/u8g2/wiki/fntlistall
   u8g2.drawStr(31,24,"SIO");  // write something to the internal memory
   u8g2.sendBuffer();         // transfer internal memory to the display
   delay(800);

   u8g2.clearBuffer();         // clear the internal memory
   u8g2.setFont(u8g2_font_logisoso28_tr);  // choose a suitable font at https://github.com/olikraus/u8g2/wiki/fntlistall
   u8g2.drawStr(10,29,"2024");  // write something to the internal memory
   u8g2.sendBuffer();         // transfer internal memory to the display
   delay(800);

   u8g2.clearBuffer();         // clear the internal memory
   u8g2.setFont(u8g2_font_logisoso28_tr);  // choose a suitable font at https://github.com/olikraus/u8g2/wiki/fntlistall
   u8g2.drawStr(4,29,"CALC");  // write something to the internal memory
   u8g2.sendBuffer();         // transfer internal memory to the display
   delay(2000);
  */
}

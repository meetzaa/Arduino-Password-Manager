//OLED
#include <Arduino.h>
#include <U8g2lib.h>
#include <SPI.h>
#include <Wire.h>

//ENCODER ROTATIV
#define ENC_A 18
#define ENC_B 19

unsigned long _lastIncReadTime = micros(); 
unsigned long _lastDecReadTime = micros(); 
int _pauseLength = 25000;//25000;
int _fastIncrement = 10;
volatile int counter = 0;
unsigned long before_display;

//OLED
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0); 

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

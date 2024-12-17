volatile bool buzzer = 0;
#define PIN_BUZZER 5
void setup() {
 setup_timer();
}

void setup_timer()
{
 TCCR3A = 0b00010000; //Semnalul OC1B isi va schimba valoarea de fiecare data cand numaratorul va trece prin valoarea stocata in registrul OCR1B
  TCCR3B = 0b00001000; //Numaratorul 1 va functiona in modul CTC
  TCCR3B |= (1<<1); //Avem prescalator de 1
  OCR3A = 3000;  //Perioada de numarare va fi 0.5ms atunci cand prescalatorul este 1 
  OCR3B = 234; //O sa punem valoarea aleatoare intre 0 si 8000
  pinMode(2,OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  TCCR3B |= (1<<1); //pornim timer ul cu prescalator de 1
  delay(1000);
  TCCR3B = 0b00001000; //oprim timer ul
  //TCCR3B &= ~(1<<1); //oprim timer ul ~00000010 = 11111101
  delay(3000);
}
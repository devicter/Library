#include "Arduino.h"
#include "MaTrix.h"
#include <SPI.h>


byte array[8][8] = { // ������ �� 64 ����
  {B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, B00000000},  // ������ 7
  {B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, B00000000},  // ������ 6
  {B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, B00000000},  // ������ 5
  {B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, B00000000},  // ������ 4
  {B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, B00000000},  // ������ 3
  {B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, B00000000},  // ������ 2
  {B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, B00000000},  // ������ 1
  {B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, B00000000}   // ������ 0
  // red3      green3      red2      green2      red1      green1       red0      green0
};

byte shadow[8][8] = { // ������ �� 64 ����
  {B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, B00000000},  // ������ 7
  {B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, B00000000},  // ������ 6
  {B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, B00000000},  // ������ 5
  {B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, B00000000},  // ������ 4
  {B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, B00000000},  // ������ 3
  {B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, B00000000},  // ������ 2
  {B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, B00000000},  // ������ 1
  {B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, B00000000}   // ������ 0
  // red3      green3      red2      green2      red1      green1       red0      green0
};

boolean longStringFlag = false;
boolean flagBreak = false;

/* �������� ���� ����������������:
���������������  �������
  A0               2
  A1               3
  A2               4
*/

byte row = 0;          // ������ �������, � ������� �������� � ������� ������
byte addrmask = B00000111; // ����� ������ �� ������� ����� �������� (���� 2,3,4 - �� �������� ����� ����������������)

unsigned char *pFont;

/*
//��� SS (7) ��������� � ST_CP ����� 74HC595
//��� SCK (13) ��������� � SH_CP ����� 74HC595
//��� MOSI (11) ��������� � DS ����� 74HC595
*/

MaTrix::MaTrix(){
}

void MaTrix::init(){
  SPI.setClockDivider(SPI_CLOCK_DIV2);
  SPI.begin();                   // �������������� SPI
  //SPI.setBitOrder(MSBFIRST);
  
  // ��� ��� ������������� ������� �������
  //TCCR5B = B00001011;  // ~1��� 
  //TCCR5B = B00000010;  // ~4��� 
  TCCR5B = B00001010;  // ~8��� 
  
  DDRL = DDRL | B11110111;       // �������������� ���� L �� OUTPUT (7, 5, 4,3,2 ����) 
  //Serial.begin(9600);
  /*
  pinMode(42, OUTPUT);  // SS ��������� ������
  pinMode(43, OUTPUT);  // ������� (E1, E2)
  pinMode(44, OUTPUT);  // ������� PWM (E3)
  pinMode(47, OUTPUT);  // ����� A2
  pinMode(48, OUTPUT);  // ����� A1
  pinMode(49, OUTPUT);  // ����� A0
  */

  noInterrupts();           // disable all interrupts
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;

  OCR1A = 2000;              // compare match register 16MHz/8/800Hz
                             // 100�� - ��� ����� �������, �� ���������� �� ��� 1 �� 8 �����
  TCCR1B |= (1 << WGM12);    // CTC mode
  TCCR1B |= (1 << CS11);     // 8 prescaler 
  TIMSK1 |= (1 << OCIE1A);   // enable timer compare interrupt
  interrupts();              // enable all interrupts
}

void MaTrix::brightness(byte brightLevel) {
	matrixBrightness=brightLevel;
	analogWrite(BRIGHT, matrixBrightness);
}

int MaTrix::getBrightness() {
	return matrixBrightness;	
}

void MaTrix::abort() {
	flagBreak=true;
}

// �������� ������ sym � ������� pos (�� 0 �� 5, ��������� ������-������)
void MaTrix::printChar(unsigned char sym, byte pos, byte color) {
  if ((sym>=pgm_read_byte(pFont+2))&&(sym<=(pgm_read_byte(pFont+2)+pgm_read_byte(pFont+3)))&&(sym>=pgm_read_byte(pFont+2))&&(pos>=0)&&(pos<=(pgm_read_byte(pFont+4)-1))) { // �������� �� ��, ��� ��������� � ������ ������
    // posMr - ����� ����� ��� "�������" ���������� �������, �� ������� ������������ �����������
    // posMl - ����� ����� ��� "�������" ���������� �������, �� ������� ������������ �����������
    // fontM - ������� ���� � ������ ��� ������� �������
    // posP - �������� ��� ��������� ������ ������� �������, ����������� � �����������
    int posMr, posMl, fontM, posP, posPl, posPf;
    // ������� ����� (������, ������ � ����� �����)
    byte mask, maskSr, maskSl, width, offset;
        
    posP=pos+11;  // ������ ������ � �������� �������
    posPl=posP+6; // ������ ������ � ������� �������
    posPf=pos+5;  // ������ ������ � ��������� ������� ������������ ������ ���.�������
    
    mask=0;
    width=pgm_read_byte(pFont);
    // ����������� ����� ��� ������
    for (byte m=0; m<width; m++){
      mask=mask+(1<<m);
    }

    // ����� ��� ����� � ������ ����� �������
    offset=pgm_read_byte(pFont+posPf);
    maskSr=mask<<offset;
    maskSl=mask>>(8-offset);
    
    // ������ ���� ��� ���������� ������ (� ������ �����)
    posMr=6-pgm_read_byte(pFont+posP)*2+color;
    posMl=4-pgm_read_byte(pFont+posP)*2+color;
        
    for (byte i=0; i<8; i++) {
      fontM=(sym-pgm_read_byte(pFont+2))*8+11+12+i;
      if(color<3 && color!=YELLOW) {
        // ������������ "�������" �������
        array[i][posMr]=(array[i][posMr] & ~maskSr) | (pgm_read_byte(pFont+fontM)<<pgm_read_byte(pFont+posPf));
        
        // ���� �����, ������������� "�������" �������
        if(pgm_read_byte(pFont+posPl) != 5) { 
          array[i][posMl]=(array[i][posMl] & ~maskSl) | (pgm_read_byte(pFont+fontM)>>(8-pgm_read_byte(pFont+posPf)));
        }
      }
      else {
        printChar(sym, pos, GREEN);
        printChar(sym, pos, RED);
      }
    }
  }
}

void MaTrix::printCharShadow(unsigned char sym, byte pos, byte color) {
  if ((sym>=pgm_read_byte(pFont+2))&&(sym<=(pgm_read_byte(pFont+2)+pgm_read_byte(pFont+3)))&&(sym>=pgm_read_byte(pFont+2))&&(pos>=0)&&(pos<=(pgm_read_byte(pFont+4)-1))) { // �������� �� ��, ��� ��������� � ������ ������
    // posMr - ����� ����� ��� "�������" ���������� �������, �� ������� ������������ �����������
    // posMl - ����� ����� ��� "�������" ���������� �������, �� ������� ������������ �����������
    // fontM - ������� ���� � ������ ��� ������� �������
    // posP - �������� ��� ��������� ������ ������� �������, ����������� � �����������
    int posMr, posMl, fontM, posP, posPl, posPf;
    // ������� ����� (������, ������ � ����� �����)
    byte mask, maskSr, maskSl, width, offset;
        
    posP=pos+11;  // ������ ������ � �������� �������
    posPl=posP+6; // ������ ������ � ������� �������
    posPf=pos+5;  // ������ ������ � ��������� ������� ������������ ������ ���.�������
    
    mask=0;
    width=pgm_read_byte(pFont);
    // ����������� ����� ��� ������
    for (byte m=0; m<width; m++){
      mask=mask+(1<<m);
    }

    // ����� ��� ����� � ������ ����� �������
    offset=pgm_read_byte(pFont+posPf);
    maskSr=mask<<offset;
    maskSl=mask>>(8-offset);
    
    // ������ ���� ��� ���������� ������ (� ������ �����)
    posMr=6-pgm_read_byte(pFont+posP)*2+color;
    posMl=4-pgm_read_byte(pFont+posP)*2+color;
        
    for (byte i=0; i<8; i++) {
      fontM=(sym-pgm_read_byte(pFont+2))*8+11+12+i;
      if(color<3 && color!=YELLOW) {
        // ������������ "�������" �������
        shadow[i][posMr]=(shadow[i][posMr] & ~maskSr) | (pgm_read_byte(pFont+fontM)<<pgm_read_byte(pFont+posPf));
        
        // ���� �����, ������������� "�������" �������
        if(pgm_read_byte(pFont+posPl) != 5) { 
          shadow[i][posMl]=(shadow[i][posMl] & ~maskSl) | (pgm_read_byte(pFont+fontM)>>(8-pgm_read_byte(pFont+posPf)));
        }
      }
      else {
        printCharShadow(sym, pos, GREEN);
        printCharShadow(sym, pos, RED);
      }
    }
  }
}

// ������� ������
void MaTrix::printArray() {
	Serial.println("Array");
  for(byte color=RED; color<YELLOW; color++){
    Serial.println((color)?"GREEN":"RED");
    for(int i=0; i<8; i++){
      for(int j=color; j<8; j=j+2) {
        for(int k=7; k>=0; k--){
          //Serial.print(bitRead(array[i][j], k), DEC);
          Serial.print((bitRead(array[i][j], k))?char(174):char(32));
        }
        //Serial.print(" ");
      }
      Serial.println();
    }
    Serial.println();
  }
}

// ������� ������� ������
void MaTrix::printShadow() {
	Serial.println("Shadow");
  for(byte color=RED; color<YELLOW; color++){
    Serial.println((color)?"GREEN":"RED");
    for(int i=0; i<8; i++){
      for(int j=color; j<8; j=j+2) {
        for(int k=7; k>=0; k--){
          //Serial.print(bitRead(array[i][j], k), DEC);
          Serial.print((bitRead(shadow[i][j], k))?char(174):char(32));
        }
        //Serial.print(" ");
      }
      Serial.println();
    }
    Serial.println();
  }
}

void MaTrix::clearLed() {
  for(int i=0; i<8; i++) for(int j=0; j<8; j++) array[i][j]=0;
}

void MaTrix::clearShadow() {
  for(int i=0; i<8; i++) for(int j=0; j<8; j++) shadow[i][j]=0;
}

//void display() 
// ����������� �������
ISR(TIMER1_COMPA_vect)          // timer compare interrupt service routine
{
  if (row++ == 8) row = 0;
  PORTL&=~(1<<7); // ���������� 7 ��� � 0
  PORTL|=(1<<6); // ���������� 6 ��� � 1 (�������� ������)
  // �������� ��������������� ������
  // ���� ������� "�����" (0,0 - ������ �����) SPI.setBitOrder(MSBFIRST);
  for(char i=0; i < 8; i++) SPI.transfer(~getByte(row,i));
  // ���� ������� "�����" (0,0 - ����� �����) SPI.setBitOrder(LSBFIRST);
  //for(char i=0; i < 8; i++) SPI.transfer(~array[7-row][i]);
  PORTL = (PORTL & ~addrmask) | (row) | (1<<7);
  // ������� ������������ ��� �� 5 ���� (�������� �� ������� ��������� ����������)
  PORTL&=~(1<<6); // ���������� 6 ��� � 1 (������ ������)
}


byte getByte(byte row, byte col) {
  byte mask=1;
  if (row>0) mask<<=row;
  return 
    (array[0][col]&mask?B10000000:0)|
    (array[1][col]&mask?B1000000:0)|
    (array[2][col]&mask?B100000:0)|
    (array[3][col]&mask?B10000:0)|
    (array[4][col]&mask?B1000:0)|
    (array[5][col]&mask?B100:0)|
    (array[6][col]&mask?B10:0)|
    (array[7][col]&mask?1:0);
}

/*
byte getByte(byte row, byte col){
  byte mask=1;
  if (row>0) mask<<=row;
  return 
    (array[7][col]&mask?B10000000:0)|
    (array[6][col]&mask?B1000000:0)|
    (array[5][col]&mask?B100000:0)|
    (array[4][col]&mask?B10000:0)|
    (array[3][col]&mask?B1000:0)|
    (array[2][col]&mask?B100:0)|
    (array[1][col]&mask?B10:0)|
    (array[0][col]&mask?1:0);
}
*/
void MaTrix::setFont(unsigned char *Font){
  pFont = Font;
}




// �������� ������
void MaTrix::printString(String s, byte pos, byte color, unsigned char *Font, char effect, int speed) {
  pFont = Font;
  int i, j, k;
  unsigned long ready;
  if (effect==0) {	// ��� �������
	unsigned char buf[s.length()+1];
	for (unsigned char i=0; i<s.length(); i++) {
		buf[i]=s[i];
	}
	buf[s.length()]='\0';  
	printStr(buf, pos, color);
  } 
  else {
  clearShadow();
  printStringShadow(s, pos, color);
  if (effect==1) {	// ������� �����
	  for (i=0; i<8; i++){
		// �������� ������ array �� 1 ������ �����
		for(j=0; j<8; j++){
		  for(k=0; k<8; k++) {
			array[k][j]=array[k+1][j];
		  }
		  // ������� �� ������� shadow ��������� ������ � ��������� �� � ������
		  array[7][j]=shadow[i][j];
		}
		ready=millis()+speed;
		while(millis()<ready) code();
	  }
  }
  else if (effect==2) {	// ������� ���� 
	  for (i=0; i<8; i++){
		// �������� ������ array �� 1 ������ ����
		for(j=0; j<8; j++){
		  for(k=7; k>=0; k--) {
			array[k][j]=array[k-1][j];
		  }
		  // ������� �� ������� shadow ��������� ������ � ��������� �� � �������
		  array[0][j]=shadow[7-i][j];
		}
		ready=millis()+speed;
		while(millis()<ready) code();
	  }
	}
	else if (effect==3) {	// ����� ����� 
	
	byte empty;
	
	if(longStringFlag == true){
	//int nChar=int(32/pgm_read_byte(pFont));
		empty=32-pgm_read_byte(pFont)*(int)(32/pgm_read_byte(pFont));
		//Serial.println(empty);
		for(k=0; k<empty; k++) {
			for(j=0; j<8; j++) {
				for(i=0; i<8; i++){
					shadow[i][j]=shadow[i][j]<<1;
					if(j<6){
						bitWrite(shadow[i][j],0,bitRead(shadow[i][j+2],7));
					}
				}
			}
		}
	}
	
	//printArray();
	//printShadow();
		for(k=0; k<(32-empty*longStringFlag); k++){	// �������� 32 ���� - ����� ����� ������� ������ ������� ����� ����� �� �����
			for(j=0; j<8; j++){		// ���������� ������� (�������� � ������ ������)
				for(i=0; i<8; i++){	// ������ (� �������)
					array[i][j]=array[i][j]<<1;	// �������� ����� ��������� ������� �� 1 (������� ��� - ��������, ������� - 0)
					
					// ��������� ������� ��� �������� �����
					if(j<6){	// ���� ��������� ���� � ������ ��������� �������
						// ����� ������� ��� �� ����������� ����� ������� �����
						bitWrite(array[i][j],0,bitRead(array[i][j+2],7));
					}
					else {		// ���� � ������� ������� ��� ������� ����� (�� �������� �������)
								// ����� �� ������ �������� ������� (����������� ������� ������)
						if(j==6){	// "�������" ����
							bitWrite(array[i][6],0,bitRead(shadow[i][0],7));
						}
						else {		// "�������" ����
							bitWrite(array[i][7],0,bitRead(shadow[i][1],7));
						}
					}
					
					// ������� ������ ���� ��������
					if(k>0){
						shadow[i][j]=shadow[i][j]<<1;
						if(j<6){
							bitWrite(shadow[i][j],0,bitRead(shadow[i][j+2],7));
						}
					}
				}
			}
			
			ready=millis()+speed;
			while(millis()<ready) code();
		}
	//Serial.println("----------------------------------------");
	//printArray();
	//printShadow();
	}
	else if (effect==4) {	// Fade out - fade in 
		byte curBr=getBrightness();
		byte step=curBr/10;
		for (i=-10; i<10; i++){
			// 10 ����� �� ������� � 10 �� ���������
			// ���� ������� - 0, ��������� ������
			if(i==0){
				for(j=0; j<8; j++){
					for(k=0; k<8; k++) {
						array[j][k]=shadow[j][k];
					}
				}
			}
			ready=millis()+speed;
			while(millis()<ready) code();
			if(i<0){
				curBr=curBr-step;
			}
			else {
				curBr=curBr+step;
			}
			brightness(curBr);
		  }
	}
  }
}


void MaTrix::printRunningString(String s, byte color, unsigned char *Font, int speed) {
//  Serial.println("printRunningString");
	pFont = Font;
	longStringFlag = true;
	//clearLed();
	clearShadow();
	int i = 0;
	int nChar=int(32/pgm_read_byte(pFont));
//  Serial.print("nChar=");
//  Serial.println(nChar);
	String sp;
	int iterations = int((s.length())/nChar);
//  Serial.print("iterations=");
//  Serial.println(iterations);
  
	for(i=0; i<=iterations; i++) {
//    Serial.println(i);
		sp=s.substring(i*nChar, nChar+i*nChar);
		printString(sp, nChar-1, color, Font, 3, speed);

//    Serial.println(sp);

		if(flagBreak) {
			flagBreak = false;
			clearLed();
			clearShadow();
			return;
		}
	}
	longStringFlag = false;
	// ����� �� ���������� "������"
	printString(" ", nChar-1, color, Font, 3, speed);	
}
		
  
// �������� ������ ��������
void MaTrix::printStr(unsigned char *s, byte pos, byte color) {
  byte p = pos;
  while (*s) 
    {
      printChar(*s, p, color);
      s++;
      p--;
    }
}
  // �������� ������
void MaTrix::printStringShadow(String s, byte pos, byte color) {
  unsigned char buf[s.length()+1];
  for (unsigned int i=0; i<s.length(); i++) {
    buf[i]=s[i];
  }
  buf[s.length()]='\0';  
  printStrShadow(buf, pos, color);
}
  
// �������� ������ ��������
void MaTrix::printStrShadow(unsigned char *s, byte pos, byte color) {
  byte p = pos;
  while (*s) 
    {
      printCharShadow(*s, p, color);
      s++;
      p--;
    }
}
  
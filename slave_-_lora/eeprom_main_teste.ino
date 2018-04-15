/*
   EEPROMCpp.cpp

   Created: 20/10/2017 16:23:50
   Author : Boludo
*/

#include <inttypes.h>
#include <Wire.h>
#include <string.h>
#include "eeprom.h"

void setup() {
  delay(3000);
  Wire.begin();
  Serial.begin(9600);
  while (!Serial) {
    ;
  }
  Serial.println("OK0");
  eeprom.switchto(0);
  eeprom.resetlogs();
  //Serial.println("OK1");
  Serial.println(eeprom.readbyte(0));
  Serial.println(eeprom.readbyte(1));
  Serial.println(eeprom.readbyte(2));
  //Serial.println(char(eeprom.readbyte(3)));
  eeprom.init(0);
  //Serial.println("OK2");
  //delay(500);
  char otherstring[240] = "123212317s8s9s0s";
  //char otherstring[240] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyx1234567890!@#$%¨&*()hahaha nao sei mais o que falar mas ainda estou digitando apenas para testar essa eeprom mas que saco enfim to so enchendo linguica ... aff ... bip bop puf til";
  //char otherstring2[240] = "Essa outra string aqui tem um monte de letra so pra tentar sobreescrever aquela string que tava aqui antes de tudo isso acontecer mas que acabou acontecendo infelizmente e eu ja nem sei mais o que eu to digitando nesse trecho de texto";
  char otherstring2[240];
  uint16_t ptr;
  char str[240];
  uint16_t aux = 0;
  //delay(500);
  //Serial.println("OK3");
  eeprom.zipstring(otherstring, otherstring2);
  eeprom.writestring(otherstring2, 1);
  //delay(30);
  //eeprom.writestring(otherstring);
  //Serial.println("OK4");
  //delay(5);
  eeprom.updatelogs();
  Serial.println(eeprom.readbyte(0));
  Serial.println(eeprom.readbyte(1));
  Serial.println(eeprom.readbyte(2));
  ptr = eeprom.readstring(3, otherstring2, 1);
  eeprom.unzipstring(otherstring2, str);
  //Serial.println("OK5");
  while (aux != 240) {
    //Serial.print(char(eeprom.readbyte(aux)));
    if (str[aux] == '\0') {
      break;
    }
    Serial.print(char(str[aux]));
    if ((aux + 1) % 32 == 0) {
      Serial.println(' ');
    }
    aux++;
  }
  //delay(50);
  strcpy(otherstring, "e1e2e3e4e5e6e7e8e9e101112");
  eeprom.writestring(otherstring, 0);
  //delay(50);
  ptr = eeprom.readstring(ptr, str, 0);
  aux = 0;
  while (aux != 240) {
    //Serial.print(char(eeprom.readbyte(aux)));
    if (str[aux] == '\0') {
      break;
    }
    Serial.print((str[aux]));
    if ((aux + 1) % 32 == 0) {
      Serial.println(' ');
    }
    aux++;
  }
  Serial.println(' ');
  Serial.println("----");
  Serial.println(' ');
  eeprom.updatelogs();
  aux = 0;
  while (aux != 80) {
    Serial.print(char(eeprom.readbyte(aux)));
    /*if (str[aux] == '\0') {
      break;
    }
    Serial.print((str[aux]));*/
    if ((aux + 1) % 32 == 0) {
      Serial.println(' ');
    }
    aux++;
  }
  while (1) {
    delay(5000);
  }
}


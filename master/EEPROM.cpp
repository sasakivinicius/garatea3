/*
 * EEPROMCpp.cpp
 *
 * Created: 20/10/2017 16:23:50
 * Author : Boludo
 */ 
 #define F_CPU 8000000

#include <avr/io.h>
#include <inttypes.h>
#include "C:\Program Files (x86)\Arduino\hardware\arduino\avr\libraries\Wire\src\Wire.h"
#include <time.h>
#include <util/delay.h>

class eeprom{
public:
	/* error id
	d7 - desligamento inesperado do sistema 
	d6 - letra nao premitida durante compressão
	d5 - 
	d4 - 
	d3 - 
	d2 - tentativa de ler em uma posição inacessivel (sem permissao ou maior que a memoria)
	d1 - tentativa de escrever em uma posição inacessivel (sem permissao ou maior que a memoria)
	d0 - tentativa de acessar uma posição inacessivel (sem permissao ou maior que a memoria)
	*/
	/* bytes reservados
	0x00 mempos[1]
	0x01 mempos[0]
	0x02 err
	*/
	//ziptable										//tabela de compactação (pode colocar char)
	#define to0x0A 0xFF
	#define to0x0B 0xFF
	#define to0x0C 0xFF
	#define to0x0D 0xFF
	#define to0x0E 0xFF
	#define to0x0F 0x0A // \n
	//agr vem o resto
	#define operationdelay 5						//tempo minimo de espera entre operações
	#define reserved 3								//numero de bytes reservados no começo da memoria
	#define pagesize 64 							//tamanho maximo de uma page
	#define memorysize 32768						//tamanho da memoria em bytes
	uint16_t memcount;								//variavel auxiliar para esctira
	uint8_t err;									//log de erros
	clock_t lastcall;
	void delayquepresta(uint16_t dms){				//em decimo de ms. _delay_ms requer variavel definida no tempo de compilação
		while(dms!=0){
			_delay_us(100);
			dms--;
		}
	}
	void mindelay(uint32_t delay, clock_t lasttime){//delay relativo
		if(((1000*(clock()-lasttime)/CLOCKS_PER_SEC) < delay) && (clock()>lasttime)){
			delayquepresta(delay - ((10000*(clock()-lasttime))/CLOCKS_PER_SEC));
		}
	}
	typedef union{									//union para auxiliar na quebra do endereço principal da memoria
		uint16_t full;
		uint8_t half[2];
	}memdiv;
	memdiv mempos;									//aponta para a posição principal na eeprom
	memdiv temppos;									//aponta para uma posição temporaria na eeprom
	void gotobyte(uint16_t x){						//move o ponto em que estão sendo salvos os dados na eeprom
		if((x < memorysize) && (x > reserved)){
			mempos.full= x;
		}
		else{
			err=err | 0x01;
		}
	}
	void writebyte(uint16_t pos, uint8_t data){		//escreve um unico byte na posiçao especificada, sem restrições
		if(pos < memorysize){
			mindelay(operationdelay, lastcall);
			temppos.full=pos;
			Wire.beginTransmission(0b10100000);
			Wire.write(temppos.half[1]);
			Wire.write(temppos.half[0]);
			Wire.write(data);
			Wire.endTransmission();
			lastcall= clock();
		}
		else{
			err=err | 0x02;
		}
	}
	uint8_t readbyte(uint16_t pos){					//le um unico byte na posição especificada, sem restrições
		if(pos < memorysize){
			Wire.beginTransmission(0b10100000);
			Wire.write(temppos.half[1]);
			Wire.write(temppos.half[0]);
			Wire.endTransmission(0);
			Wire.requestFrom(0b10100001,1);
			return Wire.read();
		}
		else{
			err=err | 0x04;
		}
		return 0;
	}
	void init(void){								//inicializa a memoria (favor usar apenas uma vez), e checa se houve um desligamento inesperado
		mempos.half[1]=readbyte(0x00);
		mempos.half[0]=readbyte(0x01);
		err=readbyte(0x02);
		if(mempos.full==0){
			mempos.full= reserved;
			err= 0;
			memcount= 0;
		}
		else{
			err=err | 0x80;
		}
	}
	void writestring(char *data){					//escreve uma string na eeprom
		uint16_t offset=0;
		uint8_t temcoisa=1;
		while(temcoisa){
			memcount=0;
			mindelay(operationdelay, lastcall);
			Wire.beginTransmission(0b10100000);
			Wire.write(mempos.half[1]);
			Wire.write(mempos.half[0]);
			while(temcoisa && (memcount != 64)){
				Wire.write(data[memcount+offset]);
				if(data[memcount+offset]='\0'){
					temcoisa=0;
				}
				memcount++;
			}
			Wire.endTransmission();
			lastcall= clock();
			offset+=memcount;
			mempos.full+=memcount;
		}
	}
	void readpage(char *buffer,uint16_t pos){					//escreve uma string na eeprom
		uint16_t offset=0;
		uint8_t temcoisa=0;
		memcount = 0;
		temppos.full=pos;
		while(buffer[memcount] != '\0'){
			Wire.beginTransmission(0b10100000);
			Wire.write(temppos.half[1]);
			Wire.write(temppos.half[0]);
			Wire.endTransmission(0);
			Wire.requestFrom(0b10100001,64);
			while(temcoisa && (memcount != 64)){
				buffer[memcount+offset]=Wire.read();
				if(buffer[memcount+offset]='\0'){
					temcoisa=0;
				}
				memcount++;
			}
			offset+=memcount;
			mempos.full+= memcount;
		}
	}
	void updatelogs(void){										//atualiza os dados de erro e posição de memoria na eeprom
		writebyte(0x00,mempos.half[1]);
		writebyte(0x01,mempos.half[0]);
		writebyte(0x02,err);
	}
	void resetlogs(){											//reseta os dados de erro e posição salva na memoria da eeprom
		writebyte(0x00,0x00);
		writebyte(0x01,0x00);
		writebyte(0x02,0x00);
	}
	uint16_t zipstring(char *inp, char *out){
		uint16_t pos=0;
		uint16_t outpos=0;
		uint8_t esquerda;
		uint8_t temcoisa=1;
		while(temcoisa){
			if(pos & 0x01){
				esquerda = 4;
				out[outpos]=0;
			}
			else{
				esquerda=0;
			}
			if((0x30<=inp[pos])&&(inp[pos]<=0x39)){
				out[outpos]+=(inp[pos] - 0x29)<<esquerda;
			}
			else if(inp[pos]==to0x0B){
				out[outpos]+=(0x0B)<<esquerda;
			}
			else if(inp[pos]==to0x0C){
				out[outpos]+=(0x0C)<<esquerda;
			}
			else if(inp[pos]==to0x0D){
				out[outpos]+=(0x0D)<<esquerda;
			}
			else if(inp[pos]==to0x0E){
				out[outpos]+=(0x0E)<<esquerda;
			}
			else if(inp[pos]==to0x0F){
				out[outpos]+=(0x0F)<<esquerda;
			}
			else if(inp[pos]=='\0'){
				if(esquerda){
					out[outpos]='\0';
				}
			}
			else{
				err=0x40;
			}
			pos++;
			if(!esquerda){
				outpos++;
			}
		}
		return outpos;
	}
	void unzipstring(char *inp, char *out){
		
	}
}eeprom;


int main(void){
	eeprom.init();
    while(1){	
	}
	return 0;
}


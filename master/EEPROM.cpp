/*
 * EEPROMCpp.cpp
 *
 * Created: 20/10/2017 16:23:50
 * Author : Boludo
 */
//descomenta se der ruim, deve ter a ver com o CLOCKS_PER_SEC
//#define F_CPU 8000000

#include <avr/io.h>
#include <inttypes.h>
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
	/* bytes reservados no inicio da eeprom, protegidos de overwriting.
	0x00 mempos[1]
	0x01 mempos[0]
	0x02 err
	*/
	//ziptable					//tabela de compactação (pode colocar char)
	#define to0x0A 'n'
	#define to0x0B 'e'
	#define to0x0C 's'
	#define to0x0D 'w'
	#define to0x0E 0xFF	//livre
	#define to0x0F 0xFF	//livre
	//agr vem o resto
	#define operationdelay 50			//tempo minimo de espera entre operações
	#define reserved 3				    //numero de bytes reservados no começo da memoria
	#define pagesize 64 				//tamanho maximo de uma page
	#define memorysize 32768			//tamanho da memoria em bytes
	uint16_t memcount;				    //variavel auxiliar para esctira
	uint8_t err;					    //log de erros
	clock_t lastcall;			       	//guarda a ultima vez que uma operação da eeprom foi chamada
	void delayquepresta(uint16_t dms){	//em decimo de ms. _delay_ms requer variavel definida no tempo de compilação
		while(dms!=0){
			_delay_us(100);
			dms--;
		}
	}
	void mindelay(uint32_t delay, clock_t lasttime){//delay relativo em decimo de ms
		if((abs(10000*(clock()-lasttime)/CLOCKS_PER_SEC) < delay)){
			delayquepresta(delay - abs((10000*(clock()-lasttime))/CLOCKS_PER_SEC));
		}
	}
	typedef union{						//union para auxiliar na quebra do endereço principal da memoria
		uint16_t full;					//usa o full pra salvar o dado inteiro
		uint8_t half[2];				//dps le as metades :)
	}memdiv;
	memdiv mempos;						//aponta para a posição principal na eeprom
	memdiv temppos;						//aponta para uma posição temporaria na eeprom
	void gotobyte(uint16_t x){                      //move o ponto em que estão sendo salvos os dados na eeprom
		if((x < memorysize) && (x > reserved)){
			mempos.full= x;			               	//atualiza a posição do ponteiro
		}
		else{
			err=err | 0x01;				//seta o bit de erro d1
		}
	}
	void writebyte(uint8_t data, uint16_t pos){		//escreve um unico byte na posiçao especificada, sem restrições
		if(pos < memorysize){			          	//verifica se a posição ta dentro da memoria
			mindelay(operationdelay, lastcall);	    //aguarda o tempo necessario pra dar o define "operationdelay", caso necessario 
			temppos.full = pos;		            	//atualiza a posição temporaria de escrita para o byte
			Wire.beginTransmission(0b10100000);
			Wire.write(temppos.half[1]);
			Wire.write(temppos.half[0]);
			Wire.write(data);
			Wire.endTransmission();
			lastcall = clock();	                    //atualiza o tempo da ultima chamada de escrita ou leitura na eeprom
		}
		else{
			err=err | 0x02;				//seta o bit de erro d2
		}
	}
	uint8_t readbyte(uint16_t pos){				    //le um unico byte na posição especificada, sem restrições
		if(pos < memorysize){		               	//verifica se a posição ta dentro da memoria
            mindelay(operationdelay, lastcall); 	//aguarda o tempo necessario pra dar o define "operationdelay", caso necessario
            temppos.full = pos;		               	//atualiza a posição temporaria de escrita para o byte
			Wire.beginTransmission(0b10100000);  	//daqui pra baixo n sei mais o q ta acontecendo
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
	void init(void){				       	//inicializa a memoria (favor usar apenas uma vez), e checa se houve um desligamento inesperado
		mempos.half[1] = readbyte(0x00);	//le a ultima posição salva do ponteiro de escrita da eeprom
		mempos.half[0] = readbyte(0x01);	//le a ultima posição salva do ponteiro de escrita da eeprom
		err = readbyte(0x02);				//carrega o log de erros da eeprom pra ram
		if(mempos.full == 0){				//se a ultima posição de escrita for 0 o log de erros é limpo, 
			mempos.full = reserved;			//joga o ponteiro de escrita para 1 byte dps da quantidade reservada (LEMBRANDO QUE A INDEXAÇÂO COMEÇA DO 0)
			err = 0;				        //limpa o log de erros
			memcount = 0;			     	//limpa a quantidade de coisas escritas (var auxiliar)
		}
		else{
			err=err | 0x80;		    		//seta o bit de erro d7 (desligamento inesperado)
		}
	}
	void writestring(char *data){              			//escreve uma string na eeprom
		uint16_t offset=0;			                   	//variavel pra caso venha mais q uma pagina (64bytes)
		uint8_t temcoisa=1;		                		//tem coisa
		while(temcoisa){	     	               		//escrevendo a string
			memcount=0;		                      		//zera o numero de BYTES escritos
			mindelay(operationdelay, lastcall);	        //garante o delay minimo caso necessario
			Wire.beginTransmission(0b10100000);      	//chama a biblioteca wire
			Wire.write(mempos.half[1]);
			Wire.write(mempos.half[0]);
			while(temcoisa && (memcount != 64)){		//escrevendo a pagina
				Wire.write(data[memcount+offset]);	    //escreve na eeprom o dado + offset
				if(data[memcount+offset]='\0'){ 		//se acabar a string para de escrever
					temcoisa=0;		                  	//acabou a coisa
				}
				memcount++;	                			//incrementa o numero de coisas escritas
			}
			Wire.endTransmission();
			lastcall = clock();	                 		//atualiza a ultima chamada de escrita | leitura
			offset += memcount;	                 		//atualiza o offset da leitura da string
			mempos.full += memcount;                	//atualiza o ponteiro de escrita
		}
	}
	void readpage(uint16_t pos, char *buffer){		//escreve uma string na eeprom
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
	void updatelogs(void){					//atualiza os dados de erro e posição de memoria na eeprom
		writebyte(0x00,mempos.half[1]);
		writebyte(0x01,mempos.half[0]);
		writebyte(0x02,err);
	}
	void resetlogs(void){					//reseta os dados de erro e posição salva na memoria da eeprom
		writebyte(0x00,0x00);
		writebyte(0x01,0x00);
		writebyte(0x02,0x00);
	}
	uint16_t zipstring(char *inp, char *out) {
		uint16_t pos = 0;
		uint16_t outpos = 0;
		uint8_t esquerda;
		uint8_t temcoisa = 1;
		while (temcoisa) {
			if (!(pos & 0x01)) {
				esquerda = 4;
				out[outpos] = 0;
			}
			else {
				esquerda = 0;
			}
			if ((0x30 <= inp[pos]) && (inp[pos] <= 0x39)) {
				out[outpos] += (inp[pos] - 0x2F) << esquerda;
			}
			else if (inp[pos] == to0x0B) {
				out[outpos] += 0x0B << esquerda;
			}
			else if (inp[pos] == to0x0C) {
				out[outpos] += 0x0C << esquerda;
			}
			else if (inp[pos] == to0x0D) {
				out[outpos] += 0x0D << esquerda;
			}
			else if (inp[pos] == to0x0E) {
				out[outpos] += 0x0E << esquerda;
			}
			else if (inp[pos] == to0x0F) {
				out[outpos] += 0x0F << esquerda;
			}
			else if (inp[pos] == '\0') {
				if (esquerda) {
					out[outpos] = '\0';
				}
				temcoisa = 0;
			}
			else {
				err = 0x40;
			}
			pos++;
			if (!esquerda) {
				outpos++;
			}
		}
		return outpos;
	}
	uint16_t unzipstring(char *inp, char *out) {
		uint16_t pos = 0;
		uint16_t outpos = 0;
		uint8_t temcoisa = 1;
		while ((inp[pos] != '\0') &&  (temcoisa)) {
			out[outpos] = (inp[pos] & 0xF0) >> 4;
			outpos++;
			if ((inp[pos] & 0x0F) != '\0') {
				out[outpos] = inp[pos] & 0x0F;
				outpos++;
			}
			else {
				temcoisa = 0;
			}
			pos++;
		}
		out[outpos] = '\0';
		outpos = 0;
		temcoisa = 1;
		while (temcoisa) {
			if ((0x01 <= out[outpos]) && (out[outpos] <= 0x0A)) {
				out[outpos] = (out[outpos] + 0x2F);
			}
			else if (out[outpos] == 0x0B) {
				out[outpos] = to0x0B;
			}
			else if (out[outpos] == 0x0C) {
				out[outpos] = to0x0C;
			}
			else if (out[outpos] == 0x0D) {
				out[outpos] = to0x0D;
			}
			else if (out[outpos] == 0x0E) {
				out[outpos] = to0x0E;
			}
			else if (out[outpos] == 0x0F) {
				out[outpos] = to0x0F;
			}
			else if (out[outpos] == '\0') {
				temcoisa = 0;
			}
			else {
				err = 0x40;
			}
			outpos++;
		}
		out[outpos] = '\0';
		return outpos;
	}
}eeprom;

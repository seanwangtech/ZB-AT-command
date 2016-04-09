#include"iocc2530.h"
#include"OnBoard.h"
#include "AT_ds18b20.h" 

#define Ds18b20IO P0_7       //define the temperature sensor IO



void Ds18b20Delay(unsigned int k);
void Ds18b20InputInitial(void);
void Ds18b20OutputInitial(void);
unsigned char Ds18b20Initial(void);
void Ds18b20Write(unsigned char infor);
unsigned char Ds18b20Read(void);


//Clock is 32M, should be adjusted according to different MCU
void Ds18b20Delay(unsigned int k)
{
  while (k--)
  {
    asm("NOP");
    asm("NOP");
    asm("NOP");
    asm("NOP");
    asm("NOP");
    asm("NOP");
    asm("NOP");
    asm("NOP"); 
    asm("NOP");  
  }
}

void Ds18b20InputInitial(void) //set the port to input
{
    P0DIR &= 0x7f;
}

void Ds18b20OutputInitial(void)//set the port to output
{
    P0DIR |= 0x80;
}

//initilize ds18b20  return: succeed 0x00£¬fail 0x01
unsigned char Ds18b20Initial(void)
{
    unsigned char Status = 0x00;
    unsigned int CONT_1 = 0;
    unsigned char Flag_1 = 1;
    Ds18b20OutputInitial();
    Ds18b20IO = 1;      
    Ds18b20Delay(260); 
    Ds18b20IO = 0;    
    Ds18b20Delay(750);  //precise  between 480us and 960us
    Ds18b20IO = 1;   
    Ds18b20InputInitial();//set IO as input
    while((Ds18b20IO != 0)&&(Flag_1 == 1))//waitting for the response of ds18b20 and avoid excessively delay
    {                                      //waitting for about 60ms
        CONT_1++;
        Ds18b20Delay(10);
        if(CONT_1 > 8000)Flag_1 = 0;
        Status = Ds18b20IO;
    }
    Ds18b20OutputInitial();
    Ds18b20IO = 1;
    Ds18b20Delay(100);
    return Status;    
}

void Ds18b20Write(unsigned char infor)
{
    unsigned int i;
    Ds18b20OutputInitial();
    for(i=0;i<8;i++)
    {
        if((infor & 0x01))
        {
            Ds18b20IO = 0;
            Ds18b20Delay(6);
            Ds18b20IO = 1;
            Ds18b20Delay(50);
        }
        else
        {
            Ds18b20IO = 0;
            Ds18b20Delay(50);
            Ds18b20IO = 1;
            Ds18b20Delay(6);
        }
        infor >>= 1;
    }
}


unsigned char Ds18b20Read(void)
{
    unsigned char Value = 0x00;
    unsigned int i;
    Ds18b20OutputInitial();
    Ds18b20IO = 1;
    Ds18b20Delay(10);
    for(i=0;i<8;i++)
    {
        Value >>= 1; 
        Ds18b20OutputInitial();
        Ds18b20IO = 0;
        Ds18b20Delay(3);
        Ds18b20IO = 1;
        Ds18b20Delay(3);
        Ds18b20InputInitial();
        if(Ds18b20IO == 1) Value |= 0x80;
        Ds18b20Delay(15);
    } 
    return Value;
}

//read temperature
unsigned short ReadDs18B20(void){
    unsigned char V1,V2;   //two 8-bit buffers 
    
    Ds18b20Initial();
    Ds18b20Write(0xcc);    // omit the serial numberr
    Ds18b20Write(0x44);    // start measuring temperatrure
    
    Ds18b20Initial();
    Ds18b20Write(0xcc);    //omit the serial number
    Ds18b20Write(0xbe);    //read temperature registers. 9 of them are available. the first two registers records the temperature
    
    V1 = Ds18b20Read();    //low bits
    V2 = Ds18b20Read();    //hight bits

    return V2 <<8 | V1;
}


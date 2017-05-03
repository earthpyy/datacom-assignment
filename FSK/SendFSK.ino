#include <Wire.h>
#include <Adafruit_MCP4725.h>
#include <Adafruit_ADS1015.h>
#define defaultFreq 1700
#define freq0 500 
#define freq1 750 
#define freq2 1000 
#define freq3 1250 
#define BITS 8
int delay0, delay1, delay2, delay3;
const uint16_t S_DAC[4] = {2048,4095, 2048, 0};
Adafruit_MCP4725 dac;
int count=0;

String str;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  dac.begin(0x62);
  pinMode(A4, OUTPUT);
  pinMode(A5, OUTPUT);
  delay0 = (1000000 / freq0 - 1000000 / defaultFreq) / 4;
  delay1 = (1000000 / freq1 - 1000000 / defaultFreq) / 4;
  delay2 = (1000000 / freq2 - 1000000 / defaultFreq) / 4;
  delay3 = (1000000 / freq3 - 1000000 / defaultFreq) / 4;
  Serial.flush();
}

void loop() {
  // put your main code here, to run repeatedly:
//--------------------Search channal--------------------
//  if(count>=128) count=0;
//    int inData = 0+count;
//    count++;
//    {
//------------------------------------------------------
  if (Serial.available() > 0) {
    String tmp="";
    str = Serial.readString();
    str = Encode_Flag(str);
    //Serial.println(str);
    tmp = Packaging(str);
    tmp = makeFullBits(tmp); //concat '0' for full frame
    int n= tmp.length()/BITS; //count frame
    for(int i=0;i<n;i++)
    {
      sendFSK8bit(tmp);
      tmp=tmp.substring(BITS);
      delay(100);
      Serial.println();
    }
  }
}

String Packaging(String str)
{
  String tmp = "";
  String flag = "011110";
  tmp=flag.substring(0);
  tmp.concat(str);
  tmp.concat(flag);
  return tmp;
}

void sendFSK8bit(String str)
{
  int pta=-2,ptb=-1;
  //---------------loop 4 times for send data each 2 bits------------
  for (int nloop = 0; nloop < 4; nloop++) 
    {
      //--------------------Select 2 bits------------------------
//      int temp = inData & 3; // and bit 11 (2bit)
//      inData >>= 2;
      String temp;
      pta+=2;
      ptb+=2;
      temp=str.substring(pta,ptb+1);
      Serial.print(temp);
      //--------------------Select frequency---------------------
      if (temp == "00") {  // 2 frequency
        for (int sl = 0; sl < 2 ; sl++)
        {
          for (int s = 0; s < 4; s++)
          {
            dac.setVoltage(S_DAC[s], false); 
            delayMicroseconds(delay0);
          }
        }
      }
      else if (temp == "01") { //3 frequency
        for (int sl = 0; sl < 3; sl++)
        {
          for (int s = 0; s < 4; s++)
          {
            dac.setVoltage(S_DAC[s], false);
            delayMicroseconds(delay1);
          }
        }
      }
      else if (temp == "10") { //4 frequency
        for (int sl = 0; sl < 4; sl++)
        {
          for (int s = 0; s < 4; s++)
          {
            dac.setVoltage(S_DAC[s], false);
            delayMicroseconds(delay2);
          }
        }
      }
      else if (temp == "11") { //5 frequency
        for (int sl = 0; sl < 5; sl++)
        {
          for (int s = 0; s < 4; s++)
          {
            dac.setVoltage(S_DAC[s], false);
            delayMicroseconds(delay3);
          }
        }
      }  
    }
    dac.setVoltage(0, false);
}


String makeFullBits(String str)
{
  int bitmake = BITS;
  int len = str.length();
  int remainder = bitmake-(len%bitmake);
  //  i=length i<length+remainder i++ '\0'
  if(str.length()%bitmake!=0)
    for(int i=len;i<len+remainder;i++)
      str.concat("0");
  return str;
}

String Encode_Flag(String input_data)
{
  String output_data;
  int ip_index = 0 ; //input data index

  int check_t_one = 0 ; //count 1
  //if met third 1 insert 0 instance
  
  //loop input_data length
    for(ip_index = 0 ; ip_index < input_data.length(); ip_index++)
    {
        if(input_data[ip_index] == '0')
        {
          output_data.concat("0");
          check_t_one = 0 ;
        }
  
        if(input_data[ip_index] == '1')
        {
          output_data.concat("1");
          check_t_one ++ ;
   
          if(check_t_one == 3)
          {
             output_data.concat("0");
              check_t_one = 0 ;
          }
        }
    }
   // Serial.print(output_data);
    return output_data; 
}

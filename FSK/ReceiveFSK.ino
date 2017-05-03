#include<Wire.h>
#include<Adafruit_MCP4725.h>
#ifndef cbi
#define cbi(sfr,bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
# ifndef sbi
#define sbi(sfr,bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif
Adafruit_MCP4725 dac;
#define max1 950
#define min1 50
#define TIME_OUT 5
int prev = 0;
int check = false;
int prev1 = 0;
int upper = 0;
int count = 0;
int delay0 = 0;
int max = 0;
int start = 0;
String data = "";
int framecount = 0;
int framefail = false;
#define r_slope 65
#define defaultFreq 1700
String flag = "011110";
unsigned long currentTime,prevTime;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(A0, INPUT);
  sbi(ADCSRA, ADPS2);
  cbi(ADCSRA, ADPS1);
  cbi(ADCSRA, ADPS0);
  dac.begin(0x62);
  delay0 = (1000000 / 1300 - 1000000 / defaultFreq) / 4; //1300 max freq from Tx
  Serial.flush();
  prevTime=millis();
}

void loop() {
  // put your main code here, to run repeatedly:
  String tmp="";
  tmp = receiveFSK8bit(); // read FSK
  if(tmp!="") //has data
  {
    prevTime=millis();
    framecount++;
    data.concat(tmp); // first byte
    if(data.indexOf(flag)==0 && !framefail) // found start flag && start keep data bit
    {
      int idxflag = data.indexOf(flag);
      framefail=false;
      data=data.substring(idxflag+6);
      while(framecount<5) //don't time out
      {
        tmp = receiveFSK8bit(); //read FSK
        if(tmp!="") //has data
        {
          prevTime=millis();
          data.concat(tmp); //concat bit
          if(data.indexOf(flag)!=-1) // found stop flag
            break;
          framecount++;
        }
        currentTime=millis();
        if(currentTime-prevTime>150)//timeout
        {
          framecount=TIME_OUT;
          prevTime=currentTime;
        }
      }
      if(framecount<TIME_OUT) // don't time out and have stop flag
      {
        //SEND SUCCESS
        framefail=false;
        framecount=0;
        data=data.substring(0,data.indexOf(flag)); //remove flag
        data=Decode_Flag(data);
        //Serial.write(data.c_str(),data.length());
        Serial.print(data);
      }
      data="";
    }
    else //reject frame
    {
      framefail=true;
      data="";
    }
  }
  currentTime=millis();
  if(currentTime-prevTime>150)
  {
    framecount=TIME_OUT;
    prevTime=currentTime;
  }
  if(framecount==TIME_OUT)
  {
    framefail=false;
    framecount=0;
  }
}

String receiveFSK8bit()
{
  String data="";
  int tmp = analogRead(A0);
  if (tmp - prev1 > r_slope) // has data
  {
    for (int k = 0; k < 4; k++) //loop 4 times for 4FSK
    {
      for (int i = 0; i < 65; i++) //sampling count pulse
      {
        int tmp = analogRead(A0);

        if (tmp - prev > r_slope && check == false)
        {
          //Serial.println(tmp);
          max = 0;
          check = true;
        }
        if (tmp > max)
        {
          max = tmp;
        }
        if (max - tmp > r_slope)
        {
          if (check == true)
          {
            count++;
          }
          check = false;
        }
        prev = tmp;
        delayMicroseconds(delay0);
      }

      if (count == 2)
      {
        data.concat("00");
        //Serial.print(" 0 0 ");
        count = 0;
      }
      else if (count == 3)
      {
        data.concat("01");
        //Serial.print(" 0 1 ");
        count = 0;
      }
      else if (count == 4)
      {
        data.concat("10");
        //Serial.print(" 1 0 ");
        count = 0;
      }
      else if (count == 5)
      {
        data.concat("11");
        //Serial.print(" 1 1 ");
        count = 0;
      }
      else
      {
        count = 0;
        //Serial.print(" X X ");
      }
    }
    //Serial.println(data);
  }
  return data;
}

String Decode_Flag(String input_data)
{
  String output_data;
  int ip_index = 0 ; //input data index

  int check_t_one = 0 ; //count 1

  for (ip_index = 0 ; ip_index < input_data.length(); ip_index++)
  {
    if (input_data[ip_index] == '0')
    {
      output_data.concat("0");
      check_t_one = 0 ;
    }

    if (input_data[ip_index] == '1')
    {
      output_data.concat("1");
      check_t_one ++ ;

      if (check_t_one == 3)
      {
        ip_index++;
        check_t_one = 0 ;
      }
    }
  }
  return output_data;
}

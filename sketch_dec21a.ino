#include <LiquidCrystal.h>
#include <SoftwareSerial.h>                        
SoftwareSerial wifi_module(10,11);  
                  //rs.en.d4:d7
LiquidCrystal lcd(9, 8, 7, 6, 5, 4);
volatile int motion=0;
volatile int smoke=0;
int led2=1,led1=1;
void setup()
{
  DDRB|=(1<<4)|(1<<5);
  Serial.begin(9600);
  wifi_module.begin(115200); 
  InitWifiModule();
  lcd.begin(16, 2);
  intInit();
  adcInit();
  Delayby_s(1);

}

void loop() 
{
  connect();
}

ISR(TIMER1_OVF_vect) 
{
  display();
  TCNT1 = 32740;
  if((PIND&(1<<2))==0)
  {
      motion=0;
  }
  if((PIND&(1<<3))==8)
  {
     smoke=0;
  }

}
ISR(INT0_vect) 
{
  motion=1;
  display();
}
ISR(INT1_vect) 
{
  smoke=1;
  display();
}


void intInit()
{
  DDRD&=~(1<<2)&~(1<<3);
  TCCR1A=0;
  TCCR1B = 0b00000101;  //scala 1024
  TIMSK1 = (1 << TOIE1);               
  TCNT1 = 32740;                     
  EIMSK |=0x03;                      
  EICRA = 0x0b; // rising edge 
  sei();                             
}

float READ_TEMP()
{
  ADCSRA|=(1<<ADSC);
  while(ADCSRA & (1<<ADIF)==0);
  float TEMP=(ADCL|(ADCH<<8));
  TEMP=(TEMP*5000/1024)/10;
  return TEMP;
}

void adcInit()
{
  ADCSRA|= (1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0)|(1<<ADEN);
  ADMUX|=(1<<REFS0);
}

void sensorInit()
{
  DDRD&=~(1<<2);
}


void wifiCOMMAND(String command)
{                                             
  wifi_module.print(command);
  String h;
  long counter=1000000;
  while( counter > 0)                                 
  {      
    while(wifi_module.available())                                      
    {
      char c=wifi_module.read();
      h+=c; 
      counter--;                                                                  
    } 
    counter--; 
  }    
  Serial.println(h);
}

void InitWifiModule()
{
  wifiCOMMAND("AT+RST\r\n");                                                  
  wifiCOMMAND("AT+CWJAP=\"eng-sayed\",\"elcom01061416112\"\r\n");        
  Delayby_s(4);
  wifiCOMMAND("AT+CWMODE=1\r\n");                                             
  Delayby_s(2);
  wifiCOMMAND("AT+CIFSR\r\n");                                             
  Delayby_s(2);
  wifiCOMMAND("AT+CIPMUX=1\r\n");                                             
  Delayby_s(2);
  wifiCOMMAND("AT+CIPSERVER=1,80\r\n"); 

}

void connect()
{
   if(wifi_module.available())                                           
 {    
    if(wifi_module.find("+IPD,"))
    {
     Delayby_s(1000);
 
      int connectionId = wifi_module.read()-48; 
      String request = wifi_module.readStringUntil('\r');   
      Serial.println("requestt "+request); 
/*
           String webpage1 = "<html>"
                   "<body background=\"palestine_flag.jpg\" style=\"text-align: center; font-family: sans-serif;\">"
                   "<div style=\"background-color: rgba(255, 255, 255, 0.8); padding: 20px; border-radius: 10px;\">"
                   "<h1 style=\"color: green;\">Control LEDs</h1>"
                   "<form>"
                   "<button name=\"led1\" value=\"Togg\" type=\"submit\" style=\"background-color: black; color: white; border: none; padding: 10px 20px; border-radius: 5px; cursor: pointer;\">LED 1</button>"
                   "<br>"
                   "<button name=\"led2\" value=\"Togg\" type=\"submit\" style=\"background-color: black; color: white; border: none; padding: 10px 20px; border-radius: 5px; cursor: pointer;\">LED 2</button>"
                   "</form>"
                   "</div>"
                   "</body>"
                   "</html>";*/                     
      String webpage1 = "<html><body><h1>Control LEDs</h1><form><button name=\"led1\" value=\"Togg\" type=\"submit\">LED 1 </button><br><button name=\"led2\" value=\"Togg\" type=\"submit\">LED 2</button></form></body></html>";
    
      String webpage2 ="<html><body><h1>abnormal</h1><form><button name=\"led1\" value=\"Togg\" type=\"submit\">LED 1 </button><br><button name=\"led2\" value=\"Togg\" type=\"submit\">LED 2</button></form><h1>ther is smoke or temp more than 30 plz check it </h1></body></html>";
      String AT_cipSend = "AT+CIPSEND=";
      AT_cipSend += connectionId;
      AT_cipSend += ",";
      if(smoke==0)
      {
        AT_cipSend +=webpage1.length();
      }
      else
      {
        AT_cipSend +=webpage2.length();
      }
        
      AT_cipSend +="\r\n";
      if (request.indexOf("/?led1") != -1) 
      {
        if(led1==1)
        {
          PORTB|=(1<<4);
          led1=0;
        }
        else
        {
          PORTB&=~(1<<4);
          led1=1;
        }
      }
      if (request.indexOf("/?led2") != -1)
      {
        if(led2==1)
        {
            PORTB|=(1<<5);
            led2=0;
        }
        else
        {
          PORTB&=~(1<<5);
          led2=1;
        }
        
      }

     wifiCOMMAND(AT_cipSend);
     if(smoke==0 ||READ_TEMP()<30)
     {
       wifiCOMMAND(webpage1);
     }
     
     else
     {
       wifiCOMMAND(webpage2);
     }
     

     String closeCommand = "AT+CIPCLOSE="; 
     closeCommand+=connectionId;
     closeCommand+="\r\n";    
     wifiCOMMAND(closeCommand);
    }
  }
}

void Delayby_s(int n) {
  for (int i = 0; i < 1000 * n; i++) {
    TCCR2A = 0x0;
    TCCR2B = 0x04;
    TCNT2 = 0x00;
    while ((TIFR2 & 1) == 0);
    TIFR2 |= (1 << TOV2);
  }
}

void display() {
  lcd.clear();                                             
  lcd.setCursor(0, 0);                                      
  lcd.print("Temp: ");                                    
  lcd.print(READ_TEMP());                                 
  lcd.print(" C");                                         
  lcd.setCursor(0, 1);                                     
  lcd.print("Motion: ");                                   
  lcd.print(motion ? "Y" : "N");  
  lcd.setCursor(9, 1);                                    
  lcd.print("Smoke:");                                     
  lcd.print(smoke ? "Y": "N");                              
}
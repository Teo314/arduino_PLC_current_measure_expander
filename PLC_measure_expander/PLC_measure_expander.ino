#include <EEPROM.h>
#include <PCF8574.h>
#include <Wire.h>

#define PLC_OUT1 0
#define PLC_OUT2 1
#define PLC_OUT3 2
#define PLC_OUT4 3
#define PLC_OUT5 4
#define PLC_OUT6 5
#define enkoderBTN 2
#define enkoderCLK 6
#define enkoderDT 7
#define LEDset1 12
#define LEDset2 11
#define LEDset3 10
#define LEDset4 9
#define LEDset5 8
#define LEDlvl1 7
#define LEDlvl2 6
#define LEDlvl3 5
#define LEDlvl4 4
#define LEDlvl5 3

//poziom zerowy czujnika natężenia prądu
#define currentZeroLvl 512

PCF8574 expander;

int state = 0;
int lastState = 0;
int counter = 0;
int actualChannelSet = 1;
int currentLVL[5] = {0, 0, 0, 0, 0};
int currentADC = 0;
int currentADCscaled = 0;

void setup() {
  Serial.begin(9600);
  
  //ustawienia pinów IO
  expander.begin(0x20);
  expander.pinMode(0, OUTPUT);
  expander.pinMode(1, OUTPUT);
  expander.pinMode(2, OUTPUT);
  expander.pinMode(3, OUTPUT);
  expander.pinMode(4, OUTPUT);
  expander.pinMode(5, OUTPUT);
  expander.pinMode(6, INPUT);
  expander.pinMode(7, INPUT);
  expander.digitalWrite(PLC_OUT1, LOW);
  expander.digitalWrite(PLC_OUT2, LOW);
  expander.digitalWrite(PLC_OUT3, LOW);
  expander.digitalWrite(PLC_OUT4, LOW);
  expander.digitalWrite(PLC_OUT5, LOW);
  expander.digitalWrite(PLC_OUT6, LOW);
  
  //ustawienia LEDów
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(LEDset1, OUTPUT);
  pinMode(LEDset2, OUTPUT);
  pinMode(LEDset3, OUTPUT);
  pinMode(LEDset4, OUTPUT);
  pinMode(LEDset5, OUTPUT);
  
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);
  pinMode(A3, INPUT);
  pinMode(A6, INPUT);
  pinMode(A7, INPUT);
  
  //ustawienia pinów enkodera 
  expander.pinMode(enkoderCLK, INPUT_PULLUP);
  expander.pinMode(enkoderDT, INPUT_PULLUP);
  pinMode(enkoderBTN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(enkoderBTN), setCurrentChannel, FALLING);
  lastState = expander.digitalRead(enkoderCLK);
  
  //odczytanie z pamięci wartości progowych dla pomiaru natężenia prądu (ACS712)
  EEPROM.get(0, currentLVL[0]);
  EEPROM.get(2, currentLVL[1]);
  EEPROM.get(4, currentLVL[2]);
  EEPROM.get(6, currentLVL[3]);
  EEPROM.get(8, currentLVL[4]);
}

void loop() {

  convertCurrentToImpulses(); 
  measureCurrent();
     
}

void convertCurrentToImpulses()
{
  //pomiar prądu i wygenerowanie proporcjonalnej serii impulsów do PLC
  int currentValue = analogRead(A0);
  int impulsesNumber = currentValue / 40;
  int T = 500 / impulsesNumber;
  for(int i = 0; i < impulsesNumber * 2; i++)
  {
    expander.digitalWrite(PLC_OUT1, LOW);
    delay(T);
    expander.digitalWrite(PLC_OUT1, HIGH);
    delay(T);
  }
  
}

void measureCurrent()
{

  int plcOut[5] = {PLC_OUT6, PLC_OUT5, PLC_OUT4, PLC_OUT3, PLC_OUT2};
  int analogChannel[5] = {A1, A2, A3, A6, A7};
  
  for (int i = 0; i < 5; i++)
  {
    currentADC = 0;
    
    for (int j = 0; j < 10; j++)
        {
          currentADC += analogRead(analogChannel[i]);
        }
        currentADC = currentADC /10;
        
      if(currentADC > currentZeroLvl) 
      {
        currentADCscaled = currentADC - currentZeroLvl;
      }
      else
      {
        currentADCscaled = currentZeroLvl - currentADC;
      }
       
       
     if(currentADCscaled > (currentLVL[i]))
     {
       expander.digitalWrite(plcOut[i], LOW);
     }
     else
     {
       expander.digitalWrite(plcOut[i], HIGH);
     }
     
  }
}


void setCurrentChannel()
{
  //funckja pozwalająca zmienić wartość progową wybranego kanału
  
    int time = 0;
    while (time < 5000)    //wybór kanału (przez ograniczony czas od ostatniego przyciśnięcia / obrotu enkoderem)
    {      
      state = expander.digitalRead(enkoderCLK);
      if(state != lastState)
      {
        if(expander.digitalRead(enkoderDT) != state)
        {
          actualChannelSet++;
          if(actualChannelSet < 1)
          {
            actualChannelSet = 5;
          }
        }
        else
        {
          actualChannelSet--;
          if(actualChannelSet > 5)
          {
            actualChannelSet = 1;
          }
        }              
        time = 0;                         
        lastState = state; 
      }   
      digitalWrite(LEDset1, LOW);
      digitalWrite(LEDset2, LOW);
      digitalWrite(LEDset3, LOW);
      digitalWrite(LEDset4, LOW);
      digitalWrite(LEDset5, LOW);
      
      //zapalenie LED przy wejściu na którym będzie ustawiana wartość progowa
      if(actualChannelSet == 1) 
      {
        digitalWrite(LEDset1, HIGH);
      }
      else
      {
        if(actualChannelSet == 2) 
        {
          digitalWrite(LEDset2, HIGH);
        }     
        else
        {
          if(actualChannelSet == 3) 
          {
            digitalWrite(LEDset3, HIGH);
          }
          else
          {
            if(actualChannelSet == 4) 
            {
              digitalWrite(LEDset4, HIGH);
            }
            else
            {
              if(actualChannelSet == 5) 
              {
                digitalWrite(LEDset5, HIGH);
              }
            }
          }
        }
      }
      time++;
      
      //przejdź do ustawiania wartości progowej dla wybranego kanału jeśli przytrzymano przycisk enkodera
      if(digitalRead(enkoderBTN) == 0)
      {
        delay(300);
        if(digitalRead(enkoderBTN) == 0) setCurrentValue(actualChannelSet - 1); 
      }     
    }
    digitalWrite(LEDset1, LOW);
    digitalWrite(LEDset2, LOW);
    digitalWrite(LEDset3, LOW);
    digitalWrite(LEDset4, LOW);
    digitalWrite(LEDset5, LOW);
}

void setCurrentValue(int channel)
{
  int time = 0;
  while (time < 10000)    //ustawienie wartości progowej, której przekroczenie skutkuje zmianą stanu na wyjściu
    {
      state = expander.digitalRead(enkoderCLK);
      if(state != lastState)
      {
        if(expander.digitalRead(enkoderDT) != state)
        {
          currentLVL[channel] += 1;
          if(currentLVL[channel] > 512) currentLVL[channel]=512;
        }
        else
        {
          currentLVL[channel] -= 1;
          if(currentLVL[channel] < 0) currentLVL[channel]=0;
        }   
        EEPROM.put(channel*2, currentLVL[channel]);  //zapis ustawienia do pamięci
        lastState = state;
        time = 0;        
      }  
      
    //wskaźnik LED wartości progowej
    if(currentLVL[channel] > 2)  //LED poziom 1
    {
      digitalWrite(LEDlvl1, HIGH);
    }
    else
    {
      digitalWrite(LEDlvl1, LOW);
    }
    if(currentLVL[channel] > 5)  //LED poziom 2
    {
      digitalWrite(LEDlvl2, HIGH);
    }
    else
    {
      digitalWrite(LEDlvl2, LOW);
    }
    if(currentLVL[channel] > 20)  //LED poziom 3
    {
      digitalWrite(LEDlvl3, HIGH);
    }
    else
    {
      digitalWrite(LEDlvl3, LOW);
    }
    if(currentLVL[channel] > 100)  //LED poziom 4
    {
      digitalWrite(LEDlvl4, HIGH);
    }
    else
    {
      digitalWrite(LEDlvl4, LOW);
    }
    if(currentLVL[channel] > 300)  //LED poziom 5
    {
      digitalWrite(LEDlvl5, HIGH);
    }
    else
    {
      digitalWrite(LEDlvl5, LOW);
    }    
    time++;
  }
  digitalWrite(LEDlvl1, LOW);
  digitalWrite(LEDlvl2, LOW);
  digitalWrite(LEDlvl3, LOW);
  digitalWrite(LEDlvl4, LOW);
  digitalWrite(LEDlvl5, LOW);
  digitalWrite(LEDset1, LOW);
  digitalWrite(LEDset2, LOW);
  digitalWrite(LEDset3, LOW);
  digitalWrite(LEDset4, LOW);
  digitalWrite(LEDset5, LOW);
}



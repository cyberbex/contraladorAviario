#include <Arduino.h>
#include <EEPROM.h>
#include <LiquidCrystal_PCF8574.h>
#include <Wire.h>
#include <DallasTemperature.h>
#include "STM32TimerInterrupt.h"
#include "STM32TimerInterrupt.h"
#include "STM32_ISR_Timer.h"

byte smiley[8] = {
    B10000,
    B01000,
    B00100,
    B00010,
    B00100,
    B01000,
    B10000,
};

LiquidCrystal_PCF8574 lcd(0x27); // set the LCD address to 0x27 for a 16 chars and 2 line display

#define ONE_WIRE_BUS PB0
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
#define tempo_leiruraTemp 20 // leitura da temperarura a cada 2 segundos
#define UmSegundo 10

#define TIMER_INTERVAL_MS 100
#define HW_TIMER_INTERVAL_MS 50

STM32Timer ITimer(TIM2);
STM32_ISR_Timer ISR_Timer;

#define TIMER_INTERVAL_50ms 100L //100ms
//#define TIMER_INTERVAL_1MS            500L //1ms

#define releG1 PA0
#define releG2 PA1
#define releNebolizacao PA2
#define releEvaporativa PA3
#define releAlarme PA4

#define bt1 PB12
#define bt2 PB13
#define bt3 PB14
#define bt4 PB15
#define bt6 PA8

#define tempoMaximo 9999
#define tempoMinimo 0x00

void TimerHandler()
{
   ISR_Timer.run();
}

uint8_t sensorTemp[8] = {0x28, 0xCE, 0x0C, 0x45, 0x06, 0x00, 0x00, 0x03};
int show = -1;

float temp_av723;
float TempLigaGrupo1 = 29, TempLigaGrupo2 = 25, TempLigaGrupo3=21;
float TempDesligaGrupo1 = 28, TempDesligaGrupo2 = 24, TempDesligaGrupo3=20;
float TempLigaNebolizacao = 25, TempLigaEvaporativa = 28;
float TempDesligaNebolizacao = 23, TempDesligaEvaporativa = 26;

float TempMaxAlarme = 28, TempMinAlarme = 15;

void LerTemperaturas(void);
void printTempDisplay(void);
void controla_grupo1(void);
void controla_grupo2(void);
void contralaEvaporativa(void);
void contralaNebolizacao(void);
void controlaAlarme(void);
void leituraBotoes(void);
void menu(void);
void setaAlarme(void);
void setaGrupo1(void);
void setaGrupo2(void);
void setaGrupo3(void);
void setaNebolizacao(void);
void setaEvaporativa(void);
void temporizador(void);
void saveEEprom(void);
void loadEEprom(void);

u_int8_t countLerTemp = 0, countTempo, posicaoMenu = 1, posicaoSeta = 1, counterControle = 0, auxMenu = 1;
bool flag_grupo1 = false, flag_grupo2 = false, flagEvaporativa = false, flagNebolizacao = false;
bool flag_ler_temp = false, flag_controle = false, flagLerBotoes = false;
bool flag_controleTemporizadorG1, flag_controleTemporizadorG2, flag_controleTemporizadorG3;

bool bt1_f = 0x00;
bool bt2_f = 0x00;
bool bt3_f = 0x00;
bool bt4_f = 0x00;
//bool bt5_f = 0x00;
bool bt6_f = 0x00;

u_int16_t count_grupo1 = 0, count_grupo2 = 0, count_grupo3 = 0, count_nebolizacao = 0, count_evaporativa = 0;

u_int16_t tempo_grupo1Liga = 50, tempo_grupo2Liga = 0, tempo_grupo3Liga = 0;
u_int16_t tempo_grupo1Desliga = 50, tempo_grupo2Desliga = 0, tempo_grupo3Desliga = 0;

u_int16_t tempo_nebolizacaoLiga = 100, tempo_evaporativaLiga = 100;

u_int16_t tempo_nebolizacaoDesliga = 100, tempo_evaporativaDesliga = 100;

void setup()
{

   sensors.begin();
   EEPROM.begin();
   lcd.clear();
   lcd.setBacklight(255);

   Wire.begin();
   Wire.beginTransmission(0x27);

   lcd.createChar(0, smiley);
   lcd.begin(16, 4);

   ITimer.attachInterruptInterval(HW_TIMER_INTERVAL_MS * 1000, TimerHandler);
   ISR_Timer.setInterval(TIMER_INTERVAL_50ms, temporizador); //100ms

   pinMode(releG1, OUTPUT);
   digitalWrite(releG1, LOW);
   pinMode(releG2, OUTPUT);
   digitalWrite(releG2, LOW);
   pinMode(releAlarme, OUTPUT);
   digitalWrite(releAlarme, LOW);
   pinMode(releNebolizacao, OUTPUT);
   digitalWrite(releNebolizacao, LOW);
   pinMode(releEvaporativa, OUTPUT);
   digitalWrite(releEvaporativa, LOW);

   pinMode(bt1, INPUT_PULLUP);
   pinMode(bt2, INPUT_PULLUP);
   pinMode(bt3, INPUT_PULLUP);
   pinMode(bt4, INPUT_PULLUP);
   pinMode(bt6, INPUT_PULLUP);
   loadEEprom();
   delay(100);

} // setup()

void loop()
{
   if (flag_ler_temp == true)
   {
      LerTemperaturas();
      printTempDisplay();
      flag_ler_temp = false;
   }

   if (flag_controle == true)
   {
      flag_controle = false;

      controla_grupo1();
      controla_grupo2();
      contralaEvaporativa();
      contralaNebolizacao();
      controlaAlarme();
   }

   if (flagLerBotoes == true)
   {
      leituraBotoes();
      flagLerBotoes = false;
   }

} // loop()

void printTempDisplay()
{
   switch (posicaoMenu)
   {
   case 1:
      lcd.setCursor(0, 0);
      lcd.print("Temp: ");
      lcd.print(temp_av723);
      break;

   case 2:
      menu();
      break;

   case 3:
      setaAlarme();
      break;

   case 4:
      setaGrupo1();
      break;

   case 5:
      setaGrupo2();
      break;

   case 6:
      setaGrupo3();
      break;

   case 7:
      setaNebolizacao();
      break;

   case 8:
      setaEvaporativa();
      break;
   }
}
void menu()
{
   switch (posicaoSeta)
   {
   case 1:
      lcd.setCursor(0, 0);
      lcd.write(byte(0));
      lcd.setCursor(1, 0);
      lcd.print("Alarme");
      lcd.setCursor(1, 1);
      lcd.print("Grupo1");
      lcd.setCursor(1, 2);
      lcd.print("Grupo2");
      lcd.setCursor(1, 3);
      lcd.print("Grupo3");

      break;
   case 2:
      lcd.setCursor(1, 0);
      lcd.print("Alarme");
      lcd.setCursor(0, 1);
      lcd.write(byte(0));
      lcd.setCursor(1, 1);
      lcd.print("Grupo1");
      lcd.setCursor(1, 2);
      lcd.print("Grupo2");
      lcd.setCursor(1, 3);
      lcd.print("Grupo3");
      break;
   case 3:
      lcd.setCursor(1, 0);
      lcd.print("Alarme");
      lcd.setCursor(1, 1);
      lcd.print("Grupo1");
      lcd.setCursor(0, 2);
      lcd.write(byte(0));
      lcd.setCursor(1, 2);
      lcd.print("Grupo2");
      lcd.setCursor(1, 3);
      lcd.print("Grupo3");

      break;
   case 4:
      lcd.setCursor(1, 0);
      lcd.print("Alarme");
      lcd.setCursor(1, 1);
      lcd.print("Grupo1");
      lcd.setCursor(1, 2);
      lcd.print("Grupo2");
      lcd.setCursor(0, 3);
      lcd.write(byte(0));
      lcd.setCursor(1, 3);
      lcd.print("Grupo3");
      break;
   case 5:
      lcd.setCursor(1, 0);
      lcd.print("Grupo1");
      lcd.setCursor(1, 1);
      lcd.print("Grupo2");
      lcd.setCursor(1, 2);
      lcd.print("Grupo3");
      lcd.setCursor(0, 3);
      lcd.write(byte(0));
      lcd.setCursor(1, 3);
      lcd.print("Nebolizacao");
      break;
   case 6:
      lcd.setCursor(1, 0);
      lcd.print("Grupo2");
      lcd.setCursor(1, 1);
      lcd.print("Grupo3");
      lcd.setCursor(1, 2);
      lcd.print("Nebolizar");
      lcd.setCursor(0, 3);
      lcd.write(byte(0));
      lcd.setCursor(1, 3);
      lcd.print("Evaporativa");
      break;
   }
}
void LerTemperaturas()
{
   sensors.requestTemperatures();
   temp_av723 = sensors.getTempC(sensorTemp);
}

void controla_grupo1()
{

   if (tempo_grupo1Liga == 0x00 && tempo_grupo1Desliga == 0x00)
      flag_controleTemporizadorG1 = false;
   if (tempo_grupo1Liga != 0x00 && tempo_grupo1Desliga != 0x00 && temp_av723 < TempDesligaGrupo1)
      flag_controleTemporizadorG1 = true;

   if (temp_av723 > TempLigaGrupo1 && flag_grupo1 == false)
   {
      digitalWrite(releG1, HIGH);
      flag_grupo1 = true;
      flag_controleTemporizadorG1 = false;
   }
   else if (temp_av723 < TempDesligaGrupo1 && flag_grupo1 == true && flag_controleTemporizadorG1 == false)
   {
      digitalWrite(releG1, LOW);
      flag_grupo1 = false;
   }

   if (flag_controleTemporizadorG1 == true)
   {
      if (count_grupo1 >= tempo_grupo1Desliga && flag_grupo1 == false)
      {
         count_grupo1 = 0x00;
         flag_grupo1 = true;
         digitalWrite(releG1, HIGH);
      }

      if (count_grupo1 >= tempo_grupo1Liga && flag_grupo1 == true)
      {
         count_grupo1 = 0x00;
         flag_grupo1 = false;
         digitalWrite(releG1, LOW);
      }
   }

   if (count_grupo1 > tempoMaximo)
      count_grupo1 = 0x00;
}

void controla_grupo2()
{

   if (tempo_grupo2Liga == 0x00 && tempo_grupo2Desliga == 0x00)
      flag_controleTemporizadorG2 = false;
   if (tempo_grupo2Liga != 0x00 && tempo_grupo2Desliga != 0x00 && temp_av723 < TempDesligaGrupo2)
      flag_controleTemporizadorG2 = true;

   if (temp_av723 > TempLigaGrupo2 && flag_grupo2 == false)
   {
      digitalWrite(releG2, HIGH);
      flag_grupo2 = true;
      flag_controleTemporizadorG2 = false;
   }
   else if (temp_av723 < TempDesligaGrupo2 && flag_grupo2 == true && flag_controleTemporizadorG2 == false)
   {
      digitalWrite(releG2, LOW);
      flag_grupo2 = false;
   }

   if (flag_controleTemporizadorG2 == true)
   {
      if (count_grupo2 >= tempo_grupo2Desliga && flag_grupo2 == false)
      {
         count_grupo2 = 0x00;
         flag_grupo2 = true;
         digitalWrite(releG2, HIGH);
      }

      if (count_grupo2 >= tempo_grupo2Liga && flag_grupo2 == true)
      {
         count_grupo2 = 0x00;
         flag_grupo2 = false;
         digitalWrite(releG2, LOW);
      }
   }

   if (count_grupo2 > tempoMaximo)
      count_grupo2 = 0x00;
}
void contralaEvaporativa()
{

   if (temp_av723 > TempLigaEvaporativa)
   {

      if (count_evaporativa > tempo_evaporativaDesliga && flagEvaporativa == false)
      {
         flagEvaporativa = true;
         digitalWrite(releEvaporativa, HIGH);
         count_evaporativa = 0x00;
      }

      if (count_evaporativa > tempo_evaporativaLiga && flagEvaporativa == true)
      {

         digitalWrite(releEvaporativa, LOW);
         flagEvaporativa = false;
         count_evaporativa = 0x00;
      }
   }

   else if (temp_av723 < TempDesligaEvaporativa && flagEvaporativa == true)
   {
      digitalWrite(releEvaporativa, LOW);
   }
   if (count_evaporativa > tempoMaximo)
      count_evaporativa = 0x00;
}

void contralaNebolizacao()

{

   if (temp_av723 > TempLigaNebolizacao)
   {

      if (count_nebolizacao > tempo_nebolizacaoDesliga && flagNebolizacao == false)
      {
         flagNebolizacao = true;
         digitalWrite(releNebolizacao, HIGH);
         count_nebolizacao = 0x00;
      }

      if (count_nebolizacao > tempo_nebolizacaoLiga && flagNebolizacao == true)
      {

         digitalWrite(releNebolizacao, LOW);
         flagNebolizacao = false;
         count_nebolizacao = 0x00;
      }
   }

   else if (temp_av723 < TempDesligaNebolizacao)
   {
      digitalWrite(releNebolizacao, LOW);
   }
   if (count_nebolizacao > tempoMaximo)
      count_nebolizacao = 0x00;
}
void controlaAlarme()
{

   if (temp_av723 > TempMinAlarme && temp_av723 < TempMaxAlarme)
      digitalWrite(releAlarme, LOW);
   if (temp_av723 <= TempMinAlarme || temp_av723 >= TempMaxAlarme)
      digitalWrite(releAlarme, HIGH);  
   
}

void leituraBotoes()
{

   //-----------Botão1-SetaParaBaixo------------------
   if (!digitalRead(bt1))
      bt1_f = 0x01;

   if (digitalRead(bt1) && bt1_f == 0x01)
   {
      bt1_f = 0x00;
      lcd.clear();
      
      //--move seta menu---------------------
      if (posicaoMenu == 2)
         posicaoSeta++;
      if(posicaoSeta > 6) 
         posicaoSeta = 0x06;
      //-------------------------------------

      //-----alarme---------------------------
      if (posicaoMenu == 3 && auxMenu == 1)
      {
         TempMaxAlarme = TempMaxAlarme - 0.5;
         if (TempMaxAlarme < 0x00)
            TempMaxAlarme = 0x00;
      }

      if (posicaoMenu == 3 && auxMenu == 2)
      {
         TempMinAlarme = TempMinAlarme - 0.5;
         if (TempMinAlarme < 0x00)
            TempMinAlarme = 0x00;
      }
      //-----------------------------------------

      //----grupo1-------------------------------
      if(posicaoMenu == 4 && auxMenu == 1)
      {
         TempLigaGrupo1 = TempLigaGrupo1 - 0.5;
         if (TempLigaGrupo1 < 0x00)
            TempLigaGrupo1 = 0x00;
      }
      if(posicaoMenu == 4 && auxMenu == 2)
      {
         TempDesligaGrupo1 = TempDesligaGrupo1 - 0.5;
         if (TempDesligaGrupo1 < 0x00)
            TempDesligaGrupo1 = 0x00;
      }
      if(posicaoMenu == 4 && auxMenu == 3)
      {
         tempo_grupo1Liga = tempo_grupo1Liga - 10;
         if(tempo_grupo1Liga > 9999)
            tempo_grupo1Liga = 0x00;
      }
      if(posicaoMenu == 4 && auxMenu == 4)
      {
         tempo_grupo1Desliga = tempo_grupo1Desliga - 10;
         if(tempo_grupo1Desliga > 9999)
            tempo_grupo1Desliga = 0x00;
      }
      //---fim grupo1----------------------------------------

      //----grupo2-------------------------------
      if(posicaoMenu == 5 && auxMenu == 1)
      {
         TempLigaGrupo2 = TempLigaGrupo2 - 0.5;
         if (TempLigaGrupo2 < 0x00)
            TempLigaGrupo2 = 0x00;
      }
      if(posicaoMenu == 5 && auxMenu == 2)
      {
         TempDesligaGrupo2 = TempDesligaGrupo2 - 0.5;
         if (TempDesligaGrupo2 < 0x00)
            TempDesligaGrupo2 = 0x00;
      }
      if(posicaoMenu == 5 && auxMenu == 3)
      {
         tempo_grupo2Liga = tempo_grupo2Liga - 10;
         if(tempo_grupo2Liga > 9999)
            tempo_grupo2Liga = 0x00;
      }
      if(posicaoMenu == 5 && auxMenu == 4)
      {
         tempo_grupo2Desliga = tempo_grupo2Desliga - 10;
         if(tempo_grupo2Desliga > 9999)
            tempo_grupo2Desliga = 0x00;
      }
      //---fim grupo2----------------------------------------
      
      //--Decrementa variáveis nebolização------------------
      
      if(posicaoMenu == 7 && auxMenu == 1)
      {
         TempLigaNebolizacao = TempLigaNebolizacao - 0.5;
         if (TempLigaNebolizacao < 0x00)
            TempLigaNebolizacao = 0x00;
      }
      if(posicaoMenu == 7 && auxMenu == 2)
      {
         TempDesligaNebolizacao = TempDesligaNebolizacao - 0.5;
         if (TempDesligaNebolizacao < 0x00)
            TempDesligaNebolizacao = 0x00;
      }
      if(posicaoMenu == 7 && auxMenu == 3)
      {
         tempo_nebolizacaoLiga = tempo_nebolizacaoLiga - 10;
         if(tempo_nebolizacaoLiga > 9999)
            tempo_nebolizacaoLiga = 0x00;
      }
      if(posicaoMenu == 7 && auxMenu == 4)
      {
         tempo_nebolizacaoDesliga = tempo_nebolizacaoDesliga - 10;
         if(tempo_nebolizacaoDesliga > 9999)
            tempo_nebolizacaoDesliga = 0x00;
      }   

      //fim-------------------------------------------------
      
      if(posicaoMenu == 4 && auxMenu == 2)

      if (posicaoSeta > 6)
         posicaoSeta = 6;
      printTempDisplay();
   
   }//--Fim botao1

   

   //-----------Botão2-SetaParaCima------------------
   if (!digitalRead(bt2))
      bt2_f = 0x01;

   if (digitalRead(bt2) && bt2_f == 0x01)
   {
      bt2_f = 0x00;
      lcd.clear();

      //--Move seta menu------------------------------
      if (posicaoMenu == 2)
         posicaoSeta--;
      if(posicaoSeta < 0x01)
         posicaoSeta = 0x01;   
      //----------------------------------------------

      //--alarme--------------------------------------
      if (posicaoMenu == 3 && auxMenu == 1)
      {
         TempMaxAlarme = TempMaxAlarme + 0.5;
         if (TempMaxAlarme > 40)
            TempMaxAlarme = 40;
      }
      if (posicaoMenu == 3 && auxMenu == 2)
      {
         TempMinAlarme = TempMinAlarme + 0.5;
         if (TempMinAlarme > 40)
            TempMinAlarme = 40;
      }
      //fim alarme--------------------------------------------
     
      //----grupo1-------------------------------
      if(posicaoMenu == 4 && auxMenu == 1)
      {
         TempLigaGrupo1 = TempLigaGrupo1 + 0.5;
         if (TempLigaGrupo1 > 40)
            TempLigaGrupo1 = 40;
      }
      if(posicaoMenu == 4 && auxMenu == 2)
      {
         TempDesligaGrupo1 = TempDesligaGrupo1 + 0.5;
         if (TempDesligaGrupo1 > 40)
            TempDesligaGrupo1 = 40;
      }
      if(posicaoMenu == 4 && auxMenu == 3)
      {
         tempo_grupo1Liga = tempo_grupo1Liga + 10;
         if(tempo_grupo1Liga > 9999)
            tempo_grupo1Liga = 9999;
      }
      if(posicaoMenu == 4 && auxMenu == 4)
      {
         tempo_grupo1Desliga = tempo_grupo1Desliga + 10;
         if(tempo_grupo1Desliga > 9999)
            tempo_grupo1Desliga = 9999;
      }
      //---fim grupo1----------------------------------------

       //----grupo2-------------------------------
      if(posicaoMenu == 5 && auxMenu == 1)
      {
         TempLigaGrupo2 = TempLigaGrupo2 + 0.5;
         if (TempLigaGrupo2 > 40)
            TempLigaGrupo2 = 40;
      }
      if(posicaoMenu == 5 && auxMenu == 2)
      {
         TempDesligaGrupo2 = TempDesligaGrupo2 + 0.5;
         if (TempDesligaGrupo2 > 40)
            TempDesligaGrupo2 = 40;
      }
      if(posicaoMenu == 5 && auxMenu == 3)
      {
         tempo_grupo2Liga = tempo_grupo2Liga + 10;
         if(tempo_grupo2Liga > 9999)
            tempo_grupo2Liga = 9999;
      }
      if(posicaoMenu == 5 && auxMenu == 4)
      {
         tempo_grupo2Desliga = tempo_grupo2Desliga + 10;
         if(tempo_grupo2Desliga > 9999)
            tempo_grupo2Desliga = 9999;
      }
      //---fim grupo2----------------------------------------

      //--Incrementa variáveis nebolização------------------
      
      if(posicaoMenu == 7 && auxMenu == 1)
      {
         TempLigaNebolizacao = TempLigaNebolizacao + 0.5;
         if (TempLigaNebolizacao > 40)
            TempLigaNebolizacao = 40;
      }
      if(posicaoMenu == 7 && auxMenu == 2)
      {
         TempDesligaNebolizacao = TempDesligaNebolizacao + 0.5;
         if (TempDesligaNebolizacao > 40)
            TempDesligaNebolizacao = 40;
      }
      if(posicaoMenu == 7 && auxMenu == 3)
      {
         tempo_nebolizacaoLiga = tempo_nebolizacaoLiga + 10;
         if(tempo_nebolizacaoLiga > 9999)
            tempo_nebolizacaoLiga = 9999;
      }
      if(posicaoMenu == 7 && auxMenu == 4)
      {
         tempo_nebolizacaoDesliga = tempo_nebolizacaoDesliga + 10;
         if(tempo_nebolizacaoDesliga > 9999)
            tempo_nebolizacaoDesliga = 9999;
      }   

      //fim-------------------------------------------------
      
      if (posicaoSeta < 1)
         posicaoSeta = 1;
      printTempDisplay();
   }

   //------------------------------------

   //-----------Botão3 OK------------------
   if (!digitalRead(bt3))
      bt3_f = 0x01;

   if (digitalRead(bt3) && bt3_f == 0x01)
   {
      bt3_f = 0x00;
      lcd.clear();
      
      //---função do botão OK, caso a posição menu seja = 2.  
      if (posicaoMenu == 2 && posicaoSeta == 1)
         posicaoMenu = 3;
      else if (posicaoMenu == 2 && posicaoSeta == 2)
         posicaoMenu = 4;
      else if (posicaoMenu == 2 && posicaoSeta == 3)
         posicaoMenu = 5;
      else if (posicaoMenu == 2 && posicaoSeta == 4)
         posicaoMenu = 6;
      else if (posicaoMenu == 2 && posicaoSeta == 5)
         posicaoMenu = 7;
      else if (posicaoMenu == 2 && posicaoSeta == 6)
         posicaoMenu = 8;
      //-------------------------------------------------------
      
      //caso posição menu seja = 3 "alarme"
      if (posicaoMenu == 3)
      {
         auxMenu++;
         if (auxMenu > 2)
            auxMenu = 1;
      }

      //caso posição menu seja = 4,5,7,8 "Grupo1, Grupo2, Nebolizacao, Evaporativa"
      if (posicaoMenu == 4 || posicaoMenu == 5 || posicaoMenu == 7 || posicaoMenu == 8)
      {
         auxMenu++;
         if (auxMenu > 4)
            auxMenu = 1;
      }
   }

   //------------------------------------

   //-----------Botão3 Sair------------------
   if (!digitalRead(bt4))
      bt4_f = 0x01;

   if (digitalRead(bt4) && bt4_f == 0x01)
   {
      bt4_f = 0x00;
      lcd.clear();
      saveEEprom();
      posicaoMenu = 1;
      posicaoSeta = 1;
   }

   //------------------------------------
   //-----------BotãoMenu------------------
   if (!digitalRead(bt6))
      bt6_f = 0x01;

   if (digitalRead(bt6) && bt6_f == 0x01)
   {
      bt6_f = 0x00;
      lcd.clear();
      posicaoMenu = 2;
      auxMenu = 0;
      printTempDisplay();
   }

   //------------------------------------
}

void setaAlarme()
{

   if (auxMenu == 1)
   {
      lcd.setCursor(0, 0);
      lcd.print("    Alarme");
      lcd.setCursor(1, 1);
      lcd.print(" Temp Maxima");
      lcd.setCursor(5, 2);
      lcd.print(TempMaxAlarme);
   }
   if (auxMenu == 2)
   {
      lcd.setCursor(0, 0);
      lcd.print("    Alarme");
      lcd.setCursor(1, 1);
      lcd.print(" Temp Minima");
      lcd.setCursor(5, 2);
      lcd.print(TempMinAlarme);
   }
}

void setaGrupo1()
{
   if (auxMenu == 1)
   {
      lcd.setCursor(0, 0);
      lcd.print("    Grupo1");
      lcd.setCursor(1, 2);
      lcd.print("Temp Liga: ");
      lcd.print(TempLigaGrupo1);
   }

   if (auxMenu == 2)
   {
      lcd.setCursor(0, 0);
      lcd.print("    Grupo1");
      lcd.setCursor(1, 2);
      lcd.print("T Desliga: ");
      lcd.print(TempDesligaGrupo1);
   }
   if (auxMenu == 3)
   {
      lcd.setCursor(0, 0);
      lcd.print("    Grupo1");
      lcd.setCursor(1, 2);
      lcd.print("Tempo Liga:");
      lcd.setCursor(4, 3);
      lcd.print(tempo_grupo1Liga);
      lcd.print("s");
   }
   if (auxMenu == 4)
   {
      lcd.setCursor(0, 0);
      lcd.print("    Grupo1");
      lcd.setCursor(1, 2);
      lcd.print("Tempo Desliga:");
      lcd.setCursor(4, 3);
      lcd.print(tempo_grupo1Desliga);
      lcd.print("s");
   }
}

void setaGrupo2()
{
   if (auxMenu == 1)
   {
      lcd.setCursor(0, 0);
      lcd.print("    Grupo2");
      lcd.setCursor(1, 2);
      lcd.print("Temp Liga: ");
      lcd.print(TempLigaGrupo2);
   }

   if (auxMenu == 2)
   {
      lcd.setCursor(0, 0);
      lcd.print("    Grupo2");
      lcd.setCursor(1, 2);
      lcd.print("T Desliga: ");
      lcd.print(TempDesligaGrupo2);
   }
   if (auxMenu == 3)
   {
      lcd.setCursor(0, 0);
      lcd.print("    Grupo2");
      lcd.setCursor(1, 2);
      lcd.print("Tempo Liga:");
      lcd.setCursor(4, 3);
      lcd.print(tempo_grupo2Liga);
      lcd.print("s");
   }
   if (auxMenu == 4)
   {
      lcd.setCursor(0, 0);
      lcd.print("    Grupo2");
      lcd.setCursor(1, 2);
      lcd.print("Tempo Desliga:");
      lcd.setCursor(4, 3);
      lcd.print(tempo_grupo2Desliga);
      lcd.print("s");
   }
}

void setaGrupo3()
{
   lcd.setCursor(0, 0);
   lcd.print("    Grupo3");
   lcd.setCursor(1, 2);
   lcd.print("T Liga: ");
   lcd.print(tempo_grupo3Liga);
   lcd.print("s");
}

void setaNebolizacao()
{
   if (auxMenu == 1)
   {
      lcd.setCursor(0, 0);
      lcd.print("  Nebolizacao ");
      lcd.setCursor(1, 2);
      lcd.print("T Liga: ");
      lcd.print(TempLigaNebolizacao);
   }
   if (auxMenu == 2)
   {
      lcd.setCursor(0, 0);
      lcd.print("  Nebolizacao ");
      lcd.setCursor(1, 2);
      lcd.print("T Desliga: ");
      lcd.print(TempDesligaNebolizacao);
   }
   if (auxMenu == 3)
   {
      lcd.setCursor(0, 0);
      lcd.print("  Nebolizacao ");
      lcd.setCursor(1, 2);
      lcd.print("Tempo Liga: ");
      lcd.print(tempo_nebolizacaoLiga);
   }
   if (auxMenu == 4)
   {
      lcd.setCursor(0, 0);
      lcd.print("  Nebolizacao ");
      lcd.setCursor(1, 2);
      lcd.print("Tempo Desliga: ");
      lcd.setCursor(4, 3);
      lcd.print(tempo_nebolizacaoDesliga);
   }
}
void setaEvaporativa()
{
    if (auxMenu == 1)
   {
      lcd.setCursor(0, 0);
      lcd.print("  Evaporativa ");
      lcd.setCursor(1, 2);
      lcd.print("T Liga: ");
      lcd.print(TempLigaEvaporativa);
   }
   if (auxMenu == 2)
   {
      lcd.setCursor(0, 0);
      lcd.print("  Evaporativa ");
      lcd.setCursor(1, 2);
      lcd.print("T Desliga: ");
      lcd.print(TempDesligaEvaporativa);
   }
   if (auxMenu == 3)
   {
      lcd.setCursor(0, 0);
      lcd.print("  Evaporativa ");
      lcd.setCursor(1, 2);
      lcd.print("Tempo Liga: ");
      lcd.print(tempo_evaporativaLiga);
   }
   if (auxMenu == 4)
   {
      lcd.setCursor(0, 0);
      lcd.print("  Evaporativa ");
      lcd.setCursor(1, 2);
      lcd.print("Tempo Desliga: ");
      lcd.print(tempo_evaporativaDesliga);
   }
}

//interrupção timer2 a cada 50ms
void temporizador()
{
   flagLerBotoes = true;

   countLerTemp++;
   if (countLerTemp == tempo_leiruraTemp) //2 segundos?
   {
      countLerTemp = 0x00;
      flag_ler_temp = true;
   }

   countTempo++;

   if (countTempo == UmSegundo) // passou 1 segundo?
   {
      countTempo = 0x00;
      count_grupo1++;
      count_grupo2++;
      count_grupo3++;
      count_nebolizacao++;
      count_evaporativa++;
      printTempDisplay();
   }

   //counterControle++;
   //if (counterControle == 2)
   //{
      flag_controle = true;
      //counterControle = 0x00;
   //}
} //fim interrupção

void saveEEprom()
{
  int t;

  for(t=0; t<4; t++)
  {
     EEPROM.write(0x00 + t  , *((char*)&TempLigaGrupo1  + t));
     EEPROM.write(0x04 + t  , *((char*)&TempDesligaGrupo1 + t));
     //EEPROM.write( 0x08 + t  , *((char*)&valor3 + t));

  }
}

void loadEEprom()
{
  int t;

  for(t=0; t<4; t++)
  {

        *((char*)&TempLigaGrupo1 + t) =  EEPROM.read(0x00 + t);
        *((char*)&TempDesligaGrupo1 + t) =  EEPROM.read(0x04 + t);
      //*((char*)&valor3 + t) =  EEPROM.read(0x08 + t);
      
  }
}
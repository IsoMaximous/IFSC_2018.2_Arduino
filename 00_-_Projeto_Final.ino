//-----------------------------------------------------------------------
//    PROJETO FINAL DO CURSO DE ARDUÍNO IFSC 2018.2
//
//ALUNOS: Guilherme Maximiliano Reichert Negri, guilhermenegri@gmail.com
//        Giovane Alexandre, giovanealexandre@hotmail.com
//
// Código do programa de Portão Eletrônico Automátio
// 
//  P - Portão
//  i - Internet
//  o - Of
//  t - Things
//  A - Automático
//
//  **********************************************
//  ********** VERSÃO PARA ARDUINO MEGA **********
//  **********************************************
//
//OBJETIVO: Controle de portão (mas poderia ser qualquer carga!)
//FUNÇÕES:
//  1-  Abrir portão
//  2-  Fechar portão
//  3-  
//
//-----------------------------------------------------------------------

// Ideias
//  1-  Criar variável para que saibamos qual foi o último movimento do portão (Abrir ou Fechar) para poder operar o portão com comando único.
//  2-  Criar função while e/ou variável que armazene estado do portão (status) para saber se está Aberto, Fechado, em Operação, impedido
//      de operar (ex.: sensor de obstáculo em operação) e enviara informações para LCD.

//-----------------------------------------------------------------------
// BIBLIOTECAS AUXILIARES
#include "Arduino.h"
#include "Button.h"
#include "LiquidCrystal.h"
#include "Ultrasonic.h"

// DEFINIÇÃO DOS PINOS

// LCD - LiquidCrystal lcd(rs, en, d4, d5, d6, d7)
#define lcdPin_RS  29
#define lcdPin_E 28
#define lcdPin_D4 24
#define lcdPin_D5 25
#define lcdPin_D6 26
#define lcdPin_D7 27

// Fim de curso - Micro Switches
#define msPinOpened 22
#define msPinClosed 23

// Relés
#define relayPinPower 30
#define relayPinGate 31

// HCSR04
#define hcPin_trig 32
#define hcPin_echo 33

// VARIÁVEIS GLOBAIS E DEFINIÇÕES
const int lcdCol = 16;  //Configuração do número de colunas do LCD
const int lcdRow =  2;  //Configuração do número de linhas do LCD
int caseVar = 0;        //Variável de controle do switch case
int msOpened = 0;       //Variável Portão Aberto
int msClosed = 0;       //Variável Portão Fechado


// INICIALIZAÇÃO DE OBJETOS
LiquidCrystal lcd(lcdPin_RS,lcdPin_E,lcdPin_D4,lcdPin_D5,lcdPin_D6,lcdPin_D7);
Button msButtonOpened(msPinOpened); //Ojbeto da biblioteca Button.h
Button msButtonClosed(msPinClosed); //Ojbeto da biblioteca Button.h
Ultrasonic ultrasonic(hcPin_trig,hcPin_echo);

 
void setup() {
  //Debug
  Serial.begin(9600);
  //
  
  lcd.begin(lcdCol, lcdRow);  //Configuração do número de colunas e linhas do LCD
  lcdWrite(2,"FIC ARDUINO",3,"IFSC-SLO");
  msButtonOpened.init();    //Inicializa botão Portão Aberto
  msButtonClosed.init();    //Inicializa botão Portão Fechado
  pinMode(relayPinPower,OUTPUT);
  pinMode(relayPinGate,OUTPUT);
  digitalWrite(relayPinPower,HIGH);  //Relé "Low level trigger", iniciar em HIGH para não atuar relé
  digitalWrite(relayPinGate,HIGH);   //Relé "Low level trigger", iniciar em HIGH para não atuar relé
}

void loop() {
  //Debug
  Serial.print("caseVar: ");
  Serial.print(caseVar);
  Serial.print(" ;msO: ");
  Serial.print(msOpened);
  Serial.print(" ;msC: ");
  Serial.println(msClosed);
  //delay(400);
  //
  msOpened = msButtonOpened.onRelease();   //Leitura do Micro Switch Aberto
  msClosed = msButtonClosed.onRelease();   //Leitura do Micro Switch Fechado

  //Le as informacoes do sensor ultrasonico em cm
  float ultraValue;
  long microsec = ultrasonic.timing();
  ultraValue = ultrasonic.convert(microsec,Ultrasonic::CM);
  //Escreve informações do sensor ultrasonico no LCD  
  lcdWrite(0,"Leitura HC-SR04",0,"Dist. = "+String(ultraValue)+"    ");

  // Le Micro Switches
  if (msOpened==1){
    caseVar = 1;
  }
  else if (msClosed==1){
    caseVar = 2;
  }
  else {
    caseVar = 0;
  }
  switch (caseVar){
    case 0:
    
    break;
    case 1:
    //Debug
    Serial.println("Case1");
    //
      gateOpened();
    break;
    case 2:
    //Debug
    Serial.println("Case2");
    //
      gateClosed();
    break;
       
  }
 

}

// Função Escreve no LCD
void lcdWrite(int lcdRow1, String lcdRow1Txt, int lcdRow2, String lcdRow2Txt){
  lcd.clear();                //Limpa display
  lcd.setCursor(lcdRow1,0);   //Posiciona cursor na linha 1
  lcd.print(lcdRow1Txt);       //Escreve na linha 1
  lcd.setCursor(lcdRow2,1);   //Posiciona cursor na linha 2
  lcd.print(lcdRow2Txt);      //Escreve na linha 2
}
// Função Abrir portão
void openGate(){
  digitalWrite(relayPinPower,LOW);
  digitalWrite(relayPinGate,LOW);
  delay(10000);
  digitalWrite(relayPinPower,HIGH);
  digitalWrite(relayPinGate,HIGH);
}
// Rotina quando portão está aberto
void gateOpened(){
  Serial.println("openGate");
  lcdWrite(1,"Portao Aberto",1,"Moendo no Void");
  openGate();
}

// Função Fehar portão
void closeGate(){
  digitalWrite(relayPinPower,LOW);
  delay(10000);
  digitalWrite(relayPinPower,HIGH);
}

// Rotina quando portão está fechado
void gateClosed(){
  lcdWrite(1,"Portao Fechado",0,"Debulhando Milho");
  closeGate();
}

//-----------------------------------------------------------------------
//UTILIDADES

//PLACA         PINOS DIGITAIS USÁVEIS PARA INTERRUPÇÃO
//Uno, Nano     2,3
//Mega          2, 3, 18, 19, 20, 21

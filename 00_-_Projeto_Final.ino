//-----------------------------------------------------------------------
//    PROJETO FINAL DO CURSO DE ARDUÍNO IFSC 2018.2
//
//ALUNOS: Guilherme Maximiliano Reichert Negri, guilhermenegri@gmail.com
//        Giovane Alexandre Werlang, giovanealexandre@hotmail.com
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
#include "IRremote.h"

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

//Infra Vermelho
#define irPin 34

// VARIÁVEIS GLOBAIS E DEFINIÇÕES
const int lcdCol = 16;  //Configuração do número de colunas do LCD
const int lcdRow =  2;  //Configuração do número de linhas do LCD
int caseVar = 0;        //Variável de controle do switch case
int msOpened = 0;       //Variável Portão Aberto
int msClosed = 0;       //Variável Portão Fechado
int on = 0;                     //IR var
unsigned long last = millis();  //IR var

// INICIALIZAÇÃO DE OBJETOS
LiquidCrystal lcd(lcdPin_RS,lcdPin_E,lcdPin_D4,lcdPin_D5,lcdPin_D6,lcdPin_D7);
Button msButtonOpened(msPinOpened); //Ojbeto da biblioteca Button.h
Button msButtonClosed(msPinClosed); //Ojbeto da biblioteca Button.h
Ultrasonic ultrasonic(hcPin_trig,hcPin_echo);
IRrecv irValue(irPin);
decode_results irResults;
 
void setup() {
  //Debug
  Serial.begin(9600);
  //
  
  lcd.begin(lcdCol, lcdRow);  //Configuração do número de colunas e linhas do LCD
  lcdWrite(2,"FIC ARDUINO",3,"Smart Gate");
  msButtonOpened.init();    //Inicializa botão Portão Aberto
  msButtonClosed.init();    //Inicializa botão Portão Fechado
  pinMode(relayPinPower,OUTPUT);
  pinMode(relayPinGate,OUTPUT);
  digitalWrite(relayPinPower,HIGH);  //Relé "Low level trigger", iniciar em HIGH para não atuar relé
  digitalWrite(relayPinGate,HIGH);   //Relé "Low level trigger", iniciar em HIGH para não atuar relé
  pinMode(irPin,INPUT);   //Inicializa Infra Vermelho
  pinMode(13, OUTPUT);
  irValue.enableIRIn();   //Inicializa o receiver
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

  irRead();
  msRead();
   
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
// Função Lê Infra Vermelho
void irRead() {
  if (irValue.decode(&irResults)) {
    // If it's been at least 1/4 second since the last
    // IR received, toggle the relay
    if (millis() - last > 250) {
      on = !on;
      digitalWrite(13, on ? HIGH : LOW);
    }
    last = millis();      
    irValue.resume(); // Receive the next value
  }
}
// Função Lê Micro Switches
void msRead() {
  msOpened = msButtonOpened.onRelease();   //Leitura do Micro Switch Aberto
  msClosed = msButtonClosed.onRelease();   //Leitura do Micro Switch Fechado

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
  for (int i=0; i <= 100; i++){
    //Le as informacoes do sensor ultrasonico em cm
    float hcValue;          //Variável que armazena valor lido pelo HC-SR04
    long microsec = ultrasonic.timing();
    hcValue = ultrasonic.convert(microsec,Ultrasonic::CM);
    //Escreve informações do sensor ultrasonico no LCD  
    lcdWrite(0,"Leitura HC-SR04",0,"Dist. = "+String(hcValue)+"    ");
    if (hcValue<100){
      openGate();
      break;
      }
    else {
      delay(10);
    }
  }
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

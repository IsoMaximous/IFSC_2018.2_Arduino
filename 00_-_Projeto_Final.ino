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
#include "LiquidCrystal.h"
#include "Ultrasonic.h"
#include "IRremote.h"
#include "RCSwitch.h"
#include "LinkedList.h"
#include "EEPROM.h"

// DEFINIÇÃO DOS PINOS

// a) LCD - LiquidCrystal lcd(rs, en, d4, d5, d6, d7)
#define lcdPin_RS  29
#define lcdPin_E 28
#define lcdPin_D4 24
#define lcdPin_D5 25
#define lcdPin_D6 26
#define lcdPin_D7 27

// b) Fim de curso - Micro Switches
#define msPinOpened 22
#define msPinClosed 23

// c) Relés
#define relayPinPower 30
#define relayPinGate 31

// d) HCSR04
#define hcPin_trig 32
#define hcPin_echo 33

// e) Infra Vermelho
#define IRrecvPin 34

// VARIÁVEIS GLOBAIS E DEFINIÇÕES
const int lcdCol = 16;                    //CONSTANTE Configuração do número de colunas do LCD
const int lcdRow =  2;                    //CONSTANTE Configuração do número de linhas do LCD
const int lcdAnimaConst = 15;             //CONSTANTE Configuração para regular velocidade de troca de tela no LCD
const int eeAddressRFListSize = 0;        //Posição na memória EEPROM que armazena tamanho da lista de controles RF cadastrados
int eeAddressRFFirstRegister = 1;         //Posição na memória EEPROM que armazena primeiro controle RF cadastrado
int rfListSize = 0;                       //Variável Tamanho da lista de controles RF cadastrados
int lcdChange = 0;                        //CONTADOR para alterar mensagem no LCD
int msOpenedStatus = 0;                   //Variável Status do micro switch Portão Aberto
int msClosedStatus = 0;                   //Variável Status do micro switch Portão Fechado
int gateLast = 0;                         //Variável com a última operação do portão (abrir ou fechar)
unsigned long gateOperateLast = millis(); //Variável com tempo inicial para IRrecev
unsigned long lcdLast = millis();         //Variável com tempo inicial para LCD Anima
unsigned long gateAutoCloseLast = 0;      //Variável com tempo inicial para gateAutoClose

// Variáveis Customizáveis
const int delayToAutoCloseGate = 5;       //CONSTANTE Configuração do tempo de espera para fechamento automático, em segundos
bool gateAutoCloseOn = 1;                 //Variável boleana gateAutoClose ( ON=1; OFF=0 )

// INICIALIZAÇÃO DE OBJETOS
LiquidCrystal lcd(lcdPin_RS,lcdPin_E,lcdPin_D4,lcdPin_D5,lcdPin_D6,lcdPin_D7);
Ultrasonic ultrasonic(hcPin_trig,hcPin_echo);
IRrecv IRrecvSignal(IRrecvPin);   //Objeto para IRrecv
decode_results IRrecvResults;
RCSwitch rfSignal = RCSwitch();   //Objeto para RF
LinkedList<unsigned long> rfList = LinkedList<unsigned long>();   //Lista de códigos salvos pelo portão
 
void setup() {
  //Debug
  Serial.begin(9600);
  //
  
  lcd.begin(lcdCol, lcdRow);        //Configuração do número de colunas e linhas do LCD
  pinMode(msPinOpened,INPUT);       //Configuração do pino do micro switch "Portão Aberto"
  pinMode(msPinClosed,INPUT);       //Configuração do pino do micro switch "Portão Fechado"
  pinMode(relayPinPower,OUTPUT);    //Configuração do pino do relé de alimentação "Power"
  pinMode(relayPinGate,OUTPUT);     //Configuração do pino do relé de direção (abre/fecha) "Gate"
  digitalWrite(relayPinPower,HIGH); //Relé "Low level trigger", iniciar em HIGH para não atuar relé
  digitalWrite(relayPinGate,HIGH);  //Relé "Low level trigger", iniciar em HIGH para não atuar relé
  pinMode(IRrecvPin,INPUT);         //Inicializa Infra Vermelho
  IRrecvSignal.enableIRIn();        //Inicializa o receiver
  rfSignal.enableReceive(0);        //Recebe na interrupt 0 => que é o Pin #2
  eepromLoad();                   
      
}

void loop() {

  msRead();
  rfRead();
  IRrecvRead();
  lcdAnima();
  gateAutoClose(delayToAutoCloseGate); 
}

// FUNÇÕES E SUBROTINAS

// 01 - Escreve no LCD
void lcdWrite(int lcdRow1, String lcdRow1Txt, int lcdRow2, String lcdRow2Txt,bool lcdClear){
  if (lcdClear) { lcd.clear(); }  //Limpa display
  lcd.setCursor(lcdRow1,0);       //Posiciona cursor na linha 1
  lcd.print(lcdRow1Txt);          //Escreve na linha 1
  lcd.setCursor(lcdRow2,1);       //Posiciona cursor na linha 2
  lcd.print(lcdRow2Txt);          //Escreve na linha 2
}

// 02 - Animação do LCD enquando nada acontece
void lcdAnima() {
  switch (lcdChange){
    case 0:   lcdWrite(2,"FIC ARDUINO",2,"PROJETO FINAL",1);          break;
    case 100: lcdWrite(0,"Controles Salvos",7,String(rfListSize),1);        break;
    case 250: lcdWrite(3,"SMART GATE",4," (-_-) ",1);                 break;
    case 290: lcdWrite(3,"SMART GATE",4," (o_o) ",0);                 break;
    case 320: lcdWrite(3,"SMART GATE",4," (-_-) ",0);                 break;
    case 340: lcdWrite(3,"SMART GATE",4," (0_0) ",0);                 break;
    case 370: lcdWrite(3,"SMART GATE",4,"_(0_0)_",0);                 break;
    case 390: lcdWrite(3,"SMART GATE",4,"|(0_0)_",0);                 break;
    case 410: lcdWrite(3,"SMART GATE",4,"/(0_0)_",0);                 break;
    case 430: lcdWrite(3,"SMART GATE",4,"|(0_0)_",0);                 break;
    case 450: lcdWrite(3,"SMART GATE",4,"/(0_0)_",0);                 break;
    case 470: lcdWrite(3,"SMART GATE",4,"_(0_0)_",0);                 break;
    case 500: lcdWrite(0,"Guilherme Negri",0,"Giovane Werlang",1);    break;
    case 800: lcdChange = -1;                                         break;
  }
  if (millis()-lcdLast > lcdAnimaConst) {
    lcdChange++;
    lcdLast=millis();
  }
}
// 03 - Operação lógica do portão
void gateOperate(){
  if (millis() - gateOperateLast > 250) {
      if      ((msClosedStatus==HIGH)&(msOpenedStatus==LOW )) { openGate(); }
      else if ((msClosedStatus==LOW )&(msOpenedStatus==HIGH)) { closeGate();}
      else {lcdWrite(2,"YOU GOT ME!",2,"What to do?",1); delay(1000); lcdChange = 0;} /* qual ação aqui? */
   }
   gateOperateLast = millis();
}
// 04 - Abrir portão
void openGate(){
  digitalWrite(relayPinPower,LOW);  //Aciona relé Power (LOW -> Relé Low Level Trigger)
  digitalWrite(relayPinGate,LOW);   //Aciona relé de direção (LOW -> Relé Low Level Trigger)
  gateLast = 1;                     //Armazena que última operação foi "Abrir"
  while (msOpenedStatus==LOW){
    msRead();
    lcdAnima();
  }
  gateOpened();
}
// 05 - Fehar portão
void closeGate(){
  digitalWrite(relayPinPower,LOW);  //Aciona relé Power (LOW -> Relé Low Level Trigger)
  gateLast = 0;                     //Armazena que última operação foi "Abrir"
  while (msClosedStatus==LOW){
    msRead();
    if (gateAntiCrush()==1){
      openGate();
      break;
    }  
  }
  gateClosed();
}
// 06 - Portão abriu com sucesso
void gateOpened(){
  digitalWrite(relayPinPower,HIGH);
  digitalWrite(relayPinGate,HIGH);
  lcdWrite(1,"Portao ABERTO",2,"com sucesso!",1);
  delay(2000);
  lcdChange = 0;  //Reinicia mensagem no LCD
  gateAutoCloseLast = millis();
}
// 07 - Portão fechou com sucesso
void gateClosed(){
  digitalWrite(relayPinPower,HIGH);
  lcdWrite(1,"Portao FECHADO",2,"com sucesso!",1);
  delay(1000);
  lcdChange = 0;  //Reinicia mensagem no LCD
}
// 08 - Leitura dos Micro Switches de fim de curso
void msRead() {
  msOpenedStatus = !digitalRead(msPinOpened); //Leitura do micro switch Portão Aberto
  msClosedStatus = !digitalRead(msPinClosed); //Leitura do micro switch Portão Fechado
}
// 09 - Leitura de RF - Controles remotos 433Mhz
void rfRead() {
   if (rfSignal.available()) {
    rfListSize = rfList.size();
    for (int i = 0; i < rfListSize; i++) {
      Serial.println(rfList.get(i));
      if (rfSignal.getReceivedValue() == rfList.get(i)){
        gateOperate();
        break;
      } else {lcdWrite(0,"Nao encontrado:",3,String(rfSignal.getReceivedValue()),1); }
    }
    rfSignal.resetAvailable();  //Receber próximo sinal
    /*
    Serial.print("Received ");
    Serial.print( rfSignal.getReceivedValue() );
    Serial.print(":");
    Serial.print( rfList.get(0) );
    Serial.print(" / ");
    Serial.print( rfSignal.getReceivedBitlength() );
    Serial.print("bit ");
    Serial.print("Protocol: ");
    Serial.print( rfSignal.getReceivedProtocol() );
    */
  }
}
// Função Carrega dados da EEPROM
void eepromLoad(){
  EEPROM.get(eeAddressRFListSize,rfListSize);
  for(int i=0;i<rfListSize;i++){
    unsigned long rfTemp = 0;
    EEPROM.get(eeAddressRFFirstRegister, rfTemp);
    rfList.add(rfTemp);
    eeAddressRFFirstRegister += sizeof(unsigned long);
  }

  Serial.print(sizeof(unsigned long));
  Serial.print(" / rfList: ");
  Serial.print(rfList.size());
  Serial.print(" / EEPROM List: ");
  Serial.println(rfListSize);

  for (int i=0; i<rfList.size();i++) {
    Serial.println(rfList.get(i));
  }
}

// Função Lê Infra Vermelho
void IRrecvRead() {
  if (IRrecvSignal.decode(&IRrecvResults)) {
    gateOperate();      
    IRrecvSignal.resume();   //Receber próximo sinal
  }
}

// Função Salva rfList na EEPROM
void rfListSave(){
  rfListSize = rfList.size();
  EEPROM.put(eeAddressRFListSize,rfListSize);
  for(int i=0;i<rfListSize;i++){
    unsigned long rfTemp = 0;
    rfTemp = rfList.get(i);
    EEPROM.put(eeAddressRFFirstRegister, rfTemp);
    eeAddressRFFirstRegister += sizeof(unsigned long);
  }
}
// Função Fechamento Automático
void gateAutoClose(int delayToClose){
  if ((gateAutoCloseOn)&(msOpenedStatus==HIGH)){
    if (millis()-gateAutoCloseLast > delayToClose*1000) {
      closeGate();
    }
  }
}
// Função Anti Esmagamento do portão
// durante o fechamento verifica se existe obstáculo no caminho
// caso positivo, retorna 1, caso negativo, retorna 0
int gateAntiCrush() {
    //Le as informacoes do sensor ultrasonico em cm
    float hcValue;                                          //Variável que armazena valor lido pelo HC-SR04
    long microsec = ultrasonic.timing();
    hcValue = ultrasonic.convert(microsec,Ultrasonic::CM);
    if (hcValue < 100) { return 1; }
      
    lcdWrite(0,"Leitura HC-SR04",0,"Dist. = "+String(hcValue)+"    ",0);  //Escreve informações do sensor ultrasonico no LCD
}


//-----------------------------------------------------------------------
//UTILIDADES

//PLACA         PINOS DIGITAIS USÁVEIS PARA INTERRUPÇÃO
//Uno, Nano     2,3
//Mega          2, 3, 18, 19, 20, 21

// Controle NS: HU0G0607744EK (Loja)
//  On/Of:  144390453
//  A:      144390437
//  B:      144390421

// Controle Unisystem (barracão da Ro)
//  Triang: 136001445
//  Bola:   136001429
//  Quadr:  136001461

//ESP
// RX - LV1
// TX - LV2
// EN - LV3
// RST- LV4

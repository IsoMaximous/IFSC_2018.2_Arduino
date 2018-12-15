/*-----------------------------------------------------------------------
 *      PROJETO FINAL DO CURSO DE ARDUÍNO IFSC 2018.2
 *      
 * ALUNOS:  Guilherme Maximiliano Reichert Negri      guilhermenegri@gmail.com
 *          Giovane Alexandre Werlang                 giovanealexandre@hotmail.com
 *          
 * OBJETIVO:  Controle de portão eletrônico (mas poderia ser qualquer carga!)
 * MOTIVAÇÃO: Moro em um condomínio com sala comercial no térreo. Durante o horário comercial o portão fica
 *            aberto para os clientes da sala comercial usarem as garagens, fora do horário comercial o portão
 *            deve ficar fechado. O problema é que o portão não sabe quando é horário comercial, e não há forma
 *            fácil de habilitar o fechamento automático. O PIOTA veio para resolver esse problemas e adicionar
 *            funções ao simples portão eletrônico tradicional.
 * 
 * Faça o download do código em: https://github.com/IsoMaximous/IFSC_2018.2_Arduino
 * 
 * Código do programa de Portão Eletrônico Automátio
 * P - Portão
 *  i - Internet
 *  o - Of
 *  t - Things
 * A - Automático
 * 
 *   **********************************************
 *   ********** VERSÃO PARA ARDUINO MEGA **********
 *   **********************************************
 *   
 * MELHORIAS A IMPLEMENTAR:
 * 1 - Gravação de novo controle na memória EEPROM
 * 2 - Conexão com internet via ESP 8266 e protocolo MQTT
 * 3 - Implementar Photo Resistor
 * 4 - Operar settings via controle IR
 * 5 - Implementar barreia IR e remover Ultrasonic
 *-----------------------------------------------------------------------*/

// BIBLIOTECAS AUXILIARES
#include "Arduino.h"          //Biblioteca nativa
#include "LiquidCrystal.h"    //Biblioteca nativa para utilizar LCD 16x2
#include "Ultrasonic.h"       //Biblioteca para sensor ultrassônico HC-SR04
#include "IRremote.h"         //Biblioteca para utilizar sensor infra vermelho (Somente receptor nesta fase do porjeto)
#include "RCSwitch.h"         //Biblioteca para utilizar receptor RF 433Mhz, mesmo que controles remotos padrão de portão eletrônico
#include "LinkedList.h"       //Biblioteca com funções para manipular listas. (Somente testes nesta fase do porjeto)
#include "EEPROM.h"           //Biblioteca com funções de escrita e leitura da EEPROM. Os códigos dos controles remotos utilizados fora salvos na EEPROM.
/*OBS: A biblioteca IRremote.h utiliza o timer 1, assim como a função nativa tone(), o que gera conflito e não compila o código.
 * Portanto, é necessário alterar qual timer o IRremote utiliza, no arquivo boarddefs.h linha 82 para ATmega2560 */
 
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

// f) Buzzer
#define sirenPin0 3
#define sirenPin1 36
#define sirenPin2 37

// g) Photo resistor, para acionar o motor ao escurecer (Não implementado nesta fase do projeto)

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
unsigned long sirenLast = 0;              //Variável com tempo inicial para siren
bool sirenChange = 0;                     //Variável que muda entre 2 estados da sirene (frequência e cor de led)
bool sirenOn = 1;                         //Variável boleana que liga/desliga função siren ( ON=1; OFF=0 )
int gac =0;                               //Variável intermediária que dispara a função gateAutoClose() quando atinge tempo limite definodo por delayToAutoCloseGate
unsigned int sirenFreq1 = 800;            //Variável usada na função Tone()para sirene
unsigned int sirenFreq2 = 1500;           //Variável usada na função Tone()para sirene
unsigned int sirenDelayToChange = 1000;   //CONSTANTE tempo entre trocas da sirene
char setValue = 0;                        //Variável para Switch Case dos Ajustes via terminal
char setValue1 = 0;                       //Variável para Switch Case dos Ajustes via bluetooth (ligado em Tx1 e Rx1 do Mega)

// Variáveis Customizáveis
const int delayToAutoCloseGate = 5;       //CONSTANTE Configuração do tempo de espera para fechamento automático, em segundos
bool gateAutoCloseOn = 1;                 //Variável boleana que liga/desliga função gateAutoClose ( ON=1; OFF=0 )

// INICIALIZAÇÃO DE OBJETOS
LiquidCrystal lcd(lcdPin_RS,lcdPin_E,lcdPin_D4,lcdPin_D5,lcdPin_D6,lcdPin_D7);  //Inicializa LCD
Ultrasonic ultrasonic(hcPin_trig,hcPin_echo);                                   //Inicializa HC-SR04
IRrecv IRrecvSignal(IRrecvPin);                                                 //Inicializa receptor IRrecv
decode_results IRrecvResults;                                                   //Converte valores lidos pelo receptor IRrecv
RCSwitch rfSignal = RCSwitch();                                                 //Inicializa receptor RF
LinkedList<unsigned long> rfList = LinkedList<unsigned long>();                 //Lista de códigos de controle salvos pelo portão
 
void setup() {
  Serial.begin(9600);               //Inicializa terminal
  Serial1.begin(9600);              //Inicializa bluetooth
   
  lcd.begin(lcdCol, lcdRow);        //Configuração do número de colunas e linhas do LCD
  pinMode(msPinOpened,INPUT);       //Configuração do pino do micro switch "Portão Aberto"
  pinMode(msPinClosed,INPUT);       //Configuração do pino do micro switch "Portão Fechado"
  pinMode(relayPinPower,OUTPUT);    //Configuração do pino do relé de alimentação "Power"
  pinMode(relayPinGate,OUTPUT);     //Configuração do pino do relé de direção (abre/fecha) "Gate"
  digitalWrite(relayPinPower,HIGH); //Relé "Low level trigger", iniciar em HIGH para não atuar relé
  digitalWrite(relayPinGate,HIGH);  //Relé "Low level trigger", iniciar em HIGH para não atuar relé
  pinMode(IRrecvPin,INPUT);         //Configuração do pino do Infra Vermelho
  pinMode(sirenPin0,OUTPUT);        //Configuração do pino da sirene - Buzzer
  pinMode(sirenPin1,OUTPUT);        //Configuração do pino da sirene - LED 1
  pinMode(sirenPin2,OUTPUT);        //Configuração do pino da sirene - LED 2
  IRrecvSignal.enableIRIn();        //Inicializa o receiver
  rfSignal.enableReceive(0);        //Recebe na interrupt 0 => que é o Pin #2
  eepromLoad();                     //Função que carrega lista de códigos de controle salvos na EEPROM                      
}

void loop() {

  msRead();       //Lê status das chaves de fim de curso
  rfRead();       //Verifica se recebeu sinal de RF
  IRrecvRead();   //Verifica se recebeu sinal de IR
  lcdAnima();     //Animação do LCD (Opcional)
  gateAutoClose(delayToAutoCloseGate);    //Verifica fechamento automáico do portão
  settings();     //Verifica se recebeu instruções para alterar configurações customizáveis do portão
    
}

// FUNÇÕES E SUBROTINAS

// 01 - Escreve no LCD
// Função interna criada para simplificar a leitura do código quando enviado comando de escrever no LCD
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
/*
 * Ao invés de usar delay(), conto o tempo com millis(), o que deixa o código mais flúido
 */
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
  digitalWrite(relayPinPower,HIGH); //Aciona relé Power (LOW -> Relé Low Level Trigger)
  delay(50);                        //Delay adicionado para coordenar a operação dos relés, evitando colamento de contato
  digitalWrite(relayPinGate,LOW);   //Aciona relé de direção (LOW -> Relé Low Level Trigger)
  delay(50);                        //Delay adicionado para coordenar a operação dos relés, evitando colamento de contato
  digitalWrite(relayPinPower,LOW);  //Aciona relé Power (LOW -> Relé Low Level Trigger)
  gateLast = 1;                     //Armazena que última operação foi "Abrir"
  while (msOpenedStatus==LOW){
    siren(); //Deve estar dentro do while, concebida assim
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
    siren(); //Deve estar dentro do while, concebida assim
    msRead();
    gateAntiCrush();
    if (gac==1){
      gac=0;
      openGate();
      break;
    }  
  }
  gateClosed();
}
// 06 - Portão abriu com sucesso
void gateOpened(){
  digitalWrite(relayPinPower,HIGH);
  delay(50);                       //Delay adicionado para coordenar a operação dos relés, evitando colamento de contato
  digitalWrite(relayPinGate,HIGH);
  digitalWrite(sirenPin1,LOW);     //Desliga LEDs da sirene
  digitalWrite(sirenPin2,LOW);     //Desliga LEDs da sirene
  noTone(sirenPin0);               //Desliga Buzzer da sirene
  lcdWrite(1,"Portao ABERTO",2,"com sucesso!",1);
  delay(2000);    //delay adicionado apenas para dar tempo de ler no LCD
  lcdChange = 0;  //Reinicia mensagem no LCD
  gateAutoCloseLast = millis();
}
// 07 - Portão fechou com sucesso
void gateClosed(){
  digitalWrite(relayPinPower,HIGH); //Desliga o motor do portão
  digitalWrite(sirenPin1,LOW);      //Desliga LEDs da sirene
  digitalWrite(sirenPin2,LOW);      //Desliga LEDs da sirene
  noTone(sirenPin0);                //Desliga Buzzer da sirene
  lcdWrite(1,"Portao FECHADO",2,"com sucesso!",1);
  delay(1000);    //delay adicionado apenas para dar tempo de ler no LCD
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
    Serial.println("rfRead entrou");
    rfListSize = rfList.size();
    for (int i = 0; i < rfListSize; i++) {
      if (rfSignal.getReceivedValue() == rfList.get(i)){
        gateOperate();
        break;
      } else {lcdWrite(0,"Nao encontrado:",3,String(rfSignal.getReceivedValue()),1); }
    }
    rfSignal.resetAvailable();  //Receber próximo sinal

/* DEBUG
 *  Serial.print("Received ");
 *  Serial.print( rfSignal.getReceivedValue() );
 *  Serial.print(":");
 *  Serial.print( rfList.get(0) );
 *  Serial.print(" / ");
 *  Serial.print( rfSignal.getReceivedBitlength() );
 *  Serial.print("bit ");
 *  Serial.print("Protocol: ");
 *  Serial.print( rfSignal.getReceivedProtocol() );
 */
  }
}
// 10 - Leitura de Infra Vermelho (IR)
/*NOTA: O objetivo original era utilizar o IR como barreira na função antiCrush. Não foi implementado nesta versão.
 * Potencial: Utilizar o IR para ajustar settings() da segunte forma: 
 * 1 - Quando em operação normal, o portão usa o receptor IR para barreira.
 * 2 - Quanto em operação de ajustes - settings() - , desabilita emissor IR da barreira e o receptor IR lê controle remoto. 
 */
void IRrecvRead() {
  if (IRrecvSignal.decode(&IRrecvResults)) {
    gateOperate();      
    IRrecvSignal.resume();   //Receber próximo sinal
  }
}
// 11 - Fechamento Automático
void gateAutoClose(int delayToClose){
  if ((gateAutoCloseOn)&(msOpenedStatus==HIGH)){
    if (millis()-gateAutoCloseLast > delayToClose*1000) {
      closeGate();
    }
  }
}
// 12 - Anti Esmagamento
/*Durante o fechamento verifica se existe obstáculo no caminho
 * caso positivo, gac 1, caso negativo, gac 0
 * 
 * NOTA: Originalmente ao invés de usar a variável gac foi utilizado return
 * No início funcionou, mas depois parou de funcionar. Por falta de tempo, não foi debugado
 */ 
void gateAntiCrush() {
    //Le as informacoes do sensor ultrasonico em cm
    float hcValue;                                          //Variável que armazena valor lido pelo HC-SR04
    long microsec = ultrasonic.timing();
    hcValue = ultrasonic.convert(microsec,Ultrasonic::CM);
    if (hcValue < 30) { gac = 1; } //return 1};
      
    lcdWrite(0,"Leitura HC-SR04",0,"Dist. = "+String(hcValue)+"    ",0);  //Escreve informações do sensor ultrasonico no LCD
}
// 13 - Sirene durante acionamento do portão
/*  Da forma como está escrito o código, a função siren() deve ser chada dentro de algum tipo de laço (for, while, etc...)
 *  a função por si só não tem nenhum recurso de laço.
 */
void siren(){
  if((sirenOn==1)&&(millis()-sirenLast>sirenDelayToChange)){
    sirenChange=!sirenChange;   //muda braço do if/else
    sirenLast=millis();         //reseta a variável sirenLast
    if (sirenChange) {
      digitalWrite(sirenPin1,HIGH);
      digitalWrite(sirenPin2,LOW);
      tone(sirenPin0, sirenFreq1);
    } else {
      digitalWrite(sirenPin1,LOW);
      digitalWrite(sirenPin2,HIGH);
      tone(sirenPin0, sirenFreq1); 
    }
  }
}

// 14 - Ajustes feitos por comandos externos (Bluetooth, Wifi, terminal...)
/*NOTA: Somente alguns parâmetros simples foram implementados
 * Mas pode-se também altrar valores numéricos como tempo do delayToAutoClose, sirenFreq1 e 2, sirenDelayToChange
 */
void settings() {
  setValue=Serial.read();
  setValue1=Serial1.read();
  if((setValue=='o')||(setValue1=='o')){ //Caso tenha outras fontes de alteração, adicionar condição
    setValue = 0; 
    lcdWrite(3,"Settings",4,"Operate",1);  Serial.println("openGate = "+String(setValue));    
    delay(1000); //delay adicionado apenas para dar tempo de ler no LCD
    gateOperate();  
  } else if ((setValue=='s')||(setValue1=='s')){
    setValue = 0;
    lcdWrite(0,"Change Settings",3,"Siren = "+String(!sirenOn),1);  Serial.println("Siren = "+String(setValue));
    delay(1000); //delay adicionado apenas para dar tempo de ler no LCD
    sirenOn=!sirenOn;                     
  } else if ((setValue=='a')||(setValue1=='a')){
    setValue = 0;
    lcdWrite(0,"Change Settings",1,"Auto Close = "+String(!gateAutoCloseOn),1); Serial.println("AutoClose = "+String(setValue)); 
    delay(1000); //delay adicionado apenas para dar tempo de ler no LCD
    gateAutoCloseOn=!gateAutoCloseOn;   
  }
}
// 15 - Carrega dados da EEPROM
/* Nesta versão, carrega apenas lista de controles e tamanho da lista, mas pode carregar todos os dados customizáveis!
 */
void eepromLoad(){
  EEPROM.get(eeAddressRFListSize,rfListSize);
  for(int i=0;i<rfListSize;i++){
    unsigned long rfTemp = 0;
    EEPROM.get(eeAddressRFFirstRegister, rfTemp);
    rfList.add(rfTemp);
    eeAddressRFFirstRegister += sizeof(unsigned long);
  }

/* DEBUG
 * Serial.print(sizeof(unsigned long));
 * Serial.print(" / rfList: ");
 * Serial.print(rfList.size());
 * Serial.print(" / EEPROM List: ");
 * Serial.println(rfListSize);
 * for (int i=0; i<rfList.size();i++) { Serial.println(rfList.get(i)); }
 */
  
}
// Função Salva rfList na EEPROM (Não implementada nesta versão)
/* OBJETIVO: Reconhecer o código do controle, verificar se já está na memória, adicionar na lista e armazenar lista na EEPROM
 */
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
/* NOTAS FINAIS:
 *  O objetivo principal deste projeto é criar um portão eletrônico conectado à internet para que possa ser operado e consultado via internet.
 *  O protocolo de comunicação provapelmente será MQTT, pode-se utilizar um Raspberry como server e algo como IBM Watson
 *  O módilo Wifi ESP 8266 não funcionou, problemas para regravar o firmware.
 *  
 * PROBLEMAS ENFRENTADOS:
 *  1 - O motor elétrico gera um campo magnético forte o suficiente para causar interferência no funcionamento do LCD.
 *      Não tive evidências de que prejudica o funionamento do microprocessador.
 *        *Nota interferência: No início tentei utilizar um dimmer de chuveiro para reduzir a velocidade do motor, e nesta
 *         situação houve interferências relevantes para o funcionamento do portão, portanto foi retirado.
 *  2 - O acionamento do portão é feito através de relés. Durante a prototipagem, um relé "colou" os contator, ficando sempre
 *      no mesmo estado independentemente do comando recebido. Isso ocorreu provavelmente devido ao faiscamento interno no 
 *      momento de reverão (função antiCrush), por esse motivo os relés foram coordenados com delay de 50ms.
 */

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
/*OBS: A biblioteca IRremote.h utiliza o timer 1, assim como a função nativa tone(), o que gera conflito e não compila o código.
 * Portanto, é necessário alterar qual timer o IRremote utiliza, no arquivo boarddefs.h linha 82 para ATmega2560 */
 
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

#include "dht.h" // biblioteca DO SENSOR DHT11
#include <Buzzer.h>
#include <stdlib.h>
#include <SPI.h>
#include <SD.h>
#include "WiFiEsp.h" //INCLUSÃO DA BIBLIOTECA
#include "SoftwareSerial.h"//INCLUSÃO DA BIBLIOTECA

//sensor dht11
#define pinoDHT11 A5 //PINO ANALÓGICO UTILIZADO PELO DHT11
//buzzer
#define  pinoBuzzer 5
// Pino ligado ao CS do modulo
#define chipSelect 4
//valvula
#define pin_valvula  8
//luz
#define pin_luz  9
// sensor solo
#define sensor_solo  A4

Buzzer buzzer(pinoBuzzer);

dht DHT; //VARIÁVEL DO TIPO DHT

//Variaveis para cartao SD
String parameter;
byte line;
String configs[8];


//parametros
int8_t humidadeMinimaSolo = 0;
String ssid = "", password = "";
byte ip[4];
bool modoAuto = false;
int8_t luzStatus = 0;

int8_t  analogSoloSeco = 400; //VALOR MEDIDO COM O SOLO SECO (VOCÊ PODE FAZER TESTES E AJUSTAR ESTE VALOR)
int8_t  analogSoloMolhado = 150; //VALOR MEDIDO COM O SOLO MOLHADO (VOCÊ PODE FAZER TESTES E AJUSTAR ESTE VALOR)
int8_t percSoloSeco = 0; //MENOR PERCENTUAL DO SOLO SECO (0% - NÃO ALTERAR)
int8_t percSoloMolhado = 100;

unsigned long millisTarefa = millis(); 

//wifi
SoftwareSerial Serial1(6, 7);
int status = WL_IDLE_STATUS; //STATUS TEMPORÁRIO ATRIBUÍDO QUANDO O WIFI É INICIALIZADO E PERMANECE ATIVO
//ATÉ QUE O NÚMERO DE TENTATIVAS EXPIRE (RESULTANDO EM WL_NO_SHIELD) OU QUE UMA CONEXÃO SEJA ESTABELECIDA
//(RESULTANDO EM WL_CONNECTED)

WiFiEspServer server(1150); 

RingBuffer buf(8);

void setup() {

  Serial.begin(9600); //INICIALIZA A SERIAL
  delay(2000); //INTERVALO DE 2 SEGUNDO ANTES DE INICIAR


  while (!Serial);
  Serial.println(F("Initializing SD card..."));
  if (!SD.begin(chipSelect))
  {
    Serial.println(F("Card failed, or not present"));
    errorInicializacao(3, 200, 500);
  }
  Serial.println(F("card initialized."));
  File myFile = SD.open("CONFIG.txt");
  if (myFile)
  {
    while (myFile.available())
    {
      char c = myFile.read();
      if (isPrintable(c))
      {
        parameter.concat(c);
      }
      else if (c == '\n')
      {
        Serial.println(parameter);
        configs[line] = parameter;
        parameter = "";
        line++;
      }
    }
  }

  myFile.close();
  configParametros();


  pinMode(pin_valvula, OUTPUT);
  pinMode(pin_luz, OUTPUT);
  digitalWrite(pin_valvula, HIGH);
  digitalWrite(pin_luz, HIGH);
  pinMode(sensor_solo, INPUT);


  // put your setup code here, to run once:
  Serial1.begin(9600); //INICIALIZA A SERIAL PARA O ESP8266
  WiFi.init(&Serial1); //INICIALIZA A COMUNICAÇÃO SERIAL COM O ESP8266
  WiFi.config(IPAddress(ip)); //COLOQUE UMA FAIXA DE IP DISPONÍVEL DO SEU ROTEADOR
  //INÍCIO - VERIFICA SE O ESP8266 ESTÁ CONECTADO AO ARDUINO, CONECTA A REDE SEM FIO E INICIA O WEBSERVER
  if (WiFi.status() == WL_NO_SHIELD) {
    errorInicializacao(3, 200, 500);
  }
  byte cont = 0;
  while (status != WL_CONNECTED) {
    if (cont > 3) {
      errorInicializacao(3, 200, 500);
    }

    status = WiFi.begin(ssid.c_str(), password.c_str());
    cont++;
  }

  server.begin();
  //quando está tudo certo
  musica();

}

void loop() {
  WiFiEspClient client = server.available(); //ATENDE AS SOLICITAÇÕES DO CLIENTE
  if (client) { //SE CLIENTE TENTAR SE CONECTAR, FAZ

    buf.init(); //INICIALIZA O BUFFER
    while (client.connected()) { //ENQUANTO O CLIENTE ESTIVER CONECTADO, FAZ
      if (client.available()) { //SE EXISTIR REQUISIÇÃO DO CLIENTE, FAZ
        char c = client.read(); //LÊ A REQUISIÇÃO DO CLIENTE
        buf.push(c); //BUFFER ARMAZENA A REQUISIÇÃO

        //IDENTIFICA O FIM DA REQUISIÇÃO HTTP E ENVIA UMA RESPOSTA
        if (buf.endsWith("\r\n\r\n")) {
response:
          sendHttpResponse(client);
          break;
        }
        if (buf.endsWith("GET /H")) {
          ligaLuz();
          luzStatus = 1;
          goto response;
        }
        else { //SENÃO, FAZ
          if (buf.endsWith("GET /L")) {
            desligaLuz();
            luzStatus = 0;
            goto response;

          } else {
            if (buf.endsWith("GET /R")) {
              acionaValvula();
              goto response;
            } 
          }
        }

      }

    }

    client.stop(); //FINALIZA A REQUISIÇÃO HTTP E DESCONECTA O CLIENTE
  }
  delay(10);
 if((millis() - millisTarefa) > 300000){
    millisTarefa = millis();
    verificarSolo();
  }


}

//MÉTODO DE RESPOSTA A REQUISIÇÃO HTTP DO CLIENTE
void sendHttpResponse(WiFiEspClient client) {

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("");
  client.println("<!DOCTYPE HTML><html><body>");
  client.println(sensorHumidade());
  client.println("<br>");
  client.println(sensorSolo());
  client.println("<br>");
  client.println(luzStatus);
  client.println( "</body></html>");
  delay(1); //INTERVALO DE 1 MILISSEGUNDO
}


//retorna os dados dos sensores de umidade/temperatura
String sensorHumidade() {
  DHT.read11(pinoDHT11); //LÊ AS INFORMAÇÕES DO SENSOR
  String humidity = String(DHT.humidity, 4);
  String temperature = String(DHT.temperature, 4);
  return humidity + "/" + temperature;
}
void musica() {
  buzzer.begin(100);
  buzzer.sound(392, 50);
  buzzer.sound(0, 50);
  buzzer.sound(440, 50);
  buzzer.sound(0, 50);
  buzzer.sound(494, 50);
  buzzer.sound(0, 50);
  buzzer.end(2000);
}


void configParametros()
{
  humidadeMinimaSolo = configs[0].toInt();
  ssid = configs[1];
  password = configs[2];
  ip[0] = configs[3].toInt();
  ip[1] = configs[4].toInt();
  ip[2] = configs[5].toInt();
  ip[3] = configs[6].toInt();

  int mode = configs[7].toInt();

  if (mode == 0) {
    modoAuto = false;
  } else if (humidadeMinimaSolo < 10) {
    modoAuto = false;
  } else {
    modoAuto = true;
  }
}

void ligaLuz() {
  digitalWrite(pin_luz, LOW);
}
void desligaLuz() {
  digitalWrite(pin_luz, HIGH);
}

void acionaValvula() {
  digitalWrite(pin_valvula, LOW);
  delay(1000);
  digitalWrite(pin_valvula, HIGH);
}

void verificarSolo() {

  int valor_solo = constrain(analogRead(sensor_solo), analogSoloMolhado, analogSoloSeco); //MANTÉM valorLido DENTRO DO INTERVALO (ENTRE analogSoloMolhado E analogSoloSeco)
  valor_solo = map(valor_solo, analogSoloMolhado, analogSoloSeco, percSoloMolhado, percSoloSeco); //EXECUTA A FUNÇÃO "map" DE ACORDO COM OS PARÂMETROS PASSADOS
  if (modoAuto) {
    if (valor_solo < humidadeMinimaSolo) {
      acionaValvula();
    }
  }
}

String sensorSolo() {
  int valor_solo = constrain(analogRead(sensor_solo), analogSoloMolhado, analogSoloSeco); //MANTÉM valorLido DENTRO DO INTERVALO (ENTRE analogSoloMolhado E analogSoloSeco)
  valor_solo =  map(valor_solo, analogSoloMolhado, analogSoloSeco, percSoloMolhado, percSoloSeco); //EXECUTA A FUNÇÃO "map" DE ACORDO COM OS PARÂMETROS PASSADOS
  String umidadeSolo = String(valor_solo, 4);
  return  umidadeSolo;
}

void errorInicializacao(int vezes, int frequencia, int del) {
  buzzer.begin(100);
  for (int i = 0; i < vezes; i++) {

    buzzer.sound(frequencia, 800);
    buzzer.sound(0, 80);
    delay(del);
  }
  delay(5000);
  asm volatile ("  jmp 0");
}

/*
void gravarEmArquivo(char* novosParametros) {

  File myFile  = SD.open("CONFIG.txt", FILE_WRITE);
  if (myFile) {

    SD.remove("CONFIG.txt");
    myFile  = SD.open("CONFIG.txt", FILE_WRITE);

    //Inicia a escrita sempre na primeira linha do arquivo, sobrescrevendo os dados anteriores.
    myFile.print(
      String(getValue(novosParametros, ';', 0) )
      + "\n" + getValue(novosParametros, ';', 1)
      + "\n" + getValue(novosParametros, ';', 2)
      + "\n" + getValue(novosParametros, ';', 3)
      + "\n" + getValue(novosParametros, ';', 4)
      + "\n" + getValue(novosParametros, ';', 5)
      + "\n" + getValue(novosParametros, ';', 6)
      + "\n" + getValue(novosParametros, ';', 7)
      + "\n"); //Escreve os valores das potências acumuladas.

    myFile.close(); //Fecha o arquivo.
    asm volatile ("  jmp 0");//reinicia o arduino
  } else {
    myFile.close();
    errorInicializacao(3, 500, 500);
  }

}

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
*/

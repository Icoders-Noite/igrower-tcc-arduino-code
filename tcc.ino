
#include "dht.h" // biblioteca DO SENSOR DHT11
#include <Buzzer.h>
#include <stdlib.h>
#include <SPI.h>
#include <SD.h>


//sensor dht11
const int pinoDHT11 = A5; //PINO ANALÓGICO UTILIZADO PELO DHT11

//buzzer
const int pinoBuzzer = 2;
Buzzer buzzer(pinoBuzzer);

dht DHT; //VARIÁVEL DO TIPO DHT

//Variaveis para cartao SD
String parameter;
byte line;
String configs[4];
// Pino ligado ao CS do modulo
const int chipSelect = 4;

//parametros
int humidadeMinimaSolo = 0;
String ssid = "", password = "";
bool modoAuto = false;

//valvula
int pin_valvula = 8;
//luz
int pin_luz = 9;

// sensor solo
int sensor_solo = A4;
int analogSoloSeco = 400; //VALOR MEDIDO COM O SOLO SECO (VOCÊ PODE FAZER TESTES E AJUSTAR ESTE VALOR)
int analogSoloMolhado = 150; //VALOR MEDIDO COM O SOLO MOLHADO (VOCÊ PODE FAZER TESTES E AJUSTAR ESTE VALOR)
int percSoloSeco = 0; //MENOR PERCENTUAL DO SOLO SECO (0% - NÃO ALTERAR)
int percSoloMolhado = 100;

void setup() {

  Serial.begin(115200); //INICIALIZA A SERIAL
  delay(2000); //INTERVALO DE 2 SEGUNDO ANTES DE INICIAR


  while (!Serial);
  Serial.println("Initializing SD card...");
  if (!SD.begin(chipSelect))
  {
    Serial.println("Card failed, or not present");
    errorInicializacao(3, 200, 500);
  }
  Serial.println("card initialized.");
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

  //quando está tudo certo
  musica();

}

void loop() {

}


//retorna os dados dos sensores de umidade/temperatura
String sensorHumidade() {
  DHT.read11(pinoDHT11); //LÊ AS INFORMAÇÕES DO SENSOR
  String humidity = String(DHT.humidity, 4);
  String temperature = String(DHT.temperature, 4);
  return  String("{") + char(34) + "umidade" + char(34) + ":" + humidity + "," + char(34) + "temperatura" + char(34) + ":" + temperature + "}";
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
  int mode = configs[3].toInt();

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
  Serial.println(valor_solo);
  valor_solo = map(valor_solo, analogSoloMolhado, analogSoloSeco, percSoloMolhado, percSoloSeco); //EXECUTA A FUNÇÃO "map" DE ACORDO COM OS PARÂMETROS PASSADOS
  Serial.println(valor_solo);
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
  return  String("{") + char(34) + "umidadeSolo" + char(34) + ":" + umidadeSolo + "}";
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

void gravarEmArquivo(char* novosParametros) {

  File myFile  = SD.open("CONFIG.txt", FILE_WRITE);
  if (myFile) {

    SD.remove("CONFIG.txt");
    myFile  = SD.open("CONFIG.txt", FILE_WRITE);
    //Inicia a escrita sempre na primeira linha do arquivo, sobrescrevendo os dados anteriores.
    myFile.print(String(getValue(novosParametros, ';', 0) ) + "\n" + getValue(novosParametros, ';', 1) + "\n" + getValue(novosParametros, ';', 2)  + "\n" + getValue(novosParametros, ';', 3)  + "\n"); //Escreve os valores das potências acumuladas.
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

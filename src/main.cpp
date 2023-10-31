#include <Arduino.h>
#include "../include/MS01.h"
#include <WiFi.h>
#include <PubSubClient.h>

#define SENSOR_PIN 25 //Digital pin of MS01 sensor

/*   Sidenotes
  Isto está meio instável, deve ser reescrito de forma a usar direitinho o 
  FreeRTOS mas dada a natureza do protocolo de comunicação do MS01 à mínima diferença
  de timmings a leitura dá em erro, de qualquer forma assim está "usável".
  Por vezes as primeiras tentativas após boot dão erro mas eventualmente (após +/- 10 ciclos)
  começam a sair medições corretas.
  Outro pormenor está na accuracy das medições, a Sonoff não tem documentação nenhuma
  do seu protocolo e apenas com a ajuda do projeto Tasmota (https://tasmota.github.io/docs/)
  é que foi possível ler do sensor e ter uma "escala" mais ou menos usável. De qualquer forma
  os valores ao usar o Tasmota eram de 0.01% em ambiente seco e nunca passavam dos 50% com o
  sensor imerso parcialmente em água, por isso no MS01.Read() tem um remap entre 0 e o valor
  medido com ele imerso em água (variável wet_raw_value).
  Por último, por vezes há umas medidas que saem muito ao lado das suas vizinhas sem que se
  tenha mexido no sensor, por isso o valor final de humidade relativa é uma média entre as
  últimas 3 medições, para desligar basta alterar o "smoothing" do ConvertHumidity de true para
  false. 
  (NÃO ALTERAR TIMMINGS)
*/

const char *ssid = "";                  //SSID
const char *password = "";              //PASSWORD
const char *broker = "broker.hivemq.com";
const int tcpPort = 1883;
const char *mqttid = "MS01-ESP32/hum";
char msg[50];
float hum = 0;

WiFiClient espClient;
PubSubClient client(espClient);

uint8_t sensorPin = 25;  //Pin where sensor is connected

TaskHandle_t task_loop1;

void reconnect(){
  while(!client.connected()) {
    Serial.print("Attempting MQTT connection...");

    // Create a random client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    Serial.println("Connecting to MQTT broker");

    if(client.connect(clientId.c_str())) {
      Serial.println("Connected");
      client.publish(mqttid, "MQTT Server is Connected");

    } else{
      Serial.println("Trying again in 1 second");
      delay(1000);

    }
  }
}

void setupWifi(){
  //Setup Wifi
  Serial.print("Connecting to ");
  Serial.print(ssid);
  Serial.print(" .");
  WiFi.begin(ssid, password);

  while(WiFi.status() != WL_CONNECTED){
    Serial.print(".");
  }

  Serial.println("\nConnected");

  client.setServer(broker, tcpPort);
}

void another_setup1() {
  setupWifi();
}

void another_loop1() {
  if(!client.connected()){
    reconnect();
  }
  client.loop();

  snprintf(msg, 50, "RH is: %.2f", hum);
  client.publish(mqttid, msg);

  delay(2500);
}

void esploop1(void *param) {
  another_setup1();
  for (;;){
    another_loop1();
  }
}

void setup() {
  Serial.begin(9600);
  MS01.pin = SENSOR_PIN;
  xTaskCreatePinnedToCore(
      esploop1,
      "mqtt_task",
      15000,
      NULL,
      1,
      &task_loop1,
      !ARDUINO_RUNNING_CORE);
  
}

void loop() {
  if(MS01.Read()){
    hum = MS01.h;
    Serial.println(MS01.h);
  } else{
    Serial.println("Error");
  }
  delay(2000);
}
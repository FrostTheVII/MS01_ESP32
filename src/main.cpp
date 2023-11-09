#include <Arduino.h>
#include "../include/MS01.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <NTPClient.h>

#define SENSOR_PIN 25 // Digital pin of MS01 sensor

/*   Sidenotes
  Pequeno pormenor na accuracy das medições, a Sonoff não tem documentação nenhuma
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

const char *ssid = "";     // SSID
const char *password = ""; // PASSWORD
const char *broker = "broker.hivemq.com";
const int tcpPort = 1883;
const char *mqttid = "MS01-ESP32/hum";
String msg;
float hum = 0;
int curr_time = 0;
int prev_update_ntp_time = 0;
int prev_update_epoch_time = 0;

const char *ntp_Server = "pool.ntp.org";
const int32_t gmt_offset_sec = 0;      // adjust to Portugal timezone
const int32_t daylight_offset_sec = 0; // adjust this if your location observes DST
unsigned long epoch_time = 0;
WiFiUDP wifi_udp_client;
NTPClient ntp_client(wifi_udp_client, ntp_Server, gmt_offset_sec, daylight_offset_sec);

WiFiClient espClient;
PubSubClient client(espClient);

TaskHandle_t task_loop1;

void reconnect()
{
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");

    // Create a random client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    Serial.println("Connecting to MQTT broker");

    if (client.connect(clientId.c_str()))
    {
      Serial.println("Connected");
      client.publish(mqttid, "MQTT Server is Connected");
    }
    else
    {
      Serial.println("Trying again in 1 second");
      delay(1000);
    }
  }
}

void setupWifi()
{
  // Setup Wifi
  Serial.print("Connecting to ");
  Serial.print(ssid);
  Serial.print(" .");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }

  Serial.println("\nConnected");

  client.setServer(broker, tcpPort);
}

void epoch_time_update()
{
  int passed_time = (curr_time - prev_update_epoch_time) / 1000;
  if (passed_time > 0)
  {
    epoch_time += passed_time;
    prev_update_epoch_time = curr_time;
  }

  if (ntp_client.update())
  {
    epoch_time = ntp_client.getEpochTime();
    // Serial.println("Synced with NTP server.");
  }
  prev_update_ntp_time = curr_time;
}

String convert_epoch_time_to_string()
{

  time_t raw_time = static_cast<time_t>(epoch_time);
  struct tm *time_info;
  time_info = localtime(&raw_time);

  char date_time_string[50]; // Adjust the size as needed
  strftime(date_time_string, sizeof(date_time_string), "%Y-%m-%d %H:%M:%S", time_info);

  return String(date_time_string);
}

void another_setup1()
{
  setupWifi();

  while (!ntp_client.update())
  {
    // force ntp time update
  }
  epoch_time = ntp_client.getEpochTime();
}

void another_loop1()
{
  curr_time = millis();
  epoch_time_update();

  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  // snprintf(msg, 100, "RH is: %.2f", hum);
  msg = convert_epoch_time_to_string() + " " + String(hum);
  client.publish(mqttid, msg.c_str());

  delay(2500);
}

void esploop1(void *param)
{
  another_setup1();
  for (;;)
  {
    another_loop1();
  }
}

void setup()
{
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

void loop()
{
  if (MS01.Read())
  {
    hum = MS01.h;
    Serial.println(MS01.h);
  }
  else
  {
    Serial.println("Error");
  }

  delay(2000);
}
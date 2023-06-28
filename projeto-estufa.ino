#include <WiFiManager.h>  // https://github.com/tzapu/WiFiManager
#include <Preferences.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "max6675.h"

#define PIN_RELE_BOMBA 27
#define PIN_RELE_COOLER_2 4
#define PIN_RELE_COOLER_1 21
#define PIN_RELE_LAMPADA 16

#define PIN_UMIDADE 32
#define WIFI_AP "Projeto Estufa"
#define WIFI_PASS "admin123"

#define TIME_UMIDADE 1000
#define TIME_RELE 1000
#define TIME_MQTT 1000
#define TIME_LAMPADA 1000
#define TIME_TEMPERATURA 1000

#define TIME_DEBOUNCE 10  //ms

int VALOR_UMIDADE=0;
float VALOR_TEMPERATURA=0.0;

// pinos sensor termoPar - NTC
int thermoDO = 19;
int thermoCS = 23;
int thermoCLK = 5;

MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

UBaseType_t uxHighWaterMark1;
UBaseType_t uxHighWaterMark2;

struct Struct_mqtt {
  uint16_t value_umidade;
  float value_temperatura;
};

struct Struct_rele {
  uint16_t pino_umidade;
  uint16_t pino_temperatura;
  uint16_t rele_bomba;
  uint16_t rele_cooler1;
};

const char *SSID = "Sem Senha 2";           // SSID / nome da rede WI-FI que deseja se conectar
const char *PASSWORD = "Total100Overdose";  // Senha da rede WI-FI que deseja se conectar

WiFiManager wm;

/* Configura os tópicos do MQTT */
#define TOPIC_PUBLISH_UMIDADE "topico_umidade"
#define TOPIC_PUBLISH_TEMPERATURA "topico_temperatura"
#define TOPIC_SUBSCRIBE_UMIDADE "topico_set_umidade"
#define TOPIC_SUBSCRIBE_TEMPERATURA "topico_set_temperatura"
#define TOPIC_SUBSCRIBE_COOLER "topico_set_cooler"
#define TOPIC_SUBSCRIBE_LAMPADA "topico_set_lampada"

#define ID_MQTT "esp32_mqtt"              // id mqtt (para identificação de sessão)
const char *BROKER_MQTT = "192.168.0.3";  // URL do broker MQTT que se deseja utilizar
int BROKER_PORT = 1883;                   // Porta do Broker MQTT
WiFiClient espClient;                     // Cria o objeto espClient
PubSubClient MQTT(espClient);             // Instancia o Cliente MQTT passando o objeto espClient

unsigned long publishUpdate;

xQueueHandle fila_mqtt;
xQueueHandle fila_rele;

/*Cria um identificador de Mutex*/
SemaphoreHandle_t xMutex_C1;
SemaphoreHandle_t xMutex_C2;

void setup() {
  Serial.begin(115200);
  delay(1000);
  pinMode(PIN_RELE_COOLER_1, OUTPUT);
  pinMode(PIN_RELE_COOLER_2, OUTPUT);
  pinMode(PIN_RELE_LAMPADA, OUTPUT);
  pinMode(PIN_RELE_BOMBA, OUTPUT);

  digitalWrite(PIN_RELE_COOLER_1, HIGH);
  digitalWrite(PIN_RELE_COOLER_2, LOW);
  digitalWrite(PIN_RELE_LAMPADA, HIGH);
  digitalWrite(PIN_RELE_BOMBA, LOW);

  Serial.println("Iniciando pra executar as funcoes..");

  //criando o MUTEX
  xMutex_C1 = xSemaphoreCreateMutex();
  xMutex_C2 = xSemaphoreCreateMutex();

  fila_mqtt = xQueueCreate(30, sizeof(Struct_mqtt));
  fila_rele = xQueueCreate(30, sizeof(Struct_rele));

  //ConfigWifi();
  ConnectWifi();
  initMQTT();

  xTaskCreatePinnedToCore(
    &TaskSensores,  /* Tarefa ou Função  */
    "TaskSensores", /* Nome da Tarefa ou Função  */
    5000,          /* Tamanho da Pilha  */
    NULL,          /* Parâmetro de Entrada  */
    5,             /* Prioridade da Tarefa  */
    NULL,          /* Identificador da Tarefa  */
    APP_CPU_NUM    /* Núcleo onde será executada a função. */
  );

  xTaskCreatePinnedToCore(
    &TaskMQTT,  /* Tarefa ou Função  */
    "TaskMQTT", /* Nome da Tarefa ou Função  */
    8000,       /* Tamanho da Pilha  */
    NULL,       /* Parâmetro de Entrada  */
    3,          /* Prioridade da Tarefa  */
    NULL,       /* Identificador da Tarefa  */
    PRO_CPU_NUM /* Núcleo onde será executada a função. */
  );

  xTaskCreatePinnedToCore(
    &TaskRele,  /* Tarefa ou Função  */
    "TaskRele", /* Nome da Tarefa ou Função  */
    5000,       /* Tamanho da Pilha  */
    NULL,       /* Parâmetro de Entrada  */
    4,          /* Prioridade da Tarefa  */
    NULL,       /* Identificador da Tarefa  */
    APP_CPU_NUM /* Núcleo onde será executada a função. */
  );
}

void loop() {

}

void ConfigWifi() {
  bool res;
  //wm.resetSettings();
  res = wm.autoConnect(WIFI_AP, WIFI_PASS);  // password protected
  if (!res) {
    Serial.println("Failed to connect");
  } else {
    Serial.println("ESP32 Conectado com sucesso no Wifi");
  }
}

void ConnectWifi() {
  if (WiFi.status() == WL_CONNECTED)
    return;

  WiFi.begin(SSID, PASSWORD);  // Conecta na rede WI-FI

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
}
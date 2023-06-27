void callbackMQTT(char *topic, byte *payload, unsigned int length)
{
  Struct_rele rele;
  String msg_broker;
  char c;
  int aux1;
  float aux2;
  
  for(int i=0; i < length; i++){
    c = (char)payload[i];
    msg_broker += c;
  }
  
  Serial.print("CallbackMQTT -> Topico: ");
  Serial.print(topic);
  Serial.print(" | Msg: ");
  Serial.println(msg_broker);
  
  if(strcmp(topic, "topico_set_umidade") == 0){
    aux1 = msg_broker.toInt();
    VALOR_UMIDADE = aux1;
  }
  
  if(strcmp(topic, "topico_set_temperatura") == 0){
    aux2 = msg_broker.toFloat();
    VALOR_TEMPERATURA = aux2;
  }

  if(strcmp(topic, "topico_set_cooler") == 0){
    aux1 = msg_broker.toInt();
    rele.pino_umidade = PIN_RELE_COOLER_2;
    rele.rele_bomba = aux1;

    xQueueSend(fila_rele, &rele, portMAX_DELAY);
  }

  if(strcmp(topic, "topico_set_lampada") == 0){
    aux1 = msg_broker.toInt();
    rele.pino_umidade = PIN_RELE_LAMPADA;
    rele.rele_bomba = aux1;

    xQueueSend(fila_rele, &rele, portMAX_DELAY);
  }
}

void initMQTT(void)
{
  MQTT.setServer(BROKER_MQTT, BROKER_PORT); // Informa qual broker e porta deve ser conectado
  MQTT.setCallback(callbackMQTT);           // Atribui função de callback (função chamada quando qualquer informação de um dos tópicos subescritos chega)

  if (MQTT.connect(ID_MQTT)) {
    Serial.println("Conectado com sucesso ao broker MQTT!");
    MQTT.subscribe(TOPIC_SUBSCRIBE_TEMPERATURA);
    MQTT.subscribe(TOPIC_SUBSCRIBE_UMIDADE);
    MQTT.subscribe(TOPIC_SUBSCRIBE_COOLER);
    MQTT.subscribe(TOPIC_SUBSCRIBE_LAMPADA);
  }
}
void checkWiFIAndMQTT(void)
{
  if (WiFi.status() != WL_CONNECTED)
    WiFi.begin(SSID, PASSWORD); // Conecta na rede WI-FI

  if (!MQTT.connected())
    reconnectMQTT(); // se não há conexão com o Broker, a conexão é refeita
}

void reconnectMQTT(void)
{
  Serial.print("* Tentando se conectar ao Broker MQTT: ");
  Serial.println(BROKER_MQTT);
  
  if (MQTT.connect(ID_MQTT)) {
    Serial.println("Conectado com sucesso ao broker MQTT!");
  } else {
    Serial.println("Falha ao reconectar no broker.");
  }
}

void sendMQTT(const char* topico, const char* value) {
  MQTT.publish(topico, value);
  MQTT.loop();
}
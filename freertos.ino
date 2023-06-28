void TaskSensores(void* parametro) {
  Struct_mqtt data;
  Struct_rele rele;
  int value_umidade;
  float value_temperatura;
  while (1) {
    if (xMutex_C2 != NULL) {
      if (xSemaphoreTake(xMutex_C2, portMAX_DELAY) == pdTRUE) {
        data.value_umidade = analogRead(PIN_UMIDADE);
        data.value_temperatura = thermocouple.readCelsius();

        Serial.print("TaskSensor -> High Water Mark: ");
        uxHighWaterMark1 = uxTaskGetStackHighWaterMark(NULL);
        Serial.print(uxHighWaterMark1);
        Serial.print(" | Umi: ");
        Serial.print(data.value_umidade);
        Serial.print(" | Temp: ");
        Serial.println(data.value_temperatura);

        rele.pino_umidade = PIN_RELE_BOMBA;
        rele.pino_temperatura = PIN_RELE_COOLER_1;
        //rele.estado_umidade = 1;

        rele.rele_bomba = 0;
        if(VALOR_UMIDADE != 0){
          if (data.value_umidade > VALOR_UMIDADE) {
            rele.rele_bomba = 1;
          }
        }
        
        rele.rele_cooler1 = 1;
        if(VALOR_TEMPERATURA != 0.0){
          if (data.value_temperatura > VALOR_TEMPERATURA) {
            Serial.println("Rele Cooler ligado");
            rele.rele_cooler1 = 0;
          }
        }

        xQueueSend(fila_mqtt, &data, portMAX_DELAY);
        xQueueSend(fila_rele, &rele, portMAX_DELAY);
      }
      xSemaphoreGive(xMutex_C2);
    }
    vTaskDelay(TIME_UMIDADE / portTICK_PERIOD_MS);
  }
}

void TaskRele(void* parametro) {
  Struct_rele rele;

  while (1) {

    if (xQueueReceive(fila_rele, &rele, 1000 / portTICK_PERIOD_MS) == pdPASS) {
      if (xMutex_C2 != NULL) {
        if (xSemaphoreTake(xMutex_C2, portMAX_DELAY) == pdTRUE) {

          Serial.print("TaskRele -> High Water Mark: ");
          uxHighWaterMark1 = uxTaskGetStackHighWaterMark(NULL);
          Serial.print(uxHighWaterMark1);
          Serial.print(" | Rele Umidade: ");
          Serial.print(rele.pino_umidade);
          Serial.print(" | Estado: ");
          Serial.print(rele.rele_bomba);

          Serial.print(" | Rele Temperatura: ");
          Serial.print(rele.pino_temperatura);
          Serial.print(" | Estado: ");
          Serial.println(rele.rele_cooler1);

          digitalWrite(rele.pino_temperatura, rele.rele_cooler1);
          digitalWrite(rele.pino_umidade, rele.rele_bomba);
        }
        xSemaphoreGive(xMutex_C2);
      }
    }
    vTaskDelay(TIME_RELE / portTICK_PERIOD_MS);
  }
}

void TaskMQTT(void* parametro) {
  Struct_mqtt pacote_mqtt;
  char buffer_umidade[10];
  char buffer_temperatura[10];

  while (1) {
    if (xQueueReceive(fila_mqtt, &pacote_mqtt, 1000 / portTICK_PERIOD_MS) == pdPASS) {
      if (xMutex_C1 != NULL) {
        if (xSemaphoreTake(xMutex_C1, portMAX_DELAY) == pdTRUE) {

          Serial.print("TaskMQTT -> High Water Mark: ");
          uxHighWaterMark2 = uxTaskGetStackHighWaterMark(NULL);
          Serial.print(uxHighWaterMark2);

          sprintf(buffer_umidade, "%u", pacote_mqtt.value_umidade);
          sprintf(buffer_temperatura, "%f", pacote_mqtt.value_temperatura);

          Serial.print(" | Topico: ");
          Serial.print(TOPIC_PUBLISH_UMIDADE);
          Serial.print(" | Valor: ");
          Serial.print(buffer_umidade);
          Serial.print(" | Topico: ");
          Serial.print(TOPIC_PUBLISH_TEMPERATURA);
          Serial.print(" | Valor: ");
          Serial.println(buffer_temperatura);

          MQTT.publish(TOPIC_PUBLISH_UMIDADE, buffer_umidade);
          MQTT.publish(TOPIC_PUBLISH_TEMPERATURA, buffer_temperatura);
          MQTT.loop();
        }
        xSemaphoreGive(xMutex_C1);
      }
    }
    vTaskDelay(TIME_MQTT / portTICK_PERIOD_MS);
  }
}
#include <Arduino.h>


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

// Data structures
typedef struct {
  uint8_t dataID;
  int32_t DataValue;
} Data_t;

// global variables
QueueHandle_t Queue1;
TaskHandle_t TaskHandle_1;
TaskHandle_t TaskHandle_2;

volatile uint8_t G_DataID = 1;
volatile int32_t G_DataValue = 100;

// task sender
void ExampleTask1(void *pvParameters) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(500);
  Data_t txData;

  while (1) {
    txData.dataID = G_DataID;
    txData.DataValue = G_DataValue;

    xQueueSend(Queue1, &txData, portMAX_DELAY);
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

// task receiver
void ExampleTask2(void *pvParameters) {
  Data_t rxData;
  UBaseType_t originalPriority = uxTaskPriorityGet(NULL);
  bool priorityIncreased = false;

  while (1) {
    if (xQueueReceive(Queue1, &rxData, portMAX_DELAY) == pdPASS) {
      Serial.print("Received -> dataID: ");
      Serial.print(rxData.dataID);
      Serial.print(", DataValue: ");
      Serial.println(rxData.DataValue);

      if (rxData.dataID == 0) {
        Serial.println("Deleting ExampleTask2 (dataID == 0)");
        vTaskDelete(NULL);
      } else if (rxData.dataID == 1) {
        if (rxData.DataValue == 0 && !priorityIncreased) {
          vTaskPrioritySet(NULL, originalPriority + 2);
          priorityIncreased = true;
          Serial.println("Priority increased by 2");
        } else if (rxData.DataValue == 1 && priorityIncreased) {
          vTaskPrioritySet(NULL, originalPriority);
          priorityIncreased = false;
          Serial.println("Priority reset to original");
        } else if (rxData.DataValue == 2) {
          Serial.println("Deleting ExampleTask2 (DataValue == 2)");
          vTaskDelete(NULL);
        }
      }
    }
  }
}

void setup() {
  Serial.begin(115200);

  Queue1 = xQueueCreate(5, sizeof(Data_t));
  if (Queue1 == NULL) {
    Serial.println("Queue creation failed");
    while (1);
  }

  xTaskCreatePinnedToCore(ExampleTask1, "Sender", 2048, NULL, 1, &TaskHandle_1, 1);
  xTaskCreatePinnedToCore(ExampleTask2, "Receiver", 2048, NULL, 1, &TaskHandle_2, 1);
}

void loop() {
  // Simulate changes to global values every 5 seconds
  static unsigned long last = 0;
  if (millis() - last > 5000) {
    G_DataID = 1;
    G_DataValue = (G_DataValue + 1) % 3;
    last = millis();
  }
}

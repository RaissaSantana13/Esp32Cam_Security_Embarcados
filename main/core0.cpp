#include "core_tasks.h"

// core 0
// PIR wifi webserver monitorar e sinalizar quando tiver monimento

void monitorTask(void* pvParameters)
{
    while (true)
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            Serial.println("Trying to connect WiFi...");

            WiFi.begin(ssid, password);

            vTaskDelay(pdMS_TO_TICKS(5000));
        }

        server.handleClient();

        bool currentPIRState = digitalRead(PIR_PIN);

        if (currentPIRState == HIGH &&
            previousPIRState == LOW &&
            (millis() - lastMotionTime > cooldown))
        {
            lastMotionTime = millis();

            Serial.println("Motion detected!");

            xSemaphoreGive(motionSemaphore);
        }

        previousPIRState = currentPIRState;

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
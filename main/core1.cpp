#include "core_tasks.h"

// core 1
// captura a imagem envia pro telegram e drive salva no sd

void cameraProcessTask(void* pvParameters)
{
    while (true)
    {
        if (xSemaphoreTake(
                motionSemaphore,
                portMAX_DELAY) == pdTRUE)
        {
            Serial.println("Camera task awakened.");

            pixel.setPixelColor(
                0,
                pixel.Color(255, 255, 255)
            );
            pixel.show();

            vTaskDelay(pdMS_TO_TICKS(250));

            capturePhotoRAM();

            pixel.setPixelColor(
                0,
                pixel.Color(0, 0, 0)
            );
            pixel.show();

            if (WiFi.status() == WL_CONNECTED)
            {
                sendTelegram();

                if (uploadGoogleDrive())
                {
                    Serial.println("Drive upload OK");
                }
                else
                {
                    Serial.println("Drive upload error");
                }
            }
            else
            {
                Serial.println("No internet. Saving to SD.");
                savePhotoSD();
            }

            syncSD();

            Serial.println(
                "Camera task finished. Waiting for next motion."
            );
        }
    }
}
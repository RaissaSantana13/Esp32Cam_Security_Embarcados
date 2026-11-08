#include <Arduino.h>

#include "core_tasks.h"

#include "esp_pm.h"
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "esp_err.h"

const char* ssid = "Raissa";
const char* password = "deznumeros";

const char* botToken = "8214184329:AAEziRtNdGB3zaIfJeaULuDdZHbVQhJk5Zg";
const char* chatID = "1063941223";

String googleScript = "/macros/s/AKfycby8EaQ8tPf_yKnLyubT_ZMhFgj0gqXbuSFcqfXNgQ-_Uf973YdQ_uNqQn3rPCnX1u4MEQ/exec";

unsigned long lastMotionTime = 0;
const unsigned long cooldown = 10000;

bool previousPIRState = LOW;
bool sdAvailable = false;

uint8_t* photoBuffer = NULL;
size_t photoSize = 0;

Adafruit_NeoPixel pixel(
    NUM_PIXELS,
    LED_FLASH,
    NEO_GRB + NEO_KHZ800
);

WebServer server(80);
WiFiClientSecure telegramClient;

SemaphoreHandle_t motionSemaphore;

void configurePowerManagement()
{
    esp_pm_config_t pm_config = {
        .max_freq_mhz = 240,
        .min_freq_mhz = 80,
        .light_sleep_enable = true
    };

    ESP_ERROR_CHECK(
        esp_pm_configure(&pm_config)
    );

    ESP_ERROR_CHECK(
        gpio_wakeup_enable(
            (gpio_num_t)PIR_PIN,
            GPIO_INTR_HIGH_LEVEL
        )
    );

    ESP_ERROR_CHECK(
        esp_sleep_enable_gpio_wakeup()
    );

    Serial.println("Automatic Light Sleep enabled.");
}

void initSD()
{
    if (!SD_MMC.begin("/sdcard", true))
    {
        Serial.println("No SD card");
        sdAvailable = false;
    }
    else
    {
        Serial.println("SD OK");
        sdAvailable = true;
    }
}

void initCamera()
{
    camera_config_t config = {};

    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;

    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;

    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;

    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;

    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;

    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;

    config.frame_size = FRAMESIZE_HD;
    config.jpeg_quality = 10;
    config.fb_count = 1;
    config.fb_location = CAMERA_FB_IN_PSRAM;

    if (esp_camera_init(&config) != ESP_OK)
    {
        Serial.println("Camera error");
        ESP.restart();
    }

    sensor_t *sensor = esp_camera_sensor_get();

    sensor->set_vflip(sensor, 1);
    sensor->set_hmirror(sensor, 0);

    Serial.println("Camera OK");
}

void capturePhotoRAM()
{
    camera_fb_t* fb = esp_camera_fb_get();

    if (!fb)
    {
        Serial.println("Capture error");
        return;
    }

    if (photoBuffer != NULL)
    {
        free(photoBuffer);
        photoBuffer = NULL;
    }

    photoBuffer = (uint8_t*)ps_malloc(fb->len);

    if (photoBuffer == NULL)
    {
        Serial.println("RAM error");
        esp_camera_fb_return(fb);
        return;
    }

    memcpy(photoBuffer, fb->buf, fb->len);
    photoSize = fb->len;

    esp_camera_fb_return(fb);

    Serial.print("Photo captured, Size: ");
    Serial.println(photoSize);
}

void connectWiFi()
{
    Serial.println("Connecting WiFi...");

    WiFi.begin(ssid, password);

    unsigned long start = millis();

    while (WiFi.status() != WL_CONNECTED &&
           millis() - start < 15000)
    {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("\nWiFi connected");
        Serial.print("IP: http://");
        Serial.println(WiFi.localIP());
    }
    else
    {
        Serial.println("\nWiFi failed");
    }
}

bool isInternetAvailable()
{
    return WiFi.status() == WL_CONNECTED;
}

void savePhotoSD()
{
    if (!sdAvailable || !photoBuffer)
        return;

    String path = "/photo_" + String(millis()) + ".jpg";

    File file = SD_MMC.open(path, FILE_WRITE);

    if (!file)
    {
        Serial.println("Error saving to SD");
        return;
    }

    file.write(photoBuffer, photoSize);
    file.close();

    Serial.println("Photo saved to SD");
}

String urlencode(String str)
{
    String encodedString = "";

    char c;
    char code0;
    char code1;

    for (int i = 0; i < str.length(); i++)
    {
        c = str.charAt(i);

        if (isalnum(c))
        {
            encodedString += c;
        }
        else
        {
            code1 = (c & 0xf) + '0';

            if ((c & 0xf) > 9)
                code1 = (c & 0xf) - 10 + 'A';

            c = (c >> 4) & 0xf;

            code0 = c + '0';

            if (c > 9)
                code0 = c - 10 + 'A';

            encodedString += '%';
            encodedString += code0;
            encodedString += code1;
        }

        yield();
    }

    return encodedString;
}

bool uploadGoogleDrive()
{
    if (!photoBuffer || photoSize == 0)
    {
        Serial.println("No photo available for Drive");
        return false;
    }

    Serial.println("Connecting to Google Drive...");

    WiFiClientSecure client;
    client.setInsecure();

    if (!client.connect("script.google.com", 443))
    {
        Serial.println("Drive connection failed");
        return false;
    }

    Serial.println("Encoding image...");

    String imageFile =
        base64::encode(photoBuffer, photoSize);

    String data = "filename=ESP32-CAM.jpg";
    data += "&mimetype=image/jpeg";
    data += "&data=";
    data += urlencode(imageFile);

    Serial.print("POST data size: ");
    Serial.println(data.length());

    client.println(
        "POST " + googleScript + " HTTP/1.1"
    );

    client.println(
        "Host: script.google.com"
    );

    client.println(
        "Content-Type: application/x-www-form-urlencoded"
    );

    client.println(
        "Content-Length: " + String(data.length())
    );

    client.println("Connection: close");
    client.println();

    const size_t chunkSize = 2048;
    size_t totalSent = 0;

    while (totalSent < data.length())
    {
        size_t remaining =
            data.length() - totalSent;

        size_t currentChunk =
            remaining > chunkSize
                ? chunkSize
                : remaining;

        size_t sent = client.write(
            (const uint8_t*)data.c_str() + totalSent,
            currentChunk
        );

        if (sent == 0)
        {
            Serial.println(
                "Error sending data to Drive"
            );

            client.stop();
            return false;
        }

        totalSent += sent;

        delay(1);
    }

    Serial.print("Data sent: ");
    Serial.print(totalSent);
    Serial.print("/");
    Serial.println(data.length());

    unsigned long timeout = millis();

    while (!client.available())
    {
        if (millis() - timeout > 35000)
        {
            Serial.println("Drive timeout - No response");
            client.stop();
            return false;
        }

        delay(10);
    }

    String response = "";

    while (client.available())
    {
        response += (char)client.read();
    }

    client.stop();

    Serial.println(response);

    if (response.indexOf("200 OK") >= 0)
        return true;

    if (response.indexOf("302") >= 0 ||
        response.indexOf("301") >= 0)
    {
        if (response.indexOf("macros/echo") >= 0)
            return true;

        Serial.println(
            "Unexpected redirect. Check Apps Script."
        );

        return false;
    }

    Serial.println("Drive upload failed");

    return false;
}

void syncSD()
{
    if (!sdAvailable || !isInternetAvailable())
        return;

    File root = SD_MMC.open("/");

    if (!root)
        return;

    File file = root.openNextFile();

    while (file)
    {
        String fileName = file.name();

        if (!file.isDirectory())
        {
            size_t size = file.size();

            uint8_t* buffer =
                (uint8_t*)malloc(size);

            if (buffer)
            {
                file.read(buffer, size);

                if (photoBuffer != NULL)
                    free(photoBuffer);

                photoBuffer = buffer;
                photoSize = size;

                bool sent = uploadGoogleDrive();

                if (sent)
                {
                    SD_MMC.remove(fileName);
                    Serial.println(
                        "File synchronized"
                    );
                }

                free(photoBuffer);

                photoBuffer = NULL;
                photoSize = 0;
            }
        }

        file = root.openNextFile();
    }
}

bool sendTelegram()
{
    if (!photoBuffer || photoSize == 0)
    {
        Serial.println("No photo available");
        return false;
    }

    telegramClient.setInsecure();

    Serial.println("Connecting to Telegram...");

    if (!telegramClient.connect(
            "api.telegram.org",
            443))
    {
        Serial.println(
            "Telegram connection failed"
        );

        return false;
    }

    String boundary =
        "----ESP32CAMBoundary";

    String head =
        "--" + boundary + "\r\n";

    head +=
        "Content-Disposition: form-data; "
        "name=\"chat_id\"\r\n\r\n";

    head += String(chatID);
    head += "\r\n";

    head +=
        "--" + boundary + "\r\n";

    head +=
        "Content-Disposition: form-data; "
        "name=\"photo\"; filename=\"photo.jpg\"\r\n";

    head +=
        "Content-Type: image/jpeg\r\n\r\n";

    String tail =
        "\r\n--" + boundary + "--\r\n";

    size_t contentLength =
        head.length() +
        photoSize +
        tail.length();

    telegramClient.println(
        "POST /bot" +
        String(botToken) +
        "/sendPhoto HTTP/1.1"
    );

    telegramClient.println(
        "Host: api.telegram.org"
    );

    telegramClient.println(
        "Content-Type: multipart/form-data; boundary=" +
        boundary
    );

    telegramClient.println(
        "Content-Length: " +
        String(contentLength)
    );

    telegramClient.println(
        "Connection: close"
    );

    telegramClient.println();

    telegramClient.print(head);

    size_t totalSent = 0;
    const size_t chunkSize = 2048;

    while (totalSent < photoSize)
    {
        size_t remaining =
            photoSize - totalSent;

        size_t currentChunk =
            remaining > chunkSize
                ? chunkSize
                : remaining;

        size_t sent =
            telegramClient.write(
                photoBuffer + totalSent,
                currentChunk
            );

        if (sent == 0)
        {
            Serial.println(
                "Error sending photo chunk"
            );

            telegramClient.stop();
            return false;
        }

        totalSent += sent;
    }

    telegramClient.print(tail);

    unsigned long timeout = millis();

    while (!telegramClient.available())
    {
        if (millis() - timeout > 35000)
        {
            Serial.println(
                "Telegram timeout"
            );

            telegramClient.stop();
            return false;
        }

        delay(10);
    }

    String response = "";

    while (telegramClient.available())
    {
        response +=
            (char)telegramClient.read();
    }

    telegramClient.stop();

    if (response.indexOf("\"ok\":true") >= 0)
    {
        Serial.println(
            "Telegram: PHOTO SENT SUCCESSFULLY"
        );

        return true;
    }

    Serial.println(
        "Telegram: ERROR in response"
    );

    return false;
}

void handlePhoto()
{
    if (!photoBuffer)
    {
        server.send(
            404,
            "text/plain",
            "No photo"
        );

        return;
    }

    server.sendHeader(
        "Content-Type",
        "image/jpeg"
    );

    server.sendHeader(
        "Content-Disposition",
        "attachment; filename=photo.jpg"
    );

    server.send_P(
        200,
        "image/jpeg",
        (const char*)photoBuffer,
        photoSize
    );
}

void handleRoot()
{
    String html =
        "<h1>ESP32 CAM</h1>";

    html +=
        "<a href='/photo'>Download last photo</a>";

    server.send(
        200,
        "text/html",
        html
    );
}

void setup()
{
    Serial.begin(115200);

    delay(1000);

    pinMode(PIR_PIN, INPUT);

    pixel.begin();
    pixel.setBrightness(255);
    pixel.clear();
    pixel.show();

    configurePowerManagement();

    connectWiFi();

    initCamera();

    initSD();

    server.on("/", handleRoot);
    server.on("/photo", handlePhoto);

    server.begin();

    motionSemaphore =
        xSemaphoreCreateBinary();

    if (motionSemaphore == NULL)
    {
        Serial.println(
            "Semaphore creation error"
        );

        ESP.restart();
    }

    xTaskCreatePinnedToCore(
        monitorTask,
        "Monitor",
        8192,
        NULL,
        1,
        NULL,
        0
    );

    xTaskCreatePinnedToCore(
        cameraProcessTask,
        "Camera",
        8192,
        NULL,
        2,
        NULL,
        1
    );

    Serial.println(
        "System initialized."
    );
}

void loop()
{
    vTaskDelay(portMAX_DELAY);
}

extern "C" void app_main()
{
    initArduino();
    setup();

    while (true)
    {
        loop();
    }
}
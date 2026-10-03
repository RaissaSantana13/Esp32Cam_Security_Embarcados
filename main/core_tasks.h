#ifndef CORE_TASKS_H
#define CORE_TASKS_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#include "esp_camera.h"

#include <WiFi.h>
#include <WebServer.h>
#include <WiFiClientSecure.h>

#include "FS.h"
#include "SD_MMC.h"
#include "base64.h"

#include <Adafruit_NeoPixel.h>

#define PIR_PIN 42
#define LED_FLASH 48
#define NUM_PIXELS 1

#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      15
#define SIOD_GPIO_NUM       4
#define SIOC_GPIO_NUM       5

#define Y9_GPIO_NUM        16
#define Y8_GPIO_NUM        17
#define Y7_GPIO_NUM        18
#define Y6_GPIO_NUM        12
#define Y5_GPIO_NUM        10
#define Y4_GPIO_NUM         8
#define Y3_GPIO_NUM         9
#define Y2_GPIO_NUM        11

#define VSYNC_GPIO_NUM      6
#define HREF_GPIO_NUM       7
#define PCLK_GPIO_NUM      13

extern Adafruit_NeoPixel pixel;

extern const char* ssid;
extern const char* password;
extern const char* botToken;
extern const char* chatID;
extern String googleScript;

extern unsigned long lastMotionTime;
extern const unsigned long cooldown;

extern bool previousPIRState;
extern bool sdAvailable;

extern uint8_t* photoBuffer;
extern size_t photoSize;

extern WebServer server;
extern WiFiClientSecure telegramClient;

extern SemaphoreHandle_t motionSemaphore;

void initSD();
void initCamera();
void connectWiFi();

void capturePhotoRAM();
void savePhotoSD();

String urlencode(String str);

bool uploadGoogleDrive();
bool sendTelegram();

void syncSD();

void handlePhoto();
void handleRoot();

void monitorTask(void* pvParameters);
void cameraProcessTask(void* pvParameters);

#endif
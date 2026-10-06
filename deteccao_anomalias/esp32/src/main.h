#ifndef MAIN_H
#define MAIN_H

// Bibliotecas padrão
#include <SPI.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include "ICM42688P.h"

// Constantes
#define PAYLOAD_SIZE 350000
#define BUTTON_PIN 33
#define SCK_PIN 20
#define MISO_PIN 7
#define MOSI_PIN 19
#define SS_PIN 5
#define ICM_INT1_PIN 35
#define ICM_INT2_PIN 34

// Definições do servidor e rede
extern const char *ssid;
extern const char *password;
extern const char *serverAddress;
extern const int serverPort;

extern IPAddress local_IP;
extern IPAddress gateway;
extern IPAddress subnet;
extern const char* hostname;

// Variáveis globais
extern WiFiClient client;
extern wifi_bandwidth_t bw;
extern uint8_t *byteBuffer;
extern size_t byteBufferSize;
extern const size_t packetSize;
extern uint32_t samplingPeriod;
extern ulong conectionTimer;
extern bool conected;

// Instância do sensor
extern ICM42688P IIM;

// Enumerações (Tipos)
typedef enum {
    _16G = 0,
    _8G,
    _4G,
    _2G
} Sensitivity_S;

typedef enum {
    _32K = 1,
    _16K,
    _8K,
    _4K,
    _2K,
    _1K,
    _200,
    _100,
    _50,
    _25,
    _12,
    _6,
    _3,
    _1,
    _500
} Frequency_S;
typedef enum {
    SIF_WAKEUP_TIMER = 0,
    SIF_WAKEUP_GPIO,
    SIF_NORMAL_OPERATION,
    WAKE_UP_TIME_MICROSECONDS = (int)15e6
} WakeupStage_S;

// Protótipos das funções globais
void initializeFileInPSRAM();
void sendSensorData(int16_t *data, size_t dataSize);
void createSin(int16_t *data, size_t dataSize);
void sendDataUDP(int16_t *data, size_t dataSize);
void buttonTask(void *parameter);

#endif // MAIN_H

#include "main.h"
#include "ICM42688P.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/soc.h"
#include <SPI.h>

// definições
#ifndef SCK_PIN
#define SCK_PIN 20
#endif
#ifndef MISO_PIN
#define MISO_PIN 7
#endif
#ifndef MOSI_PIN
#define MOSI_PIN 19
#endif
#ifndef SS_PIN
#define SS_PIN 5
#endif
#ifndef BUTTON_PIN
#define BUTTON_PIN 33
#endif

// configurações de rede
const char *ssid = "dd-wrt";       
const char *password = NULL;       

// ip receptor dos dados (servidor)
IPAddress remote_ip(192, 168, 1, 133); 
const int serverPort = 12345;

// globais
uint8_t *byteBuffer;
const size_t MAX_SAFE_BUFFER = 2000000; 
size_t byteBufferSize; 
size_t maxSamples;

const size_t packetSize = 1024; 

ICM42688P IIM(SPI);
WiFiClient client;
TaskHandle_t collectionTaskHandle = NULL;

typedef enum {
    GYRO_2000DPS = 0,
    GYRO_1000DPS,
    GYRO_500DPS,
    GYRO_250DPS,
    GYRO_125DPS,
    GYRO_62DPS,
    GYRO_31DPS,
    GYRO_15DPS
} GyroRange_S;

const Frequency_S SENSOR_ODR = _500;
const Sensitivity_S ACCEL_RANGE = _4G;
const GyroRange_S GYRO_RANGE = GYRO_1000DPS;
const uint32_t SPI_CONFIG_CLOCK_HZ = 1000000;
const uint32_t SPI_BURST_CLOCK_HZ = 8000000;

uint32_t odrCodeToHz(Frequency_S odr) {
    switch (odr) {
        case _32K: return 32000;
        case _16K: return 16000;
        case _8K:  return 8000;
        case _4K:  return 4000;
        case _2K:  return 2000;
        case _1K:  return 1000;
        case _500: return 500;
        case _200: return 200;
        case _100: return 100;
        case _50:  return 50;
        case _25:  return 25;
        case _12:  return 12;
        case _6:   return 6;
        case _3:   return 3;
        case _1:   return 1;
        default:   return 100;
    }
}

uint8_t buildAccelConfig0(Sensitivity_S range, Frequency_S odr) {
    return ((uint8_t(range) & 0x07) << 5) | (uint8_t(odr) & 0x0F);
}

uint8_t buildGyroConfig0(GyroRange_S range, Frequency_S odr) {
    return ((uint8_t(range) & 0x07) << 5) | (uint8_t(odr) & 0x0F);
}

uint16_t accelRangeToG(Sensitivity_S range) {
    switch (range) {
        case _16G: return 16;
        case _8G:  return 8;
        case _4G:  return 4;
        case _2G:  return 2;
        default:   return 16;
    }
}

float gyroRangeToDps(GyroRange_S range) {
    switch (range) {
        case GYRO_2000DPS: return 2000.0f;
        case GYRO_1000DPS: return 1000.0f;
        case GYRO_500DPS:  return 500.0f;
        case GYRO_250DPS:  return 250.0f;
        case GYRO_125DPS:  return 125.0f;
        case GYRO_62DPS:   return 62.5f;
        case GYRO_31DPS:   return 31.25f;
        case GYRO_15DPS:   return 15.625f;
        default:           return 2000.0f;
    }
}

const uint32_t ODR_HZ = odrCodeToHz(SENSOR_ODR);
const uint32_t SAMPLE_PERIOD_US = 1000000UL / ODR_HZ; 

// registradores
#define FIFO_CONFIG_REG 0x16
#define FIFO_CONFIG1_REG 0x5F
#define FIFO_COUNT_H_REG 0x2E
#define FIFO_DATA_REG 0x30
#define INT_STATUS_REG 0x2D
#define SIGNAL_PATH_RESET_REG 0x4B 
#define GYRO_CONFIG0_REG 0x4F 

void initializePSRAM();
void setupWiFi();
void collectionTask(void *parameter);
void sendDataOverTCP();

bool configureFIFOforPacket2_Manual();
void resetFIFO_Manual();
void readFIFOPackets_Manual(uint8_t *buf, uint32_t maxPackets, uint32_t *packetsRead);
void spi_write_reg(uint8_t reg, uint8_t data);
uint8_t spi_read_reg(uint8_t reg);
void spi_read_burst(uint8_t reg, uint8_t *data, uint32_t len);

void setup() {
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
    Serial.begin(115200);
    delay(1000);
    Serial.println("\nIniciando setup");

    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(SS_PIN, OUTPUT);
    digitalWrite(SS_PIN, HIGH);
    pinMode(2, OUTPUT);
    digitalWrite(2, LOW);
    
    #ifdef ICM_INT1_PIN
    pinMode(ICM_INT1_PIN, INPUT);
    #endif

    Serial.println("Inicializando sensor ICM42688P");
    IIM.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
    
    IIM.wakeup(ACCEL_RANGE, SENSOR_ODR); 
    delay(20);

    spi_write_reg(GYRO_CONFIG0_REG, buildGyroConfig0(GYRO_RANGE, SENSOR_ODR));
    delay(20);

    if (!configureFIFOforPacket2_Manual()) {
        Serial.println("Erro ao configurar FIFO");
        while (1) delay(1000);
    }
    Serial.printf("Sensor OK (ODR=%lu Hz, accel=+/-%ug, gyro=+/-%.3fdps)\n", (unsigned long)ODR_HZ, accelRangeToG(ACCEL_RANGE), gyroRangeToDps(GYRO_RANGE));

    initializePSRAM();
    setupWiFi(); 

    xTaskCreatePinnedToCore(
        collectionTask,
        "Collection Task",
        8192,
        NULL,
        1,
        &collectionTaskHandle,
        1 
    );

    Serial.println("Setup concluido");
}

void loop() {
    if (digitalRead(BUTTON_PIN) == LOW) {
        Serial.println("ímã detectado. Parando coleta e enviando...");
        
        IIM.stopFlag = true; 
        delay(100); 

        sendDataOverTCP();

        IIM.head = 0;
        IIM.tail = 0;
        
        resetFIFO_Manual();
        Serial.println("FIFO resetado.");

        IIM.stopFlag = false;
        Serial.println("\nPronto para nova coleta.");
        
        delay(2000); 
    }
    delay(100);
}

// coleta (big endian)
void collectionTask(void *parameter) {
    Serial.println("Tarefa de coleta iniciada.");
    const uint32_t maxPacketsPerRead = 64; 
    const uint8_t packetSizeFIFO = 16; 
    uint8_t fifoBuf[packetSizeFIFO * maxPacketsPerRead];
    uint32_t packetsRead = 0;
    
    uint32_t sampleCountTotal = 0;
    bool firstRun = true;

    for (;;) {
        if (IIM.stopFlag || (IIM.head + 18 > byteBufferSize)) {
            firstRun = true; 
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        readFIFOPackets_Manual(fifoBuf, maxPacketsPerRead, &packetsRead);
        
        if (packetsRead == 0) {
            vTaskDelay(pdMS_TO_TICKS(1));
            continue;
        }

        if (firstRun) {
            sampleCountTotal = 0;
            firstRun = false;
        }

        for (uint32_t p = 0; p < packetsRead; p++) {
            if (IIM.head + 18 > byteBufferSize) break;

            uint8_t *pkt = &fifoBuf[p * packetSizeFIFO];

            // big endian para little endian (int16_t)
            int16_t ax = (int16_t)((pkt[1] << 8) | pkt[2]);
            int16_t ay = (int16_t)((pkt[3] << 8) | pkt[4]);
            int16_t az = (int16_t)((pkt[5] << 8) | pkt[6]);

            int16_t gx = (int16_t)((pkt[7] << 8) | pkt[8]);
            int16_t gy = (int16_t)((pkt[9] << 8) | pkt[10]);
            int16_t gz = (int16_t)((pkt[11] << 8) | pkt[12]);
            
            // temperatura (byte 13)
            int8_t temp8 = (int8_t)pkt[13]; 
            int16_t temp = (int16_t)temp8; 

            // timestamp calculado
            uint32_t calculated_ts = sampleCountTotal * SAMPLE_PERIOD_US;

            size_t pos = IIM.head;
            memcpy(&byteBuffer[pos +  0], &ax, 2);
            memcpy(&byteBuffer[pos +  2], &ay, 2);
            memcpy(&byteBuffer[pos +  4], &az, 2);
            memcpy(&byteBuffer[pos +  6], &gx, 2);
            memcpy(&byteBuffer[pos +  8], &gy, 2);
            memcpy(&byteBuffer[pos + 10], &gz, 2);
            memcpy(&byteBuffer[pos + 12], &temp, 2);
            memcpy(&byteBuffer[pos + 14], &calculated_ts, 4);

            IIM.head += 18;
            sampleCountTotal++;
        }
    }
}

// envio tcp
void sendDataOverTCP() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi desconectado. Tentando reconectar...");
        WiFi.reconnect();
        unsigned long tRecon = millis();
        while (WiFi.status() != WL_CONNECTED && (millis() - tRecon < 5000)) delay(100);
        if (WiFi.status() != WL_CONNECTED) return;
    }

    Serial.print("Conectando a "); Serial.println(remote_ip);
    
    if (!client.connect(remote_ip, serverPort)) {
        Serial.println("Falha na conexao TCP.");
        return;
    }

    Serial.println("Conectado! Enviando...");

    size_t totalBytes = IIM.head;
    if (totalBytes == 0) {
        client.stop();
        return;
    }

    uint32_t headerSz = (uint32_t)totalBytes;
    client.write((const uint8_t*)&headerSz, 4);

    size_t bytesSentTotal = 0;
    while (bytesSentTotal < totalBytes) {
        size_t toSend = min(packetSize, totalBytes - bytesSentTotal);
        int w = client.write(&byteBuffer[bytesSentTotal], toSend);
        if (w > 0) bytesSentTotal += w;
        else break;
    }
    Serial.printf("Enviado: %d bytes.\n", bytesSentTotal);
    client.stop();
}

// auxiliares
void initializePSRAM() {
    Serial.println("Alocando Buffer...");
    byteBufferSize = MAX_SAFE_BUFFER; 
    maxSamples = byteBufferSize / 18; 
    byteBufferSize = maxSamples * 18;

    byteBuffer = (uint8_t *)ps_malloc(byteBufferSize);
    if (byteBuffer == nullptr) {
        byteBufferSize = 50000; 
        byteBuffer = (uint8_t *)malloc(byteBufferSize);
        if (byteBuffer == nullptr) while (1);
    }
}

void setupWiFi() {
    Serial.print("Conectando WiFi: "); Serial.println(ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    unsigned long startWait = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - startWait < 15000)) {
        delay(500); Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("\nIP Local: "); Serial.println(WiFi.localIP());
    } else {
        Serial.println("\nFalha WiFi.");
    }
}

void spi_write_reg(uint8_t reg, uint8_t data) {
    SPI.beginTransaction(SPISettings(SPI_CONFIG_CLOCK_HZ, MSBFIRST, SPI_MODE0));
    digitalWrite(SS_PIN, LOW);
    SPI.transfer(reg);
    SPI.transfer(data);
    digitalWrite(SS_PIN, HIGH);
    SPI.endTransaction();
}

uint8_t spi_read_reg(uint8_t reg) {
    SPI.beginTransaction(SPISettings(SPI_CONFIG_CLOCK_HZ, MSBFIRST, SPI_MODE0));
    digitalWrite(SS_PIN, LOW);
    SPI.transfer(reg | 0x80);
    uint8_t val = SPI.transfer(0x00);
    digitalWrite(SS_PIN, HIGH);
    SPI.endTransaction();
    return val;
}

void spi_read_burst(uint8_t reg, uint8_t *data, uint32_t len) {
    SPI.beginTransaction(SPISettings(SPI_BURST_CLOCK_HZ, MSBFIRST, SPI_MODE0));
    digitalWrite(SS_PIN, LOW);
    SPI.transfer(reg | 0x80);
    for (uint32_t i = 0; i < len; i++) data[i] = SPI.transfer(0x00);
    digitalWrite(SS_PIN, HIGH);
    SPI.endTransaction();
}

bool configureFIFOforPacket2_Manual() {
    uint8_t fifo_config = spi_read_reg(FIFO_CONFIG_REG);
    fifo_config = (fifo_config & 0x3F) | (0x40); 
    spi_write_reg(FIFO_CONFIG_REG, fifo_config);
    spi_write_reg(FIFO_CONFIG1_REG, 0x0F);
    return true;
}

void resetFIFO_Manual() {
    spi_write_reg(SIGNAL_PATH_RESET_REG, 0x04); 
    delay(5);
}

void readFIFOPackets_Manual(uint8_t *buf, uint32_t maxPackets, uint32_t *packetsRead) {
    uint8_t h = spi_read_reg(FIFO_COUNT_H_REG);
    uint8_t l = spi_read_reg(FIFO_COUNT_H_REG + 1);
    uint16_t count = (h << 8) | l;
    if (count == 0) { *packetsRead = 0; return; }

    uint32_t packetsAvailable = count / 16;
    uint32_t toRead = (packetsAvailable > maxPackets) ? maxPackets : packetsAvailable;
    
    if (toRead > 0) spi_read_burst(FIFO_DATA_REG, buf, toRead * 16);
    *packetsRead = toRead;
}
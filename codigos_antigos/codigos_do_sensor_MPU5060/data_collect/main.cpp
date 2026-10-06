#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <vector>

#define REED_SWITCH_PIN 4
#define I2C_SDA 21
#define I2C_SCL 22

// Bytes of the Static IP for ESP
#define IP01 192
#define IP02 168
#define IP03 4
#define IP04 1
#define IP04GateWay 1

// MPU Variables
#define MPUaddress (0b1101000)

// Controles de coleta
// garante coleta de 100Hz por 30 segundos
const int MAX_SAMPLES_IN_BUFFER = 3000;
const int MAX_SAMPLES_PER_PACKET = 30;
const int TARGET_SAMPLING_FREQUENCY_HZ = 100;
const int SAMPLING_INTERVAL_MS = 1000 / TARGET_SAMPLING_FREQUENCY_HZ;

// Data structure for a single measurement point
struct SensorDataPoint {
    unsigned long timestamp_us;
    float rotX, rotY, rotZ;
    float gForceX, gForceY, gForceZ;
};

// Global variables
std::vector<SensorDataPoint> dataBuffer;
unsigned long collectionStartTime_us;
unsigned long lastSampleTime = 0;
bool bufferFullMessageSent = false;

// UDP network
const char* net_ssid = "ESTEIRA";
const char* net_pass = "12345678";
WiFiUDP Udp;
unsigned int localPort = 8888;
IPAddress   DestinationIP;
uint16_t    PCudpPort;
boolean     IpREceived = false; 

// Measurements
long accelX, accelY, accelZ;
float gForceX, gForceY, gForceZ;
long gyroX, gyroY, gyroZ;
long gyroX_bias=0, gyroY_bias=0, gyroZ_bias=0;
long gyroXCalibrated, gyroYCalibrated, gyroZCalibrated;
float rotX, rotY, rotZ;

// Function Prototypes
void SetWifi(const char* Name, const char* Password);
void setupMPU();
void calibrateGyroForBias();
void recordAndProcessSensorData();
long checkOverFlow(long x);
void collectData();
void sendAndClearDataBuffer();
int serializeFloat(byte* buffer, int index, float value);
int serializeUlong(byte* buffer, int index, unsigned long value);
int serializeUint16(byte* buffer, int index, uint16_t value);
int serializeUint32(byte* buffer, int index, uint32_t value);

// ************************************************

void setup() {
  Serial.begin(115200);
  pinMode(REED_SWITCH_PIN, INPUT_PULLUP);
  
  // inicializa Wifi e sensores
  SetWifi(net_ssid, net_pass);
  Wire.begin(I2C_SDA, I2C_SCL, 400000);
  setupMPU();
  delay(500);
  
  calibrateGyroForBias();
  
  dataBuffer.reserve(MAX_SAMPLES_IN_BUFFER); // Reserva tam máximo do buffer

  collectionStartTime_us = micros();
}

// **************************************************

void loop() {
  // Aguarda a conexão do cliente
  if (IpREceived == false) {
    int packetSize = Udp.parsePacket(); // Verifica se chegou algum pacote
    if (packetSize) { // Se sim, muda estado para recebido
        DestinationIP = Udp.remoteIP();
        PCudpPort = Udp.remotePort();
        IpREceived = true;
        collectionStartTime_us = micros(); // Inicia o timer da coleta AGORA
    }
    delay(100); 
    return; 
  }

  // Coleta e envio de dados

  int currentReedState = digitalRead(REED_SWITCH_PIN);

  if (currentReedState == LOW) { // Se o ímã for detectado
      if (!dataBuffer.empty()) {
        sendAndClearDataBuffer();
        lastSampleTime = 0; 
        bufferFullMessageSent = false; // Reseta o sinalizador
      }
  } 
  else { // Se não houver ímã
    if (millis() - lastSampleTime >= SAMPLING_INTERVAL_MS) { // Para garantir a taxa de amostragem
      lastSampleTime = millis();

      if (dataBuffer.size() < MAX_SAMPLES_IN_BUFFER) { // Para evitar estouro do buffer
        recordAndProcessSensorData();
        collectData();
      } else { 
        if (!bufferFullMessageSent) { // Para imprimir a mensagem somente uma vez
           bufferFullMessageSent = true;
        }
      }
    }
  }
}

// **************************************************

void SetWifi(const char* Name, const char* Password) { // Config e inicializa o Wifi
    WiFi.disconnect(true);
    WiFi.mode(WIFI_AP);
    IPAddress ip(IP01, IP02, IP03, IP04);
    IPAddress gateway(IP01, IP02, IP03, IP04GateWay);
    IPAddress subnet(255, 255, 255, 0);
    WiFi.softAPConfig(ip, gateway, subnet);
    
    if (WiFi.softAP(Name, Password, 6)) {
    } else {
        if (Serial) Serial.println("Falha ao criar a rede Wi-Fi.");
    }
    
    delay(2000);
    if (Serial) Serial.print("IP do Servidor (ESP32): "); if (Serial) Serial.println(WiFi.softAPIP());
    Udp.begin(localPort);
}

void setupMPU() { // Configura sensores
    Wire.beginTransmission(MPUaddress); Wire.write(0x6B); Wire.write(0b00000000); Wire.endTransmission();
    Wire.beginTransmission(MPUaddress); Wire.write(0x1B); Wire.write(0b00000000); Wire.endTransmission();
    Wire.beginTransmission(MPUaddress); Wire.write(0x1C); Wire.write(0b00000000); Wire.endTransmission();
}

void recordAndProcessSensorData() { // Lê dados brutos e processa
    // Dados do acelerômetro
    Wire.beginTransmission(MPUaddress); Wire.write(0x3B); Wire.endTransmission();
    Wire.requestFrom(MPUaddress, 6);
    if(Wire.available() >= 6) {
        accelX = Wire.read() << 8 | Wire.read(); accelX = checkOverFlow(accelX);
        accelY = Wire.read() << 8 | Wire.read(); accelY = checkOverFlow(accelY);
        accelZ = Wire.read() << 8 | Wire.read(); accelZ = checkOverFlow(accelZ);
    }
    gForceX = accelX / 16384.0f; gForceY = accelY / 16384.0f; gForceZ = accelZ / 16384.0f;
    
    // Dados do giroscópio
    Wire.beginTransmission(MPUaddress); Wire.write(0x43); Wire.endTransmission();
    Wire.requestFrom(MPUaddress, 6);
    if(Wire.available() >= 6) {
        gyroX = Wire.read() << 8 | Wire.read(); gyroX = checkOverFlow(gyroX);
        gyroY = Wire.read() << 8 | Wire.read(); gyroY = checkOverFlow(gyroY);
        gyroZ = Wire.read() << 8 | Wire.read(); gyroZ = checkOverFlow(gyroZ);
    }
    
    gyroXCalibrated = gyroX - gyroX_bias;
    gyroYCalibrated = gyroY - gyroY_bias;
    gyroZCalibrated = gyroZ - gyroZ_bias;

    rotX = gyroXCalibrated / 131.0f; rotY = gyroYCalibrated / 131.0f; rotZ = gyroZCalibrated / 131.0f;
}

// Mede bias do giroscópio
void calibrateGyroForBias() {
    long gyroX_sum=0, gyroY_sum=0, gyroZ_sum=0;
    int daCount=0;
    long tStart=millis();
    
    while((millis()-tStart) < 2000) {
        Wire.beginTransmission(MPUaddress); Wire.write(0x43); Wire.endTransmission();
        Wire.requestFrom(MPUaddress, 6);
        if(Wire.available() >= 6) {
            gyroX_sum += checkOverFlow(Wire.read()<<8|Wire.read());
            gyroY_sum += checkOverFlow(Wire.read()<<8|Wire.read());
            gyroZ_sum += checkOverFlow(Wire.read()<<8|Wire.read());
            daCount++;
        }
        delay(3);
    }

    if (daCount > 0) {
        gyroX_bias = gyroX_sum / daCount;
        gyroY_bias = gyroY_sum / daCount;
        gyroZ_bias = gyroZ_sum / daCount;
    }
}

long checkOverFlow(long x) { 
  if(x > 32767) return x - 65536; 
  return x; 
}

void collectData() {
    SensorDataPoint newPoint;
    newPoint.timestamp_us = micros() - collectionStartTime_us;
    newPoint.rotX = rotX; newPoint.rotY = rotY; newPoint.rotZ = rotZ;
    newPoint.gForceX = gForceX; newPoint.gForceY = gForceY; newPoint.gForceZ = gForceZ;
    dataBuffer.push_back(newPoint);
}

void sendAndClearDataBuffer() { 
    // Pega o buffer grande e o quebra em pacotes menores
    if (dataBuffer.empty()){ // Se não houver dados
        return;
    }
    
    if(!IpREceived) { // Se não houver cliente conectado
        dataBuffer.clear();
        collectionStartTime_us = micros();
        return;
    }
    
    size_t totalSamples = dataBuffer.size();
    // Calcula o número de pacotes necessários
    uint16_t totalPackets = (totalSamples + MAX_SAMPLES_PER_PACKET - 1) / MAX_SAMPLES_PER_PACKET;
    // Gera um ID para o envio
    uint32_t transmissionId = esp_random();
    
    if (Serial) Serial.print("Preparando para enviar "); if (Serial) Serial.print(totalSamples);
    if (Serial) Serial.print(" amostras em "); if (Serial) Serial.print(totalPackets); if (Serial) Serial.println(" pacotes.");

    // Aloca buffer temporario para montar cada pacote
    size_t maxPayloadSize = 10 + (MAX_SAMPLES_PER_PACKET * 28); // cabeçalho + dados
    byte* udpPayload = new byte[maxPayloadSize];

    size_t samplesSent = 0;
    // no loop monta e envia cada pacote
    for (uint16_t packetNum = 0; packetNum < totalPackets; ++packetNum) {
        size_t samplesInThisPacket = min((size_t)MAX_SAMPLES_PER_PACKET, totalSamples - samplesSent);
        int byteCount = 0;
        // cabeçalho
        byteCount = serializeUint32(udpPayload, byteCount, transmissionId);
        byteCount = serializeUint16(udpPayload, byteCount, packetNum + 1);
        byteCount = serializeUint16(udpPayload, byteCount, totalPackets);
        byteCount = serializeUint16(udpPayload, byteCount, (uint16_t)samplesInThisPacket);
        // dados
        for (size_t i = 0; i < samplesInThisPacket; ++i) {
            const auto& point = dataBuffer[samplesSent + i];
            byteCount = serializeUlong(udpPayload, byteCount, point.timestamp_us);
            byteCount = serializeFloat(udpPayload, byteCount, point.rotX);
            byteCount = serializeFloat(udpPayload, byteCount, point.rotY);
            byteCount = serializeFloat(udpPayload, byteCount, point.rotZ);
            byteCount = serializeFloat(udpPayload, byteCount, point.gForceX);
            byteCount = serializeFloat(udpPayload, byteCount, point.gForceY);
            byteCount = serializeFloat(udpPayload, byteCount, point.gForceZ);
        }

        // Envia o pacote
        Udp.beginPacket(DestinationIP, PCudpPort);
        Udp.write(udpPayload, byteCount);
        Udp.endPacket();
        
        samplesSent += samplesInThisPacket;
        delay(5);
    }
    delete[] udpPayload; // libera o buffer temporário

    dataBuffer.clear(); // Limpa o buffer de principal
    collectionStartTime_us = micros();
}

// As funções a seguir convertem os dados do formato do ESP (little-endian) para o padrão de rede (big-endian) para envio via UDP

int serializeFloat(byte* buffer, int index, float value) {
    byte* pointer = (byte*)&value;
    buffer[index + 0] = pointer[3]; buffer[index + 1] = pointer[2];
    buffer[index + 2] = pointer[1]; buffer[index + 3] = pointer[0];
    return index + 4;
}
int serializeUlong(byte* buffer, int index, unsigned long value) {
    byte* pointer = (byte*)&value;
    buffer[index + 0] = pointer[3]; buffer[index + 1] = pointer[2];
    buffer[index + 2] = pointer[1]; buffer[index + 3] = pointer[0];
    return index + 4;
}
int serializeUint16(byte* buffer, int index, uint16_t value) {
    buffer[index]   = (value >> 8) & 0xFF;
    buffer[index+1] = value & 0xFF;
    return index + 2;
}
int serializeUint32(byte* buffer, int index, uint32_t value) {
    buffer[index]   = (value >> 24) & 0xFF;
    buffer[index+1] = (value >> 16) & 0xFF;
    buffer[index+2] = (value >> 8) & 0xFF;
    buffer[index+3] = value & 0xFF;
    return index + 4;
}
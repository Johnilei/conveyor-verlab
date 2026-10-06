#include "ICM42688P.h"

#include "Arduino.h"

#define SPI_READ 0x80
#define SPI_CLOCK 1000000  //1MHz para baixo clock 16MHZ para alto clock

bool _useSPI = false;
static SPIClass *spi = NULL;

unsigned long samplingsTimestamp;

static int spi_write(uint8_t reg, const uint8_t *wbuffer, uint32_t wlen);
static int spi_read(uint8_t reg, uint8_t *rbuffer, uint32_t rlen);

uint16_t convertFrom8To16(uint8_t dataFirst, uint8_t dataSecond) {
    uint16_t dataBoth = 0x0000;

    dataBoth = dataFirst;
    dataBoth = dataBoth << 8;
    dataBoth |= dataSecond;
    return dataBoth;
}

ICM42688P::ICM42688P(SPIClass &spi_ref) {
    spi = &spi_ref;
}

int ICM42688P::begin(int8_t sck, int8_t miso, int8_t mosi, int8_t ss) {
    // uint8_t who_am_i;

    spi->begin(sck,miso,mosi,ss);

    // pin_function(PIN_CS, STM_PIN_DATA(STM_MODE_OUTPUT_PP, GPIO_NOPULL, 0));
    // LL_GPIO_SetOutputPin(PIN_CS_PORT, PIN_CS_LL);

    // pin_function(PIN_INT2, STM_PIN_DATA(STM_MODE_OUTPUT_PP, GPIO_NOPULL, 0));
    // LL_GPIO_ResetOutputPin(PIN_INT2_PORT, PIN_INT2_LL);

    uint8_t pwrMgmt0 = PWR_MGMT0_RESET;
    uint8_t acellConfig0 = ACCEL_CONFIG0_SETUP;
    uint8_t gyroConfig0 = GYRO_CONFIG0_SETUP;
    uint8_t fifoConfig = 0b11000000;
    uint8_t fifoConfig1 = FIFO_CONFIG1_SETUP;
    uint8_t intfConfig0 = INTF_CONFIG0_SETUP;
    uint8_t intfConfig1 = INTF_CONFIG1_SETUP;
    uint8_t intfConfig5 = INTF_CONFIG5_SETUP;
    uint8_t accelFiltConfig = ACCEL_FILT_CONFIG_SETUP_1BW;

    spi_write(PWR_MGMT0_REG, &pwrMgmt0, 1);
    spi_write(ACCEL_CONFIG0_REG, &acellConfig0, 1);
    spi_write(GYRO_CONFIG0_REG, &gyroConfig0, 1);
    spi_write(FIFO_CONFIG_REG, &fifoConfig, 1);
    spi_write(FIFO_CONFIG1_REG, &fifoConfig1, 1);
    spi_write(INTF_CONFIG0_REG, &intfConfig0, 1);
    spi_write(INTF_CONFIG1_REG, &intfConfig1, 1);
    spi_write(INTF_CONFIG5_REG, &intfConfig5, 1);
    spi_write(ACCEL_FILT_CONFIG_REG, &accelFiltConfig, 1);

    return 1;
}

void ICM42688P::wakeup(uint8_t acellFactor, uint8_t frequencyFactor) {
    uint8_t pwrMgmt0 = PWR_MGMT0_SETUP_LN;
    uint8_t acellConfig0 = 0x00;
    // uint8_t acellConfig0 = ACCEL_CONFIG0_SETUP;
    uint8_t intfConfig0 = INTF_CONFIG0_SETUP;
    uint8_t intfConfig1 = INTF_CONFIG1_SETUP;
    uint8_t intfConfig5 = INTF_CONFIG5_SETUP;
    uint8_t accelFiltConfig = ACCEL_FILT_CONFIG_SETUP_1BW;
    acellConfig0 |= acellFactor;
    acellConfig0 = ((acellFactor & 0x07) << 5) | (frequencyFactor & 0x0F);

    spi_write(PWR_MGMT0_REG, &pwrMgmt0, 1);
    spi_write(ACCEL_CONFIG0_REG, &acellConfig0, 1);
    spi_write(INTF_CONFIG0_REG, &intfConfig0, 1);
    spi_write(INTF_CONFIG1_REG, &intfConfig1, 1);
    spi_write(INTF_CONFIG5_REG, &intfConfig5, 1);
    spi_write(ACCEL_FILT_CONFIG_REG, &accelFiltConfig, 1);
    

    delay(10);
}


IIMStatus ICM42688P::sampleAcceleration(int16_t *buf, uint32_t numOfSamples, uint32_t *samplingPeriod, Axis axis) {
    uint32_t accIndex = 0;
    uint8_t accL;
    uint8_t accH;
    int16_t acc;

    uint8_t intStatusMask = DATA_RDY_INT_MASK;
    uint8_t dataHReg;
    uint8_t dataLReg;

    switch (axis) {
        case X_AXIS:
            dataHReg = ACCEL_DATA_X1_REG;
            dataLReg = ACCEL_DATA_X0_REG;
            break;

        case Y_AXIS:
            dataHReg = ACCEL_DATA_Y1_REG;
            dataLReg = ACCEL_DATA_Y0_REG;
            break;

        case Z_AXIS:
            dataHReg = ACCEL_DATA_Z1_REG;
            dataLReg = ACCEL_DATA_Z0_REG;
            break;
    }

    samplingsTimestamp = micros();
    spi->beginTransaction(SPISettings(SPI_CLOCK, MSBFIRST, SPI_MODE0));

    while (accIndex < numOfSamples) {
        spi_read(INT_STATUS_REG, &intStatusMask, 1);

        if (intStatusMask & DATA_RDY_INT_MASK) {
            spi_read(dataHReg, &accH, 1);
            spi_read(dataLReg, &accL, 1);

            acc = (int16_t)convertFrom8To16(accH, accL);

            buf[accIndex] = acc;
            accIndex++;
        } 
    }
    spi->endTransaction();
    samplingsTimestamp = micros() - samplingsTimestamp;
    *samplingPeriod = ((uint32_t)(numOfSamples/((float)samplingsTimestamp/1000000)));
    return IIM_OK;
}

IIMStatus ICM42688P::sampleAcceleration3D(int16_t *buf, uint32_t numOfSamples, uint32_t *samplingPeriod) {
    uint32_t accIndex = 0;
    uint8_t accL;
    uint8_t accH;
    int16_t acc;

    uint8_t intStatusMask = DATA_RDY_INT_MASK;
    uint8_t dataXHReg;
    uint8_t dataXLReg;
    uint8_t dataYHReg;
    uint8_t dataYLReg;
    uint8_t dataZHReg;
    uint8_t dataZLReg;
    
    dataXHReg = ACCEL_DATA_X1_REG;
    dataXLReg = ACCEL_DATA_X0_REG;

    dataYHReg = ACCEL_DATA_Y1_REG;
    dataYLReg = ACCEL_DATA_Y0_REG;

    dataZHReg = ACCEL_DATA_Z1_REG;
    dataZLReg = ACCEL_DATA_Z0_REG;

    samplingsTimestamp = micros();
    spi->beginTransaction(SPISettings(SPI_CLOCK, MSBFIRST, SPI_MODE0));

    while (accIndex < numOfSamples) {
        spi_read(INT_STATUS_REG, &intStatusMask, 1);

        if (intStatusMask & DATA_RDY_INT_MASK) {
            spi_read(dataXHReg, &accH, 1);
            spi_read(dataXLReg, &accL, 1);

            acc = (int16_t)convertFrom8To16(accH, accL);

            buf[accIndex] = acc;
            accIndex++;

            spi_read(dataYHReg, &accH, 1);
            spi_read(dataYLReg, &accL, 1);

            acc = (int16_t)convertFrom8To16(accH, accL);

            buf[accIndex] = acc;
            accIndex++;

            spi_read(dataZHReg, &accH, 1);
            spi_read(dataZLReg, &accL, 1);

            acc = (int16_t)convertFrom8To16(accH, accL);

            buf[accIndex] = acc;
            accIndex++;
        } 
    }
    spi->endTransaction();
    samplingsTimestamp = micros() - samplingsTimestamp;
    *samplingPeriod = ((uint32_t)(numOfSamples/((float)samplingsTimestamp/1000000)));
    return IIM_OK;
}

IIMStatus ICM42688P::sampleAcceleration3Dbyte(uint8_t *buf, uint32_t numOfSamples, uint32_t *samplingPeriod) {
    uint32_t accIndex = 0;
    uint8_t accL;
    uint8_t accH;
    int16_t acc;

    uint8_t intStatusMask = DATA_RDY_INT_MASK;
    uint8_t dataXHReg;
    uint8_t dataXLReg;
    uint8_t dataYHReg;
    uint8_t dataYLReg;
    uint8_t dataZHReg;
    uint8_t dataZLReg;
    
    dataXHReg = ACCEL_DATA_X1_REG;
    dataXLReg = ACCEL_DATA_X0_REG;

    dataYHReg = ACCEL_DATA_Y1_REG;
    dataYLReg = ACCEL_DATA_Y0_REG;

    dataZHReg = ACCEL_DATA_Z1_REG;
    dataZLReg = ACCEL_DATA_Z0_REG;

    samplingsTimestamp = micros();
    spi->beginTransaction(SPISettings(SPI_CLOCK, MSBFIRST, SPI_MODE0));

    while (accIndex < numOfSamples*2) {
        spi_read(INT_STATUS_REG, &intStatusMask, 1);

        if (intStatusMask & DATA_RDY_INT_MASK) {
            spi_read(dataXHReg, &accH, 1);
            spi_read(dataXLReg, &accL, 1);

            //acc = (int16_t)convertFrom8To16(accH, accL);

            buf[accIndex] = accL;
            accIndex++;
            buf[accIndex] = accH;
            accIndex++;

            spi_read(dataYHReg, &accH, 1);
            spi_read(dataYLReg, &accL, 1);

            //acc = (int16_t)convertFrom8To16(accH, accL);

            buf[accIndex] = accL;
            accIndex++;
            buf[accIndex] = accH;
            accIndex++;

            spi_read(dataZHReg, &accH, 1);
            spi_read(dataZLReg, &accL, 1);

            //acc = (int16_t)convertFrom8To16(accH, accL);

            buf[accIndex] = accL;
            accIndex++;
            buf[accIndex] = accH;
            accIndex++;
        } 
    }
    spi->endTransaction();
    samplingsTimestamp = micros() - samplingsTimestamp;
    *samplingPeriod = ((uint32_t)(numOfSamples/((float)samplingsTimestamp/1000000)));
    return IIM_OK;
}


IIMStatus ICM42688P::sampleContinuousAcceleration3Dbyte(uint8_t *buf, uint32_t bufferSize, uint32_t *samplingPeriod) {
    uint8_t accL;
    uint8_t accH;
    uint8_t intStatus;
    
    // Accelerometer data registers
    const uint8_t dataXHReg = ACCEL_DATA_X1_REG;
    const uint8_t dataXLReg = ACCEL_DATA_X0_REG;
    const uint8_t dataYHReg = ACCEL_DATA_Y1_REG;
    const uint8_t dataYLReg = ACCEL_DATA_Y0_REG;
    const uint8_t dataZHReg = ACCEL_DATA_Z1_REG;
    const uint8_t dataZLReg = ACCEL_DATA_Z0_REG;
    
    uint32_t sampleCount = 0; // Total samples collected
    
    uint32_t startTime = micros();
    spi->beginTransaction(SPISettings(SPI_CLOCK, MSBFIRST, SPI_MODE0));
    
    while (!this->stopFlag) {
        // Read interrupt status to check if data is ready
        spi_read(INT_STATUS_REG, &intStatus, 1);
    
        if (intStatus & DATA_RDY_INT_MASK) {
            // Read X-axis data
            spi_read(dataXHReg, &accH, 1);
            spi_read(dataXLReg, &accL, 1);
            buf[this->head] = accL;
            this->head = (this->head + 1) % bufferSize;
            buf[this->head] = accH;
            this->head = (this->head + 1) % bufferSize;
            
            // Read Y-axis data
            spi_read(dataYHReg, &accH, 1);
            spi_read(dataYLReg, &accL, 1);
            buf[this->head] = accL;
            this->head = (this->head + 1) % bufferSize;
            buf[this->head] = accH;
            this->head = (this->head + 1) % bufferSize;
            
            // Read Z-axis data
            spi_read(dataZHReg, &accH, 1);
            spi_read(dataZLReg, &accL, 1);
            buf[this->head] = accL;
            this->head = (this->head + 1) % bufferSize;
            buf[this->head] = accH;
            this->head = (this->head + 1) % bufferSize;
            
            sampleCount++;
            
            // If buffer is full, move this->tail to next sample (overwrite oldest data)
            if (this->head == this->tail) {
                this->tail = (this->tail + 6) % bufferSize; // Advance this->tail by one sample (6 bytes)
            }
        } 
    }
    
    spi->endTransaction();
    uint32_t elapsedTime = micros() - startTime;
    
    // Calculate sampling period in samples per second
    *samplingPeriod = (uint32_t)((sampleCount * 1000000.0f) / elapsedTime);
    return IIM_OK;
}



IIMStatus ICM42688P::sampleContinuousMEMS3Dbyte(uint8_t *buf, uint32_t bufferSize, uint32_t *samplingPeriod) {
    uint8_t accL;
    uint8_t accH;
    uint8_t gyroL;
    uint8_t gyroH;
    uint8_t tempL;
    uint8_t tempH;
    uint8_t intStatus;
    
    
    // Accelerometer data registers
    const uint8_t dataXHReg = ACCEL_DATA_X1_REG;
    const uint8_t dataXLReg = ACCEL_DATA_X0_REG;
    const uint8_t dataYHReg = ACCEL_DATA_Y1_REG;
    const uint8_t dataYLReg = ACCEL_DATA_Y0_REG;
    const uint8_t dataZHReg = ACCEL_DATA_Z1_REG;
    const uint8_t dataZLReg = ACCEL_DATA_Z0_REG;
    const uint8_t gyroXHReg = GYRO_DATA_X1_REG;
    const uint8_t gyroXLReg = GYRO_DATA_X0_REG;
    const uint8_t gyroYHReg = GYRO_DATA_Y1_REG;
    const uint8_t gyroYLReg = GYRO_DATA_Y0_REG;
    const uint8_t gyroZHReg = GYRO_DATA_Z1_REG;
    const uint8_t gyroZLReg = GYRO_DATA_Z0_REG;
    const uint8_t tempHReg = TEMP_DATA1_REG;
    const uint8_t tempLReg = TEMP_DATA0_REG;
    
    uint32_t sampleCount = 0; // Total samples collected
    
    uint64_t startTime = micros();
    spi->beginTransaction(SPISettings(SPI_CLOCK, MSBFIRST, SPI_MODE0));
    
    loopCounter++; //Conta quantidade de amostras
    while (!this->stopFlag) {
        // Read interrupt status to check if data is ready
        spi_read(INT_STATUS_REG, &intStatus, 1);
    
        if (intStatus & DATA_RDY_INT_MASK) {
            // Read X-axis data
            spi_read(dataXHReg, &accH, 1);
            spi_read(dataXLReg, &accL, 1);
            spi_read(gyroXHReg, &gyroH, 1);
            spi_read(gyroXLReg, &gyroL, 1); 
            buf[this->head] = accL;
            this->head = (this->head + 1) % bufferSize;
            buf[this->head] = accH;
            this->head = (this->head + 1) % bufferSize;
            buf[this->head] = gyroL;
            this->head = (this->head + 1) % bufferSize; 
            buf[this->head] = gyroH;
            this->head = (this->head + 1) % bufferSize;
            
            // Read Y-axis data
            spi_read(dataYHReg, &accH, 1);
            spi_read(dataYLReg, &accL, 1);
            spi_read(gyroYHReg, &gyroH, 1);
            spi_read(gyroYLReg, &gyroL, 1);
            buf[this->head] = accL;
            this->head = (this->head + 1) % bufferSize;
            buf[this->head] = accH;
            this->head = (this->head + 1) % bufferSize;
            buf[this->head] = gyroL;
            this->head = (this->head + 1) % bufferSize;
            buf[this->head] = gyroH;
            this->head = (this->head + 1) % bufferSize;
            
            // Read Z-axis data
            spi_read(dataZHReg, &accH, 1);
            spi_read(dataZLReg, &accL, 1);
            spi_read(gyroZHReg, &gyroH, 1);
            spi_read(gyroZLReg, &gyroL, 1);
            buf[this->head] = accL;
            this->head = (this->head + 1) % bufferSize;
            buf[this->head] = accH;
            this->head = (this->head + 1) % bufferSize;
            buf[this->head] = gyroL;
            this->head = (this->head + 1) % bufferSize;
            buf[this->head] = gyroH;
            this->head = (this->head + 1) % bufferSize;

            spi_read(tempHReg, &tempH, 1);
            spi_read(tempLReg, &tempL, 1);
            buf[this->head] = tempL;
            this->head = (this->head + 1) % bufferSize;
            buf[this->head] = tempH;
            this->head = (this->head + 1) % bufferSize;
            
            sampleCount++;
            
            // If buffer is full, move this->tail to next sample (overwrite oldest data)
            if (this->head == this->tail) {
                this->tail = (this->tail + 14) % bufferSize; // Advance this->tail by one sample of each data (14 bytes)
            }
        } 

        // Check if it is time to enter deep sleep
        if ((uint64_t)(micros() - startTime) > (uint64_t)response.untriggerTime * 60ULL * 1000000ULL) {
            this->deepSleepFlag = true;
            this->loopCounter = 0; // Reset loop counter
            break;
        }

        // Check if passou do número de amostras permitido
        if (loopCounter > response.nSamples) {
            this->deepSleepFlag = true;
            this->loopCounter = 0; // Reset loop counter
            break;
        }
    }
    
    spi->endTransaction();
    uint32_t elapsedTime = micros() - startTime;
    
    // Calculate sampling period in samples per second
    *samplingPeriod = (uint32_t)((sampleCount * 1000000.0f) / elapsedTime);
    return IIM_OK;
}


int16_t ICM42688P::readTemperature() {
    uint8_t tempL;
    uint8_t tempH;

    spi_read(TEMP_DATA1_REG, &tempH, 1);
    spi_read(TEMP_DATA0_REG, &tempL, 1);

    return (int16_t)convertFrom8To16(tempH, tempL);
}

// Bloqueia até DATA_RDY e lê accel (X/Y/Z), gyro (X/Y/Z) e temp.
// Retorna true se obteve uma amostra, false em timeout.
bool ICM42688P::readOneSample(int16_t &ax, int16_t &ay, int16_t &az,
                              int16_t &gx, int16_t &gy, int16_t &gz,
                              uint16_t &temp) {
    uint8_t intStatus = 0;
    unsigned long start = micros();

    // Espera DATA_RDY com timeout
    while (true) {
        spi_read(INT_STATUS_REG, &intStatus, 1);
        if (intStatus & DATA_RDY_INT_MASK) break;
        if ((unsigned long)(micros() - start) > 1000000UL) { // 1 s timeout
            return false;
        }
        delayMicroseconds(100);
    }

    // ler registradores rapidamente dentro de uma transação
    spi->beginTransaction(SPISettings(SPI_CLOCK, MSBFIRST, SPI_MODE0));

    uint8_t hi=0, lo=0;

    // Accel X
    spi_read(ACCEL_DATA_X1_REG, &hi, 1);
    spi_read(ACCEL_DATA_X0_REG, &lo, 1);
    ax = (int16_t)convertFrom8To16(hi, lo);

    // Accel Y
    spi_read(ACCEL_DATA_Y1_REG, &hi, 1);
    spi_read(ACCEL_DATA_Y0_REG, &lo, 1);
    ay = (int16_t)convertFrom8To16(hi, lo);

    // Accel Z
    spi_read(ACCEL_DATA_Z1_REG, &hi, 1);
    spi_read(ACCEL_DATA_Z0_REG, &lo, 1);
    az = (int16_t)convertFrom8To16(hi, lo);

    // Gyro X
    spi_read(GYRO_DATA_X1_REG, &hi, 1);
    spi_read(GYRO_DATA_X0_REG, &lo, 1);
    gx = (int16_t)convertFrom8To16(hi, lo);

    // Gyro Y
    spi_read(GYRO_DATA_Y1_REG, &hi, 1);
    spi_read(GYRO_DATA_Y0_REG, &lo, 1);
    gy = (int16_t)convertFrom8To16(hi, lo);

    // Gyro Z
    spi_read(GYRO_DATA_Z1_REG, &hi, 1);
    spi_read(GYRO_DATA_Z0_REG, &lo, 1);
    gz = (int16_t)convertFrom8To16(hi, lo);

    // Temp (two bytes)
    spi_read(TEMP_DATA1_REG, &hi, 1);
    spi_read(TEMP_DATA0_REG, &lo, 1);
    temp = (uint16_t)convertFrom8To16(hi, lo);

    spi->endTransaction();

    return true;
}


void ICM42688P::sleep() {
    uint8_t pwrMgmt0 = PWR_MGMT0_RESET;
    uint8_t intfConfig1 = INTF_CONFIG1_RESET;
    uint8_t pu_pd1_resret = PU_PD_CONFIG1_RESET;
    uint8_t pu_pd2_resret = PU_PD_CONFIG2_RESET;
    uint8_t apex_reset = APEX_CONFIG0_RESET;
    uint8_t bank0 = SEL_BANK_0;
    uint8_t bank1 = SEL_BANK_1;
    uint8_t bank2 = SEL_BANK_2;
    uint8_t bank3 = SEL_BANK_3;
    uint8_t bank4 = SEL_BANK_4;

    //spi_write(INTF_CONFIG1_REG, &intfConfig1, 1);
    spi_write(REG_BANK_SEL, &bank3, 1);
    delay(10);
    spi_write(PU_PD_CONFIG1, &pu_pd1_resret, 1);
    spi_write(PU_PD_CONFIG2, &pu_pd2_resret, 1);
    spi_write(REG_BANK_SEL, &bank0, 1);
    delay(10);
    spi_write(PWR_MGMT0_REG, &pwrMgmt0, 1);
    // spi_write(APEX_CONFIG0, &apex_reset,1);
}

static int spi_write(uint8_t reg, const uint8_t *wbuffer, uint32_t wlen) {
    spi->beginTransaction(SPISettings(SPI_CLOCK, MSBFIRST, SPI_MODE0));

    //LL_GPIO_ResetOutputPin(PIN_CS_PORT, PIN_CS_LL);
    digitalWrite(5, LOW);
    spi->transfer(reg);

    for (uint8_t i = 0; i < wlen; i++) {
        spi->transfer(wbuffer[i]);
    }

    //LL_GPIO_SetOutputPin(PIN_CS_PORT, PIN_CS_LL);
    digitalWrite(5, HIGH);
    spi->endTransaction();
    return 0;
}

static int spi_read(uint8_t reg, uint8_t *rbuffer, uint32_t rlen) {
    
    //LL_GPIO_ResetOutputPin(PIN_CS_PORT, PIN_CS_LL);
    digitalWrite(5, LOW);
    spi->transfer(reg | SPI_READ);
    spi->transfer(rbuffer, rlen);

    //LL_GPIO_SetOutputPin(PIN_CS_PORT, PIN_CS_LL);
    digitalWrite(5, HIGH);
    
    return 0;
}

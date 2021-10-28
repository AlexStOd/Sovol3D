/************************************************************************************************
 * AHT20 Temp&Humidity sensor:
 ************************************************************************************************/

#include "aht20.h"

#include <stm8s_i2c.h>

#define I2C_SPEED     400000
#define TIMEOUT       0xFFFF
#define AHT20_ADDRESS 0x38

// private:
void initI2C()
{
    /*!< I2C Peripheral clock enable */
    CLK_PeripheralClockConfig(CLK_PERIPHERAL_I2C, ENABLE);
    /* I2C configuration after enabling it */
    I2C_Cmd(ENABLE);
    I2C_Init(I2C_SPEED, 15, I2C_DUTYCYCLE_2, I2C_ACK_CURR, I2C_ADDMODE_7BIT, 16);
}

// private:
bool waitEvent(I2C_Event_TypeDef event)
{
    uint32_t timeout = TIMEOUT;
    while (!I2C_CheckEvent(event))
    {
        if ((timeout--) == 0)
        {
            return false;
        }
    }

    return true;
}

// private:
bool sendByte(uint8_t byte)
{
    I2C_SendData(byte);

    /* Test on EV8 and clear it */
    return waitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED);
}

// private:
bool sendBuffer(uint8_t *buffer, uint8_t bytes)
{
    for (uint8_t i = 0; i < bytes; i++)
    {
        if (!sendByte(buffer[i]))
        {
            return false;
        }
    }

    return true;
}

// public:
bool initAHT20(GPIO_TypeDef* port_name_sck, GPIO_Pin_TypeDef port_pin_sck, GPIO_TypeDef* port_name_sda, GPIO_Pin_TypeDef port_pin_sda)
{
    GPIO_Init(port_name_sck, port_pin_sck, GPIO_MODE_OUT_OD_HIZ_FAST);
    GPIO_Init(port_name_sda, port_pin_sda, GPIO_MODE_OUT_OD_HIZ_FAST);
    GPIO_ExternalPullUpConfig(port_name_sck, port_pin_sck, ENABLE);   // подключили внешний pull-up
    GPIO_ExternalPullUpConfig(port_name_sda, port_pin_sda, ENABLE);   // подключили внешний pull-up

    initI2C();
    /* Send START condition */
    I2C_GenerateSTART(ENABLE);

    /* Test on EV5 and clear it */
    if (!waitEvent(I2C_EVENT_MASTER_MODE_SELECT))
    {
        fatal(0);
        return false;
    }

    /* Send EEPROM address for write */
    I2C_Send7bitAddress((uint8_t)(AHT20_ADDRESS << 1), I2C_DIRECTION_TX);

    /* Test on EV6 and clear it */
    if (!waitEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
    {
        fatal(1);
        return false;
    }

    if (!sendByte(0xBE))
    {
        fatal(2);
        return false;
    }

    /* Send STOP condition */
    I2C_GenerateSTOP(ENABLE);

    /* Perform a read on SR1 and SR3 register to clear eventually pending flags */
    (void)I2C->SR1;
    (void)I2C->SR3;

    return true;
}

// private:
bool readFromAHT20(uint8_t *buffer, uint8_t bytes)
{
    /* While the bus is busy */
    uint32_t timeout = TIMEOUT;
    while(I2C_GetFlagStatus(I2C_FLAG_BUSBUSY))
    {
        if ((timeout--) == 0)
        {
            fatal(11);
            return false;
        };
    }

    /* Send START condition */
    I2C_GenerateSTART(ENABLE);

    /* Test on EV5 and clear it */
    if (!waitEvent(I2C_EVENT_MASTER_MODE_SELECT))
    {
        fatal(12);
        return false;
    }

    /* Send EEPROM address for write */
    I2C_Send7bitAddress((uint8_t)(AHT20_ADDRESS << 1), I2C_DIRECTION_RX);

    if (!waitEvent(I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED))
    {
        fatal(13);
        return false;
    }

    while(bytes--)
    {
        if (bytes)
        {
            /* Clear ACK */
            I2C_AcknowledgeConfig(I2C_ACK_CURR);
        }
        else
        {
            /* Clear ACK */
            I2C_AcknowledgeConfig(I2C_ACK_NONE);
        }

        /* Test on EV6 and clear it */
        if (!waitEvent(I2C_EVENT_MASTER_BYTE_RECEIVED))
        {
            fatal(14);
            return false;
        }

        *buffer = I2C_ReceiveData();
        buffer++;
    }

    /* Send STOP condition */
    I2C_GenerateSTOP(ENABLE);

    /* Perform a read on SR1 and SR3 register to clear eventually pending flags */
    (void)I2C->SR1;
    (void)I2C->SR3;
    return true;
}

// private:
bool startAHT20()
{
    /* Send START condition */
    I2C_GenerateSTART(ENABLE);

    /* Test on EV5 and clear it */
    if (!waitEvent(I2C_EVENT_MASTER_MODE_SELECT))
    {
        fatal(20);
        return false;
    }

    /* Send EEPROM address for write */
    I2C_Send7bitAddress((uint8_t)(AHT20_ADDRESS << 1), I2C_DIRECTION_TX);

    /* Test on EV6 and clear it */
    if (!waitEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
    {
        fatal(21);
        return false;
    }

    uint8_t data[] = {0xAC, 0x33, 0x00};
    if (!sendBuffer(data, sizeof(data)))
    {
        fatal(22);
        return false;
    }

    /* Send STOP condition */
    I2C_GenerateSTOP(ENABLE);

    while(1)
    {
        uint8_t data;
        if (!readFromAHT20(&data, 1))
        {
            fatal(23);
            return false;
        }

        if (0 == (data & 0x80))
        {
            break;
        }

        delayMs(10);
    }

    /* Perform a read on SR1 and SR3 register to clear eventually pending flags */
    (void)I2C->SR1;
    (void)I2C->SR3;
    return true;
}

// public:
bool readAHT20(int8_t *t, uint8_t *h)
{
    if (!startAHT20())
    {
        fatal(30);
        return false;
    }

    uint8_t data[6];
    if (!readFromAHT20(data, 6))
    {
        fatal(31);
        return false;
    }

    uint32_t humi = 0;
    int32_t  temp = 0;

    humi = data[1];
    humi <<= 8;
    humi += data[2];
    humi <<= 4;
    humi += data[3] >> 4;

    *h = (humi * 100) >> 20;


    temp = data[3]&0x0f;
    temp <<=8;
    temp += data[4];
    temp <<=8;
    temp += data[5];

    *t = ((temp * 200) >> 20) - 50;

    return true;
}


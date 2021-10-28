/************************************************************************************************
 * AHT20 Temp&Humidity sensor:
 ************************************************************************************************/

#pragma once

#include <utilities.h>
#include <stm8s.h>
#include <stdint.h>
#include <stdbool.h>

// Inits Sensor and I2C bus
bool initAHT20(GPIO_TypeDef* port_name_sck, GPIO_Pin_TypeDef port_pin_sck, GPIO_TypeDef* port_name_sda, GPIO_Pin_TypeDef port_pin_sda);

// Reads temp in humidity from sensor. Blocking
bool readAHT20(int8_t *t, uint8_t *h);

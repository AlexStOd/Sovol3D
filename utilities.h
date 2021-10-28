
#pragma once

#include <stdint.h>
#include <stdbool.h>

#include <stm8s.h>

void delayMs(uint16_t ms);

void fatal(uint8_t err);

void writePin(GPIO_TypeDef* GPIOx, GPIO_Pin_TypeDef PortPin, bool val);

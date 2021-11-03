/************************************************************************************************
 * TM1621C LCD driver:
 ************************************************************************************************/

#pragma once

#include <stdint.h>
#include <stdbool.h>

#include <stm8s.h>

typedef enum
{
    DISP_DEG_C,
    DISP_PERCENT,
    DISP_TEMP,
    DISP_WORK,
    DISP_COLON,
    DISP_TIME,
} EDispItem;

void initTM1621C(GPIO_TypeDef* port_name_cs,      GPIO_Pin_TypeDef port_pin_cs,
                 GPIO_TypeDef* port_name_wr,      GPIO_Pin_TypeDef port_pin_wr,
                 GPIO_TypeDef* port_name_data,    GPIO_Pin_TypeDef port_pin_data,
                 GPIO_TypeDef* in_port_backlight, GPIO_Pin_TypeDef in_pin_backlight);

void clearDisp();

void printDigits(uint8_t left, uint8_t right);

void setDigitSegments(uint8_t pos, uint8_t segments);

void setBacklightState(bool active);

void setItemStatus(EDispItem item, bool status);

void printErr(uint8_t err);

/*************************************************************************************************
 * Keys:
 ************************************************************************************************/

#pragma once

#include <stdint.h>
#include <stdbool.h>

#include <stm8s.h>

typedef enum
{
    KEY_NONE,
    KEY_POWER,
    KEY_MODE,
    KEY_UP,
} EKeyId;

typedef enum
{
    KEY_NOT_ACTIVE,
    KEY_PRESSED,
    KEY_LONG_PRESSED,
} EKeyEvent;

typedef struct SKeyHandler
{
    EKeyId           key_id;
    GPIO_TypeDef*    port_key;
    GPIO_Pin_TypeDef pin_key;
    GPIO_TypeDef*    port_led;
    GPIO_Pin_TypeDef pin_led;
    uint16_t         time_ms;
} SKeyHandler;

extern SKeyHandler keys[];

void initKey(SKeyHandler *this);

bool getKeyState(EKeyId *key, EKeyEvent *state);

void setLedState(EKeyId key, bool state);

void initKeys(uint8_t keys_count);

void handleKeys();

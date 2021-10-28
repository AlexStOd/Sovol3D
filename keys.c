/*************************************************************************************************
 * Keys:
 ************************************************************************************************/

#include <keys.h>
#include <utilities.h>

#define LONG_PRESS_TIME_MS 2000

EKeyId    new_key_id    = KEY_NONE;
EKeyEvent new_key_event = KEY_NOT_ACTIVE;

uint8_t keys_count = 0;

// private:
void initKey(SKeyHandler *this)
{
    this->time_ms = 0;

    GPIO_Init(this->port_key, this->pin_key, GPIO_MODE_IN_PU_NO_IT);
    GPIO_Init(this->port_led, this->pin_led, GPIO_MODE_OUT_PP_LOW_FAST);

    writePin(this->port_led, this->pin_led, 1);
}

// public:
void setLedState(EKeyId key, bool state)
{
    for (uint8_t i = 0; 0 < keys_count; i++)
    {
        if (key == keys[i].key_id)
        {
            writePin(keys[i].port_led, keys[i].pin_led, !state);
            break;
        }
    }
}

// private:
void sendKeyState(EKeyId key, EKeyEvent state)
{
    new_key_id    = key;
    new_key_event = state;
}

// public:
bool getKeyState(EKeyId *key, EKeyEvent *state)
{
    if (KEY_NONE != new_key_id)
    {
        *key   = new_key_id;
        *state = new_key_event;

        new_key_id    = KEY_NONE;
        new_key_event = KEY_NOT_ACTIVE;
        return true;
    }
    return false;
}

// private:
void handleKey(SKeyHandler *this)
{
    bool key_state = !GPIO_ReadInputPin(this->port_key, this->pin_key);

    if (key_state) // key pressed
    {
        if (LONG_PRESS_TIME_MS > this->time_ms)
        {
            this->time_ms++;
        }
        else if (LONG_PRESS_TIME_MS == this->time_ms)
        {
            sendKeyState(this->key_id, KEY_LONG_PRESSED);
            this->time_ms++;
        }
        else
        {
            // Do nothing. Event has already been reported.
        }
    }
    else
    {
        if ((0 != this->time_ms) && (LONG_PRESS_TIME_MS > this->time_ms))
        {
            sendKeyState(this->key_id, KEY_PRESSED);
        }

        this->time_ms = 0;
    }
}

// public:
void initKeys(uint8_t count)
{
    keys_count = count;
    for (uint8_t i = 0; i < keys_count; i++)
    {
        initKey(&keys[i]);
    }
}

// public:
void handleKeys()
{
    for (uint8_t i = 0; i < keys_count; i++)
    {
        handleKey(&keys[i]);
    }
}

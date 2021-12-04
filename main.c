/**
 * Compile by:
 *  https://github.com/XaviDCR92/sdcc-gas
 *  https://github.com/XaviDCR92/stm8-binutils-gdb
 * Compilation example:
 *  https://github.com/XaviDCR92/stm8-dce-example
 */

/* Includes ------------------------------------------------------------------*/

#include "aht20.h"
#include <tm1621c.h>
#include <keys.h>

#include <utilities.h>

#include "stm8s.h"
#include "stm8s_clk.h"
#include "stm8s_adc1.h"

#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Evalboard I/Os configuration */

#define GPIO_BACKLIGHT    GPIOA, GPIO_PIN_2
#define GPIO_LED_POWER    GPIOD, GPIO_PIN_6
#define GPIO_LED_MODE     GPIOD, GPIO_PIN_5
#define GPIO_LED_UP       GPIOD, GPIO_PIN_3

#define GPIO_DISP_CS      GPIOA, GPIO_PIN_1
#define GPIO_DISP_WR      GPIOC, GPIO_PIN_5
#define GPIO_DISP_DATA    GPIOC, GPIO_PIN_7

#define GPIO_I2C_SDA      GPIOB, GPIO_PIN_4
#define GPIO_I2C_SCL      GPIOB, GPIO_PIN_5

#define GPIO_KEY_POWER    GPIOD, GPIO_PIN_2
#define GPIO_KEY_MODE     GPIOC, GPIO_PIN_3
#define GPIO_KEY_UP       GPIOC, GPIO_PIN_6

#define GPIO_TEMP_SENSOR  GPIOC, GPIO_PIN_4

#define GPIO_HEATER       GPIOA, GPIO_PIN_3

#define GPIO_FAN          GPIOD, GPIO_PIN_1

#define GPIO_BEEPER       GPIOD, GPIO_PIN_4

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

volatile uint32_t millis = 0;

// Settings which are stored to eeprom:
typedef struct
{
    bool    use_beeper;
    bool    start_power_state;
    uint8_t start_temp_index;
    uint8_t start_time_index;
} SEeprom;

SEeprom eeprom;

/* Private function prototypes -----------------------------------------------*/

void handleTick1ms();

void fatal(uint8_t err);

/* Private functions ---------------------------------------------------------*/
/* Public functions ----------------------------------------------------------*/

/************************************************************************************************
 * GPIO:
 ************************************************************************************************/

void writePin(GPIO_TypeDef* GPIOx, GPIO_Pin_TypeDef PortPin, bool val)
{

    if (val)
    {
        GPIO_WriteHigh(GPIOx, PortPin);
    }
    else
    {
        GPIO_WriteLow(GPIOx, PortPin);
    }
}

/************************************************************************************************
 * Timers:
 ************************************************************************************************/

void initTimer2()
{
    TIM2_TimeBaseInit(TIM2_PRESCALER_16, 1000);
    TIM2_ITConfig(TIM2_IT_UPDATE, ENABLE);
    TIM2_Cmd(ENABLE);
}

INTERRUPT_HANDLER(TIM2_UPD_OVF_BRK_IRQHandler, 13)
{
    TIM2_ClearITPendingBit(TIM2_IT_UPDATE);
    millis++;

    handleKeys();
    handleTick1ms();
}

/************************************************************************************************
 * ADC1:
 ************************************************************************************************/

void initADC()
{
    /*  Init GPIO for ADC1 */
    GPIO_Init(GPIO_TEMP_SENSOR, GPIO_MODE_IN_FL_NO_IT);

    /* Init ADC1 peripheral */
    ADC1_Init(ADC1_CONVERSIONMODE_CONTINUOUS,
              ADC1_CHANNEL_2,
              ADC1_PRESSEL_FCPU_D2,
              ADC1_EXTTRIG_TIM,
              DISABLE,
              ADC1_ALIGN_RIGHT,
              ADC1_SCHMITTTRIG_CHANNEL2,
              DISABLE);

    /* Enable EOC interrupt */
//    ADC1_ITConfig(ADC1_IT_EOC, ENABLE);

    /* Enable general interrupts */
//    enableInterrupts();

    /*Start Conversion */
    ADC1_StartConversion();
}

typedef struct
{
    int32_t res;
    uint8_t temp;
} SResToTemp;

SResToTemp res2temp[] =
{
    {166500, 15},  // 0
    {128475l, 20},  // 1
    {99500l,  25},  // 2
    {75530l,  30},  // 3
    {46050l,  40},  // 4
    {31070l,  50},  // 5
    {20800l , 60},  // 6
    {14490l,  70},  // 7
    {10000l,  80},  // 8
    {7160l,   90},  // 9
    {5220l,   100}, // 10
    {3860l,   110}, // 11
    {3316l,   120}, // 12
    {2873l,   125}, // 13
};

int8_t getHeaterTemperature()
{
    int32_t adc_val = ADC1_GetConversionValue();
    int32_t v       = (adc_val * 3300l) / 1024l;
    int32_t res     = 100000l * v / (3300l - v);
    int32_t temp    = 50;
    bool    found   = false;

    for (uint8_t i = 0; i < ((sizeof(res2temp) / sizeof(SResToTemp)) - 1); i++)
    {
        if ((res2temp[i].res > res) && (res2temp[i+1].res <= res))
        {
            int32_t dR1  = res2temp[i+1].res  - res2temp[i].res;
            int32_t dT1  = res2temp[i+1].temp - res2temp[i].temp;
            int32_t dR2  = res - res2temp[i].res;
            int32_t dT2  = dT1 * dR2 / dR1;
            temp = res2temp[i].temp + dT2;
            found = true;
            break;
        }
    }

    if (!found)
    {
        fatal(51);
    }
    return temp;
}

/************************************************************************************************
 * Beeper:
 ************************************************************************************************/

#define BEEP_SHORT_TIME_MS  50
#define BEEP_LONG_TIME_MS   500

uint32_t beep_time_left_ms = 0;

void setBeeperState(bool on)
{
    writePin(GPIO_BEEPER, on);
}

void handleBeepTick()
{
    if (0 != beep_time_left_ms)
    {
        beep_time_left_ms--;

        if (0 == beep_time_left_ms)
        {
            setBeeperState(false);
        }
    }
}

void beep(uint32_t time)
{
    if (eeprom.use_beeper)
    {
        beep_time_left_ms = time;

        setBeeperState(true);
    }
}

/************************************************************************************************
 * Heater/Fan:
 ************************************************************************************************/

bool heater_state = false;

void switchHeater(bool on)
{
    if (on != heater_state)
    {
        heater_state = on;
        writePin(GPIO_HEATER, on);
    }
}

inline void switchFan(bool on)
{
    writePin(GPIO_FAN, on);
}

SKeyHandler keys[] =
{
    {KEY_POWER, GPIO_KEY_POWER, GPIO_LED_POWER, 0},
    {KEY_MODE,  GPIO_KEY_MODE,  GPIO_LED_MODE,  0},
    {KEY_UP,    GPIO_KEY_UP,    GPIO_LED_UP,    0},
};

/**************************************************************************************************
 * MENU:
 *************************************************************************************************/

uint8_t temp_values[] =
{
    40, 45, 50, 55, 60, 65, 70
};

uint8_t time_values[] =
{
    3, 4, 5, 6, 7, 8, 9, 10, 11, 12
};

uint8_t curr_temp_index = sizeof(temp_values) - 1;
uint8_t curr_time_index = sizeof(time_values) - 1;

typedef enum
{
    MENU_TEMP,
    MENU_TIME,
    MENU_WORK,
    MENU_ITEMS_COUNT
} EMenuItem;

typedef enum
{
    SCREEN_TEMP_HUM,
    SCREEN_TIME,
    SCREEN_HEATER_TEMP,
    SCREEN_ITEMS_COUNT
} EScreenItem;

bool      curr_on_off_state = false;
EMenuItem curr_menu_state   = MENU_WORK;

uint32_t    curr_time_left_ms = 10ul * 3600ul * 1000ul;
EScreenItem curr_screen       = 0;
uint32_t    main_screen_timer = 1;
uint32_t    menu_active_timer = 0;

const uint32_t c_menu_active_timeout_ms = 5ul * 1000ul;

void handleStateOff(EKeyId key);
void handleStateOn(EKeyId key);

void handleStateOnOff(EKeyId key)
{
    if (curr_on_off_state)
    {
        handleStateOn(key);
    }
    else
    {
        handleStateOff(key);
    }
}

void switchPowerOff()
{
    curr_on_off_state = false;
    clearDisp();
    setBacklightState(false);
    switchHeater(false);
    switchFan(false);
}

void switchPowerOn()
{
    curr_time_left_ms = (uint32_t)(time_values[sizeof(time_values) - 1]) * 3600ul * 1000ul; // TODO: get index from EEPROM
    // TODO: set temp and time to heater controller

    curr_menu_state   = MENU_WORK;
    curr_on_off_state = true;

    setBacklightState(true);
    setItemStatus(DISP_WORK, true);

    switchFan(true);
}

void handleStateOff(EKeyId key)
{
    if (KEY_POWER == key)
    {
        switchPowerOn();
    }
}

void showTime()
{
    setItemStatus(DISP_TEMP,    false);
    setItemStatus(DISP_DEG_C,   false);
    setItemStatus(DISP_PERCENT, false);

    if (MENU_TIME == curr_menu_state)
    {
        setItemStatus(DISP_TIME,    true);
    }

    setItemStatus(DISP_COLON,   true);

    uint32_t time_min = curr_time_left_ms / 60000ul;

    printDigits(time_min / 60, time_min % 60);
}

void showTemp()
{
    setItemStatus(DISP_TEMP,  true);
    setItemStatus(DISP_DEG_C, true);

    printDigits(temp_values[curr_temp_index], 0xFF);
}

void showHeaterTemp()
{
    setItemStatus(DISP_TIME,    false);
    setItemStatus(DISP_COLON,   false);

    setItemStatus(DISP_WORK,    false);
    setItemStatus(DISP_DEG_C,   false);
    setItemStatus(DISP_PERCENT, false);

    setItemStatus(DISP_WORK,    true);

    printNumberWithPreffix(0b1110110, getHeaterTemperature()); // 0b1110110 = "H"
}

uint8_t curr_temperature = 0;
uint8_t curr_humidity    = 0;

void showCurrTempHum()
{
    setItemStatus(DISP_TIME,    false);
    setItemStatus(DISP_COLON,   false);

    setItemStatus(DISP_WORK,    true);
    setItemStatus(DISP_DEG_C,   true);
    setItemStatus(DISP_PERCENT, true);

    printDigits(curr_temperature, curr_humidity);
}

void handleTick1ms()
{
    if (curr_on_off_state)
    {
        if (0 != curr_time_left_ms)
        {
            curr_time_left_ms--;
        }
        else
        {
            switchPowerOff();
        }

        main_screen_timer--;
        if (0 == main_screen_timer)
        {
            main_screen_timer = 2000;

            curr_screen++;
            if (SCREEN_ITEMS_COUNT <= curr_screen)
            {
                curr_screen = SCREEN_TEMP_HUM;
            }
        }

        if (0 != menu_active_timer)
        {
            menu_active_timer--;

            if (0 == menu_active_timer)
            {
                curr_menu_state   = MENU_WORK;
                main_screen_timer = 1;
                setItemStatus(DISP_TIME,    false);
                setItemStatus(DISP_TEMP,    false);
                setItemStatus(DISP_COLON,   false);
                setItemStatus(DISP_WORK,    false);
                setItemStatus(DISP_DEG_C,   false);
                setItemStatus(DISP_PERCENT, false);

                setItemStatus(DISP_WORK,    true);
            }
        }
    }

    handleBeepTick();
}

void showWorkScreen()
{
    switch (curr_screen)
    {
        case SCREEN_TEMP_HUM:
            showCurrTempHum();
            break;
        case SCREEN_TIME:
            showTime();
            break;
        case SCREEN_HEATER_TEMP:
            showHeaterTemp();
            break;
        default:
            curr_screen = SCREEN_TEMP_HUM;
            showCurrTempHum();
            break;
    }
}

void handleStateOn(EKeyId key)
{
    switch (key)
    {
        case KEY_POWER:
            // TODO: switch heater off
            switchPowerOff();
            break;

        case KEY_MODE:
            clearDisp();

            curr_menu_state++;
            menu_active_timer = c_menu_active_timeout_ms;

            if (MENU_ITEMS_COUNT == curr_menu_state)
            {
                curr_menu_state = 0;
            }

            switch (curr_menu_state)
            {
                case MENU_TEMP:
                    // TODO: turn heater off
                    showTemp();
                    break;

                case MENU_TIME:
                    // TODO: turn heater off
                    showTime();
                    break;

                case MENU_WORK:
                    // TODO: turn on heater
                    setItemStatus(DISP_WORK, true);
                    // temperature will be shown in the main loop
                    break;
            }
            break;

        case KEY_UP:
            menu_active_timer = c_menu_active_timeout_ms;

            switch (curr_menu_state)
            {
                case MENU_TEMP:
                    curr_temp_index++;
                    if (sizeof(temp_values) <= curr_temp_index)
                    {
                        curr_temp_index = 0;
                    }
                    showTemp();
                    break;

                case MENU_TIME:
                {
                    uint32_t time_hours = (curr_time_left_ms + 15000ul) / 3600000ul;
                    uint8_t  new_index  = 0;
                    for (uint8_t i = 0; i < sizeof(time_values); i++)
                    {
                        if (time_values[i] > time_hours)
                        {
                            new_index       = i;
                            curr_time_index = i;
                            break;
                        }
                    }

                    curr_time_left_ms = time_values[new_index];
                    curr_time_left_ms *= 3600ul * 1000ul;
                    curr_time_left_ms += 100; // to show proper time in menu like 06:00

                    showTime();
                    break;
                }

                case MENU_WORK:
                    // do nothing
                    break;

                default:
                    // do nothing
                    break;
            }
            break;

        default:
            // do nothing
            break;
    }
}

/**************************************************************************************************
 * EEPROM:
 *************************************************************************************************/

void readFromEeprom()
{
    uint8_t *eeprom_addr = (uint8_t*)0x4000;
    uint8_t crc          = 0;
    uint8_t i            = 0;
    uint8_t *buffer      = (uint8_t*)&eeprom;

    for (i = 0; i < sizeof(eeprom); i++)
    {
        buffer[i] = eeprom_addr[i];
        crc      += eeprom_addr[i];
    }

    if (crc != eeprom_addr[i])
    {
        // set default values:
        eeprom.use_beeper        = true;
        eeprom.start_power_state = false;
        eeprom.start_temp_index  = 0;
        eeprom.start_time_index  = 0;
    }

    curr_temp_index   = eeprom.start_temp_index;
    curr_time_index   = eeprom.start_time_index;

    curr_time_left_ms = time_values[curr_temp_index];
}

void storeToEeprom()
{
    uint16_t eeprom_addr = 0x4000;
    uint8_t *buffer      = (uint8_t*)&eeprom;
    uint8_t crc          = 0;
    uint8_t i            = 0;

    FLASH_Unlock(FLASH_MEMTYPE_DATA);

    for (i = 0; i < sizeof(eeprom); i++)
    {
        FLASH_ProgramByte(eeprom_addr + i, buffer[i]);
        crc += buffer[i];
    }

    FLASH_ProgramByte(eeprom_addr + i, crc);

    FLASH_Lock(FLASH_MEMTYPE_DATA);
}

/************************************************************************************************
 * MAIN:
 ************************************************************************************************/

void main(void)
{
    /* Initialization of the clock */
    CLK_HSICmd(ENABLE);
    CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV1);
    CLK_SYSCLKConfig(CLK_PRESCALER_CPUDIV1);

    readFromEeprom();

    writePin(GPIO_LED_POWER, 1);
    writePin(GPIO_LED_MODE,  1);
    writePin(GPIO_LED_UP,    1);

    initTimer2();
    enableInterrupts();

    initTM1621C(GPIO_DISP_CS, GPIO_DISP_WR, GPIO_DISP_DATA, GPIO_BACKLIGHT);
    setBacklightState(false);
    clearDisp();

    const uint8_t keys_count = sizeof(keys)/sizeof(SKeyHandler);
    initKeys(keys_count);

    initADC();

    GPIO_Init(GPIO_HEATER, GPIO_MODE_OUT_PP_LOW_FAST);
    GPIO_Init(GPIO_FAN,    GPIO_MODE_OUT_PP_LOW_FAST);
    GPIO_Init(GPIO_BEEPER, GPIO_MODE_OUT_PP_LOW_FAST);

    delayMs(40);

    if (eeprom.start_power_state)
    {
        beep(BEEP_LONG_TIME_MS);
        switchPowerOn();
    }
    else
    {
        switchPowerOff();
    }

    // Perform up to 10 attempts to initialize sensor
    uint8_t count = 10;
    while (!initAHT20(GPIO_I2C_SCL, GPIO_I2C_SDA))
    {
        if (0 != count)
        {
            count--;
        }
        else
        {
            // Panic
            while (1);
        }
    }

    uint32_t timer_1s = millis + 1000;

    while (1)
    {
        if (timer_1s <= millis)
        {
            timer_1s = millis + 1000;

            if (!readAHT20(&curr_temperature, &curr_humidity))
            {
                switchHeater(false);
                while(1) ;
            }

            int8_t heater_temp     = getHeaterTemperature();
            uint8_t requested_temp = temp_values[curr_temp_index];

            if (curr_on_off_state)
            {
                if ((curr_temperature < requested_temp) && (heater_temp < (requested_temp + 40)))
                {
                    switchHeater(true);
                }
                else
                {
                    switchHeater(false);
                }
            }
        }

        EKeyId    new_key_id    = 1;
        EKeyEvent new_key_event = 12;

        if (getKeyState(&new_key_id, &new_key_event))
        {
            if (KEY_PRESSED == new_key_event)
            {
                beep(BEEP_SHORT_TIME_MS);

                switch (new_key_id)
                {
                    case KEY_POWER:
                        setLedState(KEY_POWER, 1);
                        break;

                    case KEY_MODE:
                        setLedState(KEY_MODE, 1);
                        break;

                    case KEY_UP:
                        setLedState(KEY_UP, 1);
                        break;

                    default:
                        break;
                }

                handleStateOnOff(new_key_id);
            }
            else if (KEY_LONG_PRESSED == new_key_event)
            {
                beep(BEEP_LONG_TIME_MS);

                switch (new_key_id)
                {
                    case KEY_POWER:
                        eeprom.start_power_state = !eeprom.start_power_state;
                        storeToEeprom();
                        break;

                    case KEY_MODE:
                        eeprom.start_temp_index = curr_temp_index;
                        eeprom.start_time_index = curr_time_index;
                        storeToEeprom();
                        break;

                    case KEY_UP:
                        eeprom.use_beeper = !eeprom.use_beeper;
                        storeToEeprom();

                        if (eeprom.use_beeper)
                        {
                            beep(BEEP_LONG_TIME_MS);
                        }
                        break;

                    default:
                        break;
                }
            }
            else
            {
                // do nothing
            }
        }
        else
        {
            if (curr_on_off_state && (MENU_WORK == curr_menu_state))
            {
                showWorkScreen();
            }

            setLedState(KEY_POWER, 0);
            setLedState(KEY_MODE,  0);
            setLedState(KEY_UP,    0);
        }

        delayMs(20);
    }
}

void delayMs(uint16_t ms)
{
    // note: this routine may skip 1ms
    uint32_t wait_until = millis + ms;

    while (millis != wait_until)
    {
        // do nothing, just wait
    }
}

void fatal(uint8_t err)
{
    switchHeater(false);

    printErr(err);

    delayMs(5000);
}

#ifdef USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *   where the assert_param error has occurred.
  * @param file: pointer to the source file name
  * @param line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
    /* User can add his own implementation to report the file name and line number,
        ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

    /* Infinite loop */
    while (1)
    {
    }
}
#endif

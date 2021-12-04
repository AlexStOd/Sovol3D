/************************************************************************************************
 * TM1621C LCD driver:
 ************************************************************************************************/

#include <tm1621c.h>

#include <utilities.h>

enum ECommand
{
    WRITE              = 0b1010000000000,
    COMMAND_SYS_DIS    = 0b100000000000,
    COMMAND_SYS_EN     = 0b100000000010,
    COMMAND_LCD_OFF    = 0b100000000100,
    COMMAND_LCD_ON     = 0b100000000110,
    COMMAND_XTAL32K    = 0b100000101000,
    COMMAND_RC256K     = 0b100000110000,
    COMMAND_BIAS1_2_00 = 0b100001000000,
    COMMAND_BIAS1_2_01 = 0b100001001000,
    COMMAND_BIAS1_2_10 = 0b100001010000,
    COMMAND_BIAS1_3_00 = 0b100001000010,
    COMMAND_BIAS1_3_01 = 0b100001001010,
    COMMAND_BIAS1_3_10 = 0b100001010010,
    COMMAND_TOPT       = 0b100111000000,
    COMMAND_TNORMAL    = 0b100111000110,
};

const uint8_t digit2segments[] =
{
    //gfedcba
    0b0111111, // 0
    0b0000110, // 1
    0b1011011, // 2
    0b1001111, // 3
    0b1100110, // 4
    0b1101101, // 5
    0b1111101, // 6
    0b0000111, // 7
    0b1111111, // 8
    0b1101111, // 9
/*
    0b1110111, // A
    0b1111100, // b
    0b1011000, // c
    0b1011111, // d
    0b1111001, // E
    0b1110001, // F
    0b1101111, // g
    0b1110100, // h
    0b1110110, // H
*/
};

uint8_t disp_data[] =
{
    0b1111, // 1A 2A 3A 4A          0
    0b1111, // 1B 2B 3B 4B          1
    0b1111, // 1C 2C 3C 4C          2
    0b1111, // 1D 2D 3D 4D          3
    0b1111, // 1E 2E 3E 4E          4
    0b1111, // 1F 2F 3F 4F          5
    0b1111, //                      6
    0b1111, //                      7
    0b1111, //                      8
    0b1111, //                      9
    0b1111, //                      10
    0b1111, //                      11
    0b1111, //                      12
    0b0000, // 1G 2G 3G 4G          13
    0b0000, // degC % TEMP WORK     14
    0b0000, // : TIME ? ?           15
    0b1111, //                      16
    0b1111, //                      17
};

const uint8_t segment2address[] = {0, 1, 2, 3, 4, 5, 13};

GPIO_TypeDef*    port_cs        = GPIOA;
GPIO_Pin_TypeDef pin_cs         = GPIO_PIN_0;
GPIO_TypeDef*    port_wr        = GPIOA;
GPIO_Pin_TypeDef pin_wr         = GPIO_PIN_0;
GPIO_TypeDef*    port_data      = GPIOA;
GPIO_Pin_TypeDef pin_data       = GPIO_PIN_0;
GPIO_TypeDef*    port_backlight = GPIOA;
GPIO_Pin_TypeDef pin_backlight  = GPIO_PIN_0;

#define CS        port_cs,        pin_cs
#define WR        port_wr,        pin_wr
#define DATA      port_data,      pin_data
#define BACKLIGHT port_backlight, pin_backlight

void writeTM1621C(uint16_t data);

void initTM1621C(GPIO_TypeDef* in_port_cs,        GPIO_Pin_TypeDef in_pin_cs,
                 GPIO_TypeDef* in_port_wr,        GPIO_Pin_TypeDef in_pin_wr,
                 GPIO_TypeDef* in_port_data,      GPIO_Pin_TypeDef in_pin_data,
                 GPIO_TypeDef* in_port_backlight, GPIO_Pin_TypeDef in_pin_backlight)
{
    port_cs        = in_port_cs;
    pin_cs         = in_pin_cs;
    port_wr        = in_port_wr;
    pin_wr         = in_pin_wr;
    port_data      = in_port_data;
    pin_data       = in_pin_data;
    port_backlight = in_port_backlight;
    pin_backlight  = in_pin_backlight;


    GPIO_Init(port_cs,        pin_cs,        GPIO_MODE_OUT_PP_LOW_FAST);
    GPIO_Init(port_wr,        pin_wr,        GPIO_MODE_OUT_PP_LOW_FAST);
    GPIO_Init(port_data,      pin_data,      GPIO_MODE_OUT_PP_LOW_FAST);
    GPIO_Init(port_backlight, pin_backlight, GPIO_MODE_OUT_PP_LOW_FAST);

    writeTM1621C(COMMAND_SYS_EN);
    writeTM1621C(COMMAND_LCD_ON);
    writeTM1621C(COMMAND_BIAS1_2_10);
    writeTM1621C(COMMAND_TNORMAL);
}

void setBacklightState(bool active)
{
    writePin(BACKLIGHT, active);
}

void writeTM1621C(uint16_t data)
{
    const uint16_t pause = 10;

    writePin(CS, 0);
    uint8_t len = 12;
    if (0 != (data & (1 << 11)))
    {
        len = 11; // for commands
    }

    for (int8_t bit = len; bit >= 0; bit--)
    {
        writePin(DATA, 0 != (data & (1 << bit)));
        writePin(WR,   0);
        writePin(WR,   1);
    }

    writePin(DATA, 1);
    writePin(CS,   1);
}

void writeDispData()
{
    for (char addr = 0; addr < 18; addr++)
    {
        writeTM1621C(WRITE | (addr << 4) | (disp_data[addr]));
    }
}

void setDigitSegments(uint8_t pos, uint8_t segments)
{
    for (uint8_t i = 0; i < 7; i++)
    {
        disp_data[segment2address[i]] &= ~(1 << (3 - pos));
        if (segments & (1 << i))
        {
            disp_data[segment2address[i]] |= 1 << (3 - pos);
        }
    }
}

void clearDisp()
{
    for (uint8_t i = 0; i < sizeof(disp_data); i++)
    {
        disp_data[i] = 0;
    }

    writeDispData();
}

void printDigits(uint8_t left, uint8_t right)
{
    // clear right digits:
//    for (uint8_t j = 0; j < 6; j++)
//    {
//        disp_data[j] = 0;
//    }

//    disp_data[13] = 0;

    uint8_t digits[4];
    digits[0] = left  / 10; // left digit
    digits[1] = left  % 10;
    digits[2] = right / 10;
    digits[3] = right % 10; // right digit

    uint8_t start = 0;
    uint8_t stop  = 4;

    if (0xFF == left)
    {
        start = 2;
    }

    if (0xFF == right)
    {
        stop = 2;
    }

    for (uint8_t i = start; i < stop; i++)
    {
        uint8_t digit = digits[i];

        if (digit < 10)
        {
            digit = digit2segments[digit];
        }
        else
        {
            digit = 0;
        }

        setDigitSegments(i, digit);
    }

    writeDispData();
}

void printNumberWithPreffix(uint8_t prefix_bitmap, uint16_t number)
{

    setDigitSegments(0, prefix_bitmap);

    if (999 < number)
    {
        setDigitSegments(1, 0b1000000);
        setDigitSegments(2, 0b1000000);
        setDigitSegments(3, 0b1000000);
    }
    else
    {
        uint8_t digit0 = number / 100;
        uint8_t digit1 = (number % 100) / 10;
        uint8_t digit2 = (number % 10);

        if (0 == digit0)
        {
            setDigitSegments(1, 0b0000000);
        }
        else
        {
            setDigitSegments(1, digit2segments[digit0]);
        }

        if ((0 == digit0) && (0 == digit1))
        {
            setDigitSegments(2, 0xb0000000);
        }
        else
        {
            setDigitSegments(2, digit2segments[digit1]);
        }

        setDigitSegments(3, digit2segments[digit2]);
    }

    writeDispData();
}

void printErr(uint8_t err)
{
    clearDisp();

    uint8_t segments_E = 0b1111001;
    uint8_t segments_r = 0b1010000;

    setDigitSegments(0, segments_E);
    setDigitSegments(1, segments_r);

    if (err < 99)
    {
        setDigitSegments(2, digit2segments[err / 10]);
        setDigitSegments(3, digit2segments[err % 10]);
    }
    else
    {
        setDigitSegments(2, segments_E);
        setDigitSegments(3, segments_r);
    }

    writeDispData();
}

void setBit(uint8_t *byte, uint8_t bit, bool status)
{
    if (status)
    {
        *byte |= 1 << bit;
    }
    else
    {
        *byte &= ~(1 << bit);
    }
}

void setItemStatus(EDispItem item, bool status)
{
    switch (item)
    {
        case DISP_DEG_C:
            setBit(&disp_data[14], 3, status);
            break;

        case DISP_PERCENT:
            setBit(&disp_data[14], 2, status);
            break;

        case DISP_TEMP:
            setBit(&disp_data[14], 1, status);
            break;

        case DISP_WORK:
            setBit(&disp_data[14], 0, status);
            break;

        case DISP_COLON:
            setBit(&disp_data[15], 3, status);
            break;

        case DISP_TIME:
            setBit(&disp_data[15], 2, status);
            break;

        default:
            // do nothing
            break;
    }

    writeDispData();
}

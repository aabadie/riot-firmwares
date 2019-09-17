#ifndef TFT_DISPLAY_H
#define TFT_DISPLAY_H

#include <stdio.h>
#include "ucg.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Message codes for demo display messages
 */
enum {
    TFT_DISPLAY_LOGO  = 0xA0,
    TFT_DISPLAY_LED   = 0xA1,
    TFT_DISPLAY_TEMP  = 0xA2,
    TFT_DISPLAY_PRES  = 0xA3,
    TFT_DISPLAY_HUM   = 0xA4,
    TFT_DISPLAY_HELLO = 0xA5,
    TFT_DISPLAY_TVOC  = 0xA6,
    TFT_DISPLAY_ECO2  = 0xA7,
    TFT_DISPLAY_TSP   = 0xA8,
    TFT_DISPLAY_TLP   = 0xA9,
};

/**
 * @brief   Message to be displayed on top of the display, application name
 */
#ifndef APPLICATION_NAME
#define APPLICATION_NAME       "TFT DISPLAY"
#endif
/**
 * @brief   Version of the application, if APP_VER is not defined
 */
#ifndef NODE_SUIT_VERSION
#define NODE_SUIT_VERSION      "XXXXXXXX"
#endif

/**
 * @brief   ucglib display definitions
 */
#ifndef TFT_DISPLAY
#define TFT_DISPLAY            (ucg_dev_st7735_18x128x128)
#endif
#ifndef TFT_DISPLAY_EXT
#define TFT_DISPLAY_EXT        (ucg_ext_st7735_18)
#endif

/**
 * @brief   ucglib abstraction layer pin definition
 */
#ifdef BOARD_NUCLEO_L476RG
#ifndef TFT_DEV_SPI
#define TFT_DEV_SPI            (0U)
#endif
#ifndef TFT_PIN_CS
#define TFT_PIN_CS             (GPIO_PIN(PORT_B,6))
#endif
#ifndef TFT_PIN_CD
#define TFT_PIN_CD             (GPIO_PIN(PORT_A,9))
#endif
#ifndef TFT_PIN_RESET
#define TFT_PIN_RESET          (GPIO_PIN(PORT_C,7))
#endif
#elif defined(BOARD_SAMR21_XPRO)
#ifndef TFT_DEV_SPI
#define TFT_DEV_SPI            (1U)
#endif
#ifndef TFT_PIN_CS
#define TFT_PIN_CS             (GPIO_PIN(PA,14))
#endif
#ifndef TFT_PIN_CD
#define TFT_PIN_CD             (GPIO_PIN(PA,15))
#endif
#ifndef TFT_PIN_RESET
#define TFT_PIN_RESET          (GPIO_PIN(PA,8))
#endif
#elif defined(BOARD_NRF52840_MDK)
#ifndef TFT_DEV_SPI
#define TFT_DEV_SPI            (0U)
#endif
#ifndef TFT_PIN_CS
#define TFT_PIN_CS             (GPIO_PIN(0, 31))
#endif
#ifndef TFT_PIN_CD
#define TFT_PIN_CD             (GPIO_PIN(0, 3))
#endif
#ifndef TFT_PIN_RESET
#define TFT_PIN_RESET          (GPIO_PIN(0, 2))
#endif
#endif

/**
 * @brief   Setups the display, starts the thread.
 *
 * @param[in] ucg    pointer to ucg device descriptor
 * @param[out] pid    pid of thread
 */
int init_st7735_printer(ucg_t* ucg);

/**
 * @brief   Utility function that returns the tft display thread pid
 *
 * @param[out] pid    pid of thread
 */
int tft_get_pid(void);

/**
 * @brief   Utility function that sends a message (msg_t) copying the content
 *          of the msg_ptr before hand. Internally a mutex impeds multiple
 *          access to the buffer.
 *
 * @param[in] type   the type of the msg to send
 * @param[in] buf    pointer to the buffer, msg.content.ptr
 * @param[in] len    the length of the buffer to send
 *
 */
void display_send_buf(uint16_t type, uint8_t* buf, uint8_t len);

/**
 * @brief   Utility function that sends a message (msg_t)
 *
 * @param[in] type   the type of the msg to send
 * @param[in] value  content of the msg msg.content.value.
 *
 */
void display_send_val(uint16_t type, uint32_t value);

#ifdef __cplusplus
}
#endif

#endif

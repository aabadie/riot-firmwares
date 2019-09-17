#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "periph/gpio.h"
#include "periph/spi.h"

#include "mutex.h"
#include "xtimer.h"
#include "riotboot/slot.h"

#ifdef MODULE_COAP_SUIT
#include "suit/v4/suit.h"
#endif

#ifdef MODULE_SUITREG
#include "suitreg.h"
#endif

#include "logo.h"
#include "tft_display.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

#define TFT_DISPLAY_QUEUE_SIZE    (4)
#define SUIT_FW_PROGRESS_CYCLE    (16U)
#define SUIT_ERROR_DELAY          (5*US_PER_SEC)

static msg_t _tft_display_msg_queue[TFT_DISPLAY_QUEUE_SIZE];
static char tft_display_stack[THREAD_STACKSIZE_DEFAULT];

static mutex_t lock = MUTEX_INIT;
static char msg_data_buffer[16];

static gpio_t pins[] = {
    [UCG_PIN_CS]  = TFT_PIN_CS,
    [UCG_PIN_CD]  = TFT_PIN_CD,
    [UCG_PIN_RST] = TFT_PIN_RESET
};

static uint32_t pins_enabled = (
    (1 << UCG_PIN_CS) +
    (1 << UCG_PIN_CD) +
    (1 << UCG_PIN_RST)
    );

/* keep a reference to the threads pid */
static volatile int tft_display_pid;

static void _init_st7735(ucg_t * ucg)
{
    /* Initialize to SPI */
    DEBUG("Initializing to SPI.\n");
    ucg_SetPins(ucg, pins, pins_enabled);
    ucg_SetDevice(ucg, SPI_DEV(TFT_DEV_SPI));

    /* Initialize the display */
    DEBUG("Initializing display.\n");
    ucg_Init(ucg, TFT_DISPLAY, TFT_DISPLAY_EXT, ucg_com_riotos_hw_spi);
    ucg_SetRotate90(ucg);
    /* Initial Screen Setup*/
    ucg_ClearScreen(ucg);
    ucg_SetFontMode(ucg, UCG_FONT_MODE_SOLID);
}

static void tft_puts(ucg_t * ucg, char* str_pre, char* str_data, char* str_post,
              uint8_t offset_x, uint8_t offset_y, uint8_t center)
{
    char buf[32];
    strcpy(buf, str_pre);
    strcat(buf, str_data);
    strcat(buf, str_post);

    if(center) {
        uint8_t width = ucg_GetStrWidth(ucg, buf);
        offset_x = (width/2 < (offset_x)) ? (offset_x - width/2) : 0;
    }
    ucg_SetColor(ucg, 1, 0, 0, 0);
    ucg_SetColor(ucg, 0, 255, 255, 255);
    ucg_DrawString(ucg, offset_x, offset_y, 0, buf);
}

#ifdef MODULE_RIOTBOOT_SLOT
static void tft_print_int(ucg_t * ucg, int data, uint8_t offset_x,
                      uint8_t offset_y, uint8_t center)
{
    char buffer [8];
    sprintf(buffer, "%i", data);
    tft_puts(ucg, buffer, NULL, NULL, offset_x, offset_y, center);
}
#endif

static void _draw_riot_logo(ucg_t* ucg, uint16_t start_x, uint16_t start_y)
{
    for (int y = 0; y < 48 ; y++) {
        for (int x = 0; x < 96; x++) {
            uint32_t offset = (x + (y * 96)) * 3;
            ucg_SetColor(ucg, 0, logo[offset + 2], logo[offset + 1], logo[offset + 0]);
            ucg_DrawPixel(ucg, x + start_x, y + start_y);
        }
    }
}

static void _draw_riotboot(ucg_t* ucg)
{
#ifdef MODULE_RIOTBOOT_SLOT
    /* Running Slot */
    ucg_SetFontPosCenter(ucg);
    ucg_SetFont(ucg, ucg_font_profont15_mr);
    tft_print_int(ucg, riotboot_slot_current(), 117, 118, 1);

    /* Draw Frame For slot Info */
    ucg_SetColor(ucg, 1, 0, 0, 0);
    ucg_SetColor(ucg, 0, 255, 255, 255);
    ucg_DrawRFrame(ucg, 108, 108, 18, 18, 4);

    /* Draw Frame For Version Info */
    ucg_DrawRFrame(ucg, 2, 108, 104, 18, 4);
    ucg_SetFont(ucg, ucg_font_profont12_mr);
    ucg_SetFontPosTop(ucg);
    char buffer [32];
    sprintf(buffer, "VER: %s", NODE_SUIT_VERSION);
    tft_puts(ucg, buffer, NULL, NULL, 52, 112, 1);
#else
    (void) ucg;
#endif
}

static void _draw_app_name(ucg_t* ucg)
{
    ucg_SetFontPosTop(ucg);
    ucg_SetFont(ucg, ucg_font_profont12_mr);
    tft_puts(ucg, (char* ) APPLICATION_NAME, NULL, NULL, 63, 0, 1);
}

static void _clear_logo_area(ucg_t* ucg)
{
    ucg_SetColor(ucg, 0, 0, 0, 0);
    ucg_DrawBox(ucg, 0, 14, 128, 48);
    ucg_SetColor(ucg, 0, 255, 255, 255);
}

static void _clear_data_area(ucg_t* ucg)
{
#if ((defined(MODULE_BMX280) + defined(MODULE_CCS811) + \
      defined(MODULE_SM_PWM_01C)) > 1)
    ucg_SetColor(ucg, 0, 0, 0, 0);
    ucg_DrawBox(ucg, 0, 66, 128, 40);
    ucg_SetColor(ucg, 0, 255, 255, 255);
#else
    (void) ucg;
#endif
}

int tft_get_pid(void)
{
    return tft_display_pid;
}


void display_send_buf(uint16_t type, uint8_t* str, uint8_t len)
{
    mutex_lock(&lock);
    memset(msg_data_buffer, 0, sizeof(msg_data_buffer));
    memcpy(msg_data_buffer, str, len);
    msg_t m;
    m.type = type;
    m.content.ptr = msg_data_buffer;
    msg_send(&m, tft_get_pid());
}

void display_send_val(uint16_t type, uint32_t value)
{
    msg_t m;
    m.type = type;
    m.content.value = value;
    msg_send(&m, tft_get_pid());
}

static void display_release(void)
{
    mutex_unlock(&lock);
}

void *tft_display_thread(void *args)
{
    (void) args;

    ucg_t * ucg = (ucg_t *) args;

    msg_init_queue(_tft_display_msg_queue, TFT_DISPLAY_QUEUE_SIZE);

    msg_t m;
#ifdef MODULE_SUITREG
    msg_t m_tx;
    xtimer_t timer;
    suitreg_t entry = SUITREG_INIT_PID(SUITREG_TYPE_STATUS | SUITREG_TYPE_ERROR,
                                       thread_getpid());
    suitreg_register(&entry);
    uint8_t count = 0;
    uint32_t fw_size = 0;
#endif

    while (1) {
        msg_receive(&m);
        switch(m.type) {
            case TFT_DISPLAY_LED:
                ucg_SetFontPosCenter(ucg);
                ucg_SetFont(ucg, ucg_font_profont17_mr);
                if (m.content.value) {
                    tft_puts(ucg, "LED ON ", NULL, NULL, 64, 86, 1);
                }
                else {
                    tft_puts(ucg, "LED OFF", NULL, NULL, 64, 86, 1);
                }
                ucg_SetFontPosTop(ucg);
                break;
            case TFT_DISPLAY_TEMP:
                _clear_data_area(ucg);
                ucg_SetFont(ucg, ucg_font_profont15_mr);
                tft_puts(ucg,"TEMP: ", m.content.ptr, NULL, 64, 66, 1);
                display_release();
                break;
            case TFT_DISPLAY_PRES:
                ucg_SetFont(ucg, ucg_font_profont15_mr);
                tft_puts(ucg,"PRES: ", m.content.ptr, NULL, 64, 78, 1);
                display_release();
                break;
            case TFT_DISPLAY_HUM:
                ucg_SetFont(ucg, ucg_font_profont15_mr);
                tft_puts(ucg,"HUM: ", m.content.ptr, NULL, 64, 90, 1);
                display_release();
                break;
            case TFT_DISPLAY_ECO2:
                _clear_data_area(ucg);
                ucg_SetFont(ucg, ucg_font_profont15_mr);
                tft_puts(ucg,"ECO2: ", m.content.ptr, NULL, 64, 66, 1);
                display_release();
                break;
            case TFT_DISPLAY_TVOC:
                ucg_SetFont(ucg, ucg_font_profont15_mr);
                tft_puts(ucg,"TVOC: ", m.content.ptr, NULL, 64, 84, 1);
                display_release();
                break;
            case TFT_DISPLAY_TSP:
                _clear_data_area(ucg);
                ucg_SetFont(ucg, ucg_font_profont15_mr);
                tft_puts(ucg,"TSP: ", m.content.ptr, NULL, 64, 66, 1);
                display_release();
                break;
            case TFT_DISPLAY_TLP:
                ucg_SetFont(ucg, ucg_font_profont15_mr);
                tft_puts(ucg,"TLP: ", m.content.ptr, NULL, 64, 84, 1);
                display_release();
                break;
            case TFT_DISPLAY_HELLO:
                ucg_SetFont(ucg, ucg_font_profont15_mr);
                tft_puts(ucg, "H E L L O", NULL, NULL, 63, 70, 1);
                tft_puts(ucg, "W O R L D !!", NULL, NULL,  63, 90, 1);
                break;
            case TFT_DISPLAY_LOGO:
                _clear_logo_area(ucg);
                _draw_riot_logo(ucg, 16, 14);
                break;
#ifdef MODULE_SUITREG
            case SUIT_TRIGGER:
                _clear_logo_area(ucg);
                ucg_SetFont(ucg, ucg_font_profont12_mr);
                tft_puts(ucg, "  UPDATE  ", NULL, NULL,  63, 24, 1);
                tft_puts(ucg, "STARTING", NULL, NULL,  63, 38, 1);
                break;
            case SUIT_SIGNATURE_START:
                _clear_logo_area(ucg);
                ucg_SetFont(ucg, ucg_font_profont12_mr);
                tft_puts(ucg, "VERIFYING", NULL, NULL,  63, 24, 1);
                tft_puts(ucg, "SIGNATURE", NULL, NULL,  63, 38, 1);
                break;
            case SUIT_SIGNATURE_ERROR:
                ucg_SetFont(ucg, ucg_font_profont12_mr);
                tft_puts(ucg, "SIGNATURE", NULL, NULL,  63, 24, 1);
                tft_puts(ucg, "  ERROR  ", NULL, NULL,  63, 38, 1);
                m_tx.type = TFT_DISPLAY_LOGO;
                xtimer_set_msg(&timer, SUIT_ERROR_DELAY, &m_tx, thread_getpid());
                break;
            case SUIT_SEQ_NR_ERROR:
                _clear_logo_area(ucg);
                ucg_SetFont(ucg, ucg_font_profont12_mr);
                tft_puts(ucg, "  INVALID  ", NULL, NULL,  63, 24, 1);
                tft_puts(ucg, "SEQUENCE NUMBER", NULL, NULL,  63, 38, 1);
                m_tx.type = TFT_DISPLAY_LOGO;
                xtimer_set_msg(&timer, SUIT_ERROR_DELAY, &m_tx, thread_getpid());
                break;
            case SUIT_DIGEST_START:
                ucg_SetFont(ucg, ucg_font_profont12_mr);
                tft_puts(ucg, "VERIFYING", NULL, NULL,  63, 24, 1);
                tft_puts(ucg, " DIGEST ", NULL, NULL,  63, 38, 1);
                m_tx.type = TFT_DISPLAY_LOGO;
                xtimer_set_msg(&timer, SUIT_ERROR_DELAY, &m_tx, thread_getpid());
                break;
            case SUIT_DIGEST_ERROR:
                _clear_logo_area(ucg);
                ucg_SetFont(ucg, ucg_font_profont12_mr);
                tft_puts(ucg, "INVALID", NULL, NULL,  63, 24, 1);
                tft_puts(ucg, " DIGEST ", NULL, NULL,  63, 38, 1);
                m_tx.type = TFT_DISPLAY_LOGO;
                xtimer_set_msg(&timer, SUIT_ERROR_DELAY, &m_tx, thread_getpid());
                break;
            case SUIT_REBOOT:
                _clear_logo_area(ucg);
                ucg_SetFont(ucg, ucg_font_profont12_mr);
                tft_puts(ucg, "UPDATE", NULL, NULL,  63, 24, 1);
                tft_puts(ucg, "FINALIZED", NULL, NULL,  63, 38, 1);
                break;
            case SUIT_DOWNLOAD_START:
                _clear_logo_area(ucg);
                ucg_SetFont(ucg, ucg_font_profont12_mr);
                tft_puts(ucg, "UPDATING", NULL, NULL,  63, 20, 1);
                ucg_SetColor(ucg, 0, 255, 255, 255);
                ucg_DrawFrame(ucg, 12, 36, 104, 18);
                fw_size = m.content.value;
                break;
            case SUIT_DOWNLOAD_PROGRESS:
                if (count == 0) {
                    ucg_SetColor(ucg, 0, 255, 255, 255);
                    ucg_DrawBox(ucg, 14, 38,
                                (100*m.content.value)/ fw_size, 14);
                }
                count++;
                count = count % SUIT_FW_PROGRESS_CYCLE;
                break;
            case SUIT_DOWNLOAD_ERROR:
                _clear_logo_area(ucg);
                ucg_SetFont(ucg, ucg_font_profont12_mr);
                tft_puts(ucg, "FAILED", NULL, NULL,  63, 24, 1);
                tft_puts(ucg, "DOWNLOAD", NULL, NULL,  63, 38, 1);
                m_tx.type = TFT_DISPLAY_LOGO;
                xtimer_set_msg(&timer, SUIT_ERROR_DELAY, &m_tx, thread_getpid());
                break;
#endif
            default:
                break;
        }
    }
    return NULL;
}

int init_st7735_printer(ucg_t * ucg)
{
    _init_st7735(ucg);

    _draw_riot_logo(ucg, 16, 14);
    _draw_app_name(ucg);
    _draw_riotboot(ucg);

    tft_display_pid = thread_create(tft_display_stack, sizeof(tft_display_stack),
                                         THREAD_PRIORITY_MAIN - 1,
                                         THREAD_CREATE_STACKTEST, tft_display_thread,
                                         ucg, "tft_display thread");
    if (tft_display_pid == -EINVAL || tft_display_pid == -EOVERFLOW) {
        puts("Error: failed to create tft display thread, exiting\n");
        return tft_display_pid;
    }
    else {
        puts("Successfully created tft display thread !\n");
        return tft_display_pid;
    }
}
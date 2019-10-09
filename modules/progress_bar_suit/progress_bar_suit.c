#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "progress_bar.h"

#include "mutex.h"
#include "xtimer.h"
#include "riotboot/slot.h"

#include "progress_bar_suit.h"

#include "suitreg.h"

#ifdef MODULE_COAP_SUIT
#include "suit/v4/suit.h"
#endif

#define ENABLE_DEBUG (0)
#include "debug.h"

#define PROGRESS_BAR_QUEUE_SIZE    (4)

static msg_t _progress_bar_msg_queue[PROGRESS_BAR_QUEUE_SIZE];
static char _progress_bar_stack[THREAD_STACKSIZE_DEFAULT];

/* keep a reference to the threads pid */
static volatile int _progress_bar_pid;


void *progress_bar_thread(void *args)
{
    (void) args;

    progress_bar_t progress_bar;

    msg_init_queue(_progress_bar_msg_queue, PROGRESS_BAR_QUEUE_SIZE);

    suitreg_t entry = SUITREG_INIT_PID(SUITREG_TYPE_STATUS | SUITREG_TYPE_ERROR,
                                       thread_getpid());
    suitreg_register(&entry);
    uint32_t fw_size = 0;

    msg_t m;
    while (1) {
        msg_receive(&m);
        switch(m.type) {
            case SUIT_TRIGGER:
                puts("Start SUIT update");
                break;
            case SUIT_SIGNATURE_START:
                printf("Verifying signature:");
                break;
            case SUIT_SIGNATURE_ERROR:
                puts("\nFailed:");
                break;
            case SUIT_SIGNATURE_END:
                puts(" Success!");
                break;
            case SUIT_SEQ_NR_ERROR:
                puts("Failed:");
                break;
            case SUIT_DIGEST_START:
                puts("Verifying digest: ");
                break;
            case SUIT_DIGEST_ERROR:
                printf("Failed: ");
                break;
            case SUIT_REBOOT:
                puts("\nReboot!");
                break;
            case SUIT_DOWNLOAD_START:
                sprintf(progress_bar.prefix, "%s ", "Downloading firmware:");
                progress_bar_update(&progress_bar);
                fw_size = m.content.value;
                break;
            case SUIT_DOWNLOAD_PROGRESS:
                progress_bar.value = (100 * m.content.value) / fw_size;
                sprintf(progress_bar.suffix, " %d%%", progress_bar.value);
                progress_bar_update(&progress_bar);
                if (progress_bar.value == 100) {
                    puts(" Download complete");
                }
                break;
            case SUIT_DOWNLOAD_ERROR:
                sprintf(progress_bar.suffix, " %s", "Failed: ");
                break;
            default:
                break;
        }
    }
    return NULL;
}

void init_progress_bar_printer(void)
{
    _progress_bar_pid = thread_create(_progress_bar_stack, sizeof(_progress_bar_stack),
                                      THREAD_PRIORITY_MAIN - 1,
                                      THREAD_CREATE_STACKTEST, progress_bar_thread,
                                      NULL, "progress bar thread");
    if (_progress_bar_pid == -EINVAL) {
        puts("Error: failed to create progress bar thread, exiting\n");
    }
    else {
        puts("Successfully created progress bar thread !\n");
    }
}

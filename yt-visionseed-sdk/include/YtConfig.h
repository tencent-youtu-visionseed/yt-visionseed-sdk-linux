#define PB_ENABLE_MALLOC 1

#if defined(YTMSG_FULL)
    // Linux
    #define PB_FIELD_32BIT 1
#elif defined(YTMSG_LITE)
    // FreeRTOS, ESP32, STM32
    #define PB_FIELD_32BIT 1
#else
    // Arduino AVR
    #define PB_FIELD_16BIT 1
    #define PB_NO_ERRMSG 1
#endif

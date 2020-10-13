#ifndef ORANGE_LED
#define ORANGE_LED 21
#endif

#define MAX485_DE 16
#define MAX485_RE_NEG 17

#ifndef READING_FREQUENCY
#define READING_FREQUENCY UINT32_C(1 * 60 * 1000)
#endif

#ifndef MQTT_KEEPALIVE
#define MQTT_KEEPALIVE 30
#endif

#ifndef MQTTled
#define MQTTled 0
#endif

#ifdef COMP_OLED
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#endif

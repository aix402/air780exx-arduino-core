#include <Arduino.h>
#include <AIR780EPM_LuatOS.h>

namespace {

constexpr uint8_t kLcdPowerPin = PIN_GPIO28;
constexpr uint8_t kLcdResetPin = PIN_GPIO36;
constexpr unsigned long kStageDelayMs = 1200;
constexpr uint32_t kLcdBusSpeed = 20000000UL;

luat_lcd_conf_t gLcd = {};
bool gLcdReady = false;
uint8_t gStage = 0;

struct ColorStage {
  luat_color_t color;
  const char* name;
};

constexpr ColorStage kStages[] = {
    {0xF800, "RED"},
    {0x07E0, "GREEN"},
    {0x001F, "BLUE"},
    {0xFFE0, "YELLOW"},
    {LCD_WHITE, "WHITE"},
    {LCD_BLACK, "BLACK"},
};

void configureLcd() {
  memset(&gLcd, 0, sizeof(gLcd));
  gLcd.port = LUAT_LCD_HW_ID_0;
  gLcd.pin_dc = LUAT_GPIO_NONE;
  gLcd.pin_pwr = LUAT_GPIO_NONE;
  gLcd.pin_rst = kLcdResetPin;
  gLcd.lcd_clk_pin = LUAT_GPIO_NONE;
  gLcd.lcd_sda_pin = LUAT_GPIO_NONE;
  gLcd.lcd_cs_pin = LUAT_GPIO_NONE;
  gLcd.interface_mode = LUAT_LCD_IM_4_WIRE_8_BIT_INTERFACE_I;
  gLcd.auto_flush = 1;
  gLcd.direction = 0;
  gLcd.w = 320;
  gLcd.h = 480;
  gLcd.bus_speed = kLcdBusSpeed;
  gLcd.opts = &lcd_opts_st7796;
}

void drawDiagnostics() {
  luat_lcd_clear(&gLcd, LCD_BLACK);
  luat_lcd_draw_rectangle(&gLcd, 8, 8, gLcd.w - 9, gLcd.h - 9, LCD_WHITE);
  luat_lcd_draw_line(&gLcd, 0, 0, gLcd.w - 1, gLcd.h - 1, 0xF800);
  luat_lcd_draw_line(&gLcd, 0, gLcd.h - 1, gLcd.w - 1, 0, 0x07E0);
  luat_lcd_draw_circle(&gLcd, gLcd.w / 2, gLcd.h / 2, 48, 0x001F);
}

void showStage(const ColorStage& stage) {
  Serial.printf("+ARDUINO: LCD,STAGE,%s\r\n", stage.name);
  luat_lcd_clear(&gLcd, stage.color);
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(300);

  Serial.printf("+ARDUINO: LCD,READY\r\n");
  Serial.printf("+ARDUINO: LCD,BOARD,V1.2\r\n");
  Serial.printf("+ARDUINO: LCD,POWER,GPIO28\r\n");
  Serial.printf("+ARDUINO: LCD,RESET,GPIO36\r\n");
  Serial.printf("+ARDUINO: LCD,IF,HWID_0\r\n");

  pinMode(kLcdPowerPin, OUTPUT);
  digitalWrite(kLcdPowerPin, HIGH);
  delay(80);

  configureLcd();
  luat_lcd_IF_init(&gLcd);
  const int initRc = luat_lcd_init(&gLcd);
  Serial.printf("+ARDUINO: LCD,INIT,%d\r\n", initRc);
  gLcdReady = (initRc == 0);

  if (!gLcdReady) {
    return;
  }

  showStage(kStages[0]);
  delay(kStageDelayMs);
  drawDiagnostics();
  delay(kStageDelayMs);
}

void loop() {
  if (!gLcdReady) {
    delay(1000);
    return;
  }

  showStage(kStages[gStage]);
  gStage = (gStage + 1) % (sizeof(kStages) / sizeof(kStages[0]));
  delay(kStageDelayMs);
}

#include <EEPROM.h>

#include <Arduino.h>

#define SWCLK_PIN 18
#define SWDIO_PIN 19
#define SWD_DELAY_US 1000

inline void swclk_high() { digitalWrite(SWCLK_PIN, HIGH); }
inline void swclk_low()  { digitalWrite(SWCLK_PIN, LOW);  }
inline void swdio_high() { digitalWrite(SWDIO_PIN, HIGH); }
inline void swdio_low()  { digitalWrite(SWDIO_PIN, LOW);  }

inline void swdio_output() { pinMode(SWDIO_PIN, OUTPUT); }
inline void swdio_input()  { pinMode(SWDIO_PIN, INPUT_PULLUP); }
inline int  swdio_read()   { return digitalRead(SWDIO_PIN); }

inline void swd_delay() { delayMicroseconds(SWD_DELAY_US); }

void swd_clock_cycle()
{
  swclk_low();  swd_delay();
  swclk_high(); swd_delay();
}

// ========== 切换到 SWD 协议 ==========
void swd_switch_sequence()
{
  swdio_output();
  swdio_high();

  // Step 1: JTAG-to-SWD sequence
  for (int i = 0; i < 60; i++) swd_clock_cycle();  // >50个1

  uint16_t magic = 0xE79E; // JTAG->SWD
  for (int i = 0; i < 16; i++) {
    if (magic & (1 << i)) swdio_high(); else swdio_low();
    swd_clock_cycle();
  }
  swdio_high();
  for (int i = 0; i < 60; i++) swd_clock_cycle();  // >50个1
  // Step 3: 至少8个1（空闲）
  swdio_low();
  for (int i = 0; i < 20; i++) swd_clock_cycle();
  swdio_high();
}

// ========== 发送 Line Reset ==========
void swd_line_reset()
{
  swdio_output();
  swdio_high();
  for (int i = 0; i < 56; i++) swd_clock_cycle();
  swdio_low();
  for (int i = 0; i < 2; i++) swd_clock_cycle();
  swdio_high();
  for (int i = 0; i < 56; i++) swd_clock_cycle();
}

// ========== 写/读 bit ==========
void swd_write_bit(int bit)
{
  swdio_output();
  if (bit) swdio_high(); else swdio_low();
  swd_clock_cycle();
}
#if 0
int swd_read_bit()
{
  int bit;
  swdio_input();
  swclk_high(); swd_delay();
  bit = swdio_read();
  swclk_low(); swd_delay();
  return bit;
}
#endif
int swd_read_bit()
{
  int bit;
  swdio_input();
  swclk_low(); swd_delay();
  bit = swdio_read();
  swclk_high(); swd_delay();
  return bit;
}
void swd_write_byte(uint8_t data)
{
  for (int i = 0; i < 8; i++)
    swd_write_bit((data >> i) & 1);
}

uint32_t swd_read_u32()
{
  uint32_t val = 0;
  for (int i = 0; i < 32; i++)
    val |= (swd_read_bit() << i);
  return val;
}

// ========== 读取 IDCODE ==========
void swd_read_idcode()
{
  uint8_t req = 0b10100101; // Start(1), APnDP=0, RnW=1, A2/A3=00, Parity=0, Stop(0), Park(1)
  swd_write_byte(req);

  // Turnaround 1 bit
  swdio_input();
  swd_read_bit();

  // Read ACK
  uint8_t ack = 0;
  for (int i = 0; i < 3; i++)
    ack |= (swd_read_bit() << i);

  if (ack == 0b001) {
    uint32_t id = swd_read_u32();
    Serial.printf("ACK OK, IDCODE = 0x%08X\n", id);
  } else {
    Serial.printf("ACK ERROR = %d (binary %03b)\n", ack, ack);
  }
}

// ========== 主函数 ==========
void setup()
{
  Serial.begin(115200);
  pinMode(SWCLK_PIN, OUTPUT);
  pinMode(SWDIO_PIN, OUTPUT);
  Serial.println("SWD Switch + Read IDCODE");
  delay(100);
  Serial.printf("swd init");

}

void loop() {
    swd_switch_sequence();
 // swd_line_reset();
  swd_read_idcode();
  delay(3000);


}

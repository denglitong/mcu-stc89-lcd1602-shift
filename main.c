#include <reg52.h>

// data bus 数据总线
#define LCD1602_DB P0

// Enable
sbit LCD1602_E = P1 ^ 5;   
// Read/Write
sbit LCD1602_RW = P1 ^ 1;
// Record/inStruction
sbit LCD1602_RS = P1 ^ 0;

bit flag500ms = 0; // 500ms 定时标志
unsigned char InterruptCount = 0;
unsigned char T0RH = 0; // T0 重载值的高字节
unsigned char T0RL = 0;	// T0重载值的低字节

unsigned char str1[] = "Welcome to LCD1602 show!";
unsigned char str2[] = "Supported by Litong Deng";

void ConfigTimer0(unsigned int ms);
void InitLcd1602();
void LcdShowStr(unsigned char x, unsigned char y, unsigned char *str, unsigned char len);

void main() {
	unsigned char i = 0, index = 0;
	unsigned char pdata bufMove1[16 + sizeof(str1) + 16];
	unsigned char pdata bufMove2[16 + sizeof(str2) + 16];

	EA = 1; // enable global interrupt
	ConfigTimer0(20); // 配置T0定时20ms
	InitLcd1602();

	for (i=0; i<16; i++) {
		bufMove1[i] = ' ';
		bufMove2[i] = ' ';
	}
	for (i=0; i<sizeof(str1)-1; i++) {
		bufMove1[16+i] = str1[i];
		bufMove2[16+i] = str2[i];
	}
	for (i=16+sizeof(str1)-1; i<sizeof(bufMove1); i++) {
		bufMove1[i] = ' ';
		bufMove2[i] = ' ';
	}

	// shift to left
	while (1) {
		if (flag500ms) { // 每 500ms 移动一次屏幕
			flag500ms = 0;
			LcdShowStr(0, 0, bufMove1 + index, 16);
			LcdShowStr(0, 1, bufMove2 + index, 16);
			index++;
			if (index >= 16+sizeof(str1)-1) {
				index = 0;
			}
		}
	}

	// shift to right
//	index = sizeof(bufMove1)-1-16;
//	while (1) {
//		if (flag500ms) { // 每 500ms 移动一次屏幕
//			flag500ms = 0;
//			LcdShowStr(0, 0, bufMove1 + index, 16);
//			LcdShowStr(0, 1, bufMove2 + index, 16);
//			index--;
//			if (index <= 0) {
//				index = sizeof(bufMove1)-1-16;
//			}
//		}
//	}
}

/**
 * Config Timer0 interval milliseconds
 * @param interval_ms Timer interval milliseconds, valid number: 1 ~ 71
 */
void ConfigTimer0(unsigned int interval_ms) {
  unsigned long total_time_beats = (11059200 / 12 / 1000) * interval_ms;
  unsigned long init_time_beats = 65536 - total_time_beats;

  ET0 = 1;       // enable Timer0 interrupt
  TMOD &= 0xF0;  // 清除 T0 的控制位
  TMOD |= 0x01;  // set Timer0 mode TH0-TL0 16 bits timer
  // setup TH0 TL0 initial value
  TH0 = (unsigned char)(init_time_beats >> 8);
  TL0 = (unsigned char)(init_time_beats);
  T0RH = TH0;
  T0RL = TL0;
  TR0 = 1;  // start Timer0
}

void LcdWaitReady() {
	unsigned char sta = 0xFF;

	// 1. read instruction
	LCD1602_RW = 1;
	LCD1602_RS = 0;

	// disable default, in case the P0 output affter other circle
	LCD1602_DB = 0xFF;
	LCD1602_E = 0;

	// wait until STA7 is 0 (not busy, then we can send instruction/data)
	do {
		LCD1602_E = 1;
		sta = LCD1602_DB;
		// after we have read the data, we should turn off P0 immediately
		// in case the P0 output affect other circle
		LCD1602_E = 0;
	} while (sta & 0x80);
}

void LcdWriteCmd(unsigned char cmd) {
	LcdWaitReady();
	LCD1602_RW = 0;
	LCD1602_RS = 0;
	LCD1602_DB = cmd;
	LCD1602_E = 1;
	LCD1602_E = 0;
}

void InitLcd1602() {
	// 设置 16x2 显示，5x7点阵，8位数据接口
	LcdWriteCmd(0x38);
	// 设置显示开，不显示光标
	LcdWriteCmd(0x0D);
	// 当读写一个字符后地址指针加一，且光标加一
	LcdWriteCmd(0x06);
	// 数据指针清零，所有显示清零
	LcdWriteCmd(0x01);
}

// 00 01 02 03 ... 0E 0F
// 40 41 42 43 ... 4E 4F
void LcdSetCursor(unsigned char x, unsigned char y) {
	unsigned char addr;
	if (y == 0) {
		addr = 0x00 + x;
	} else {
		addr = 0x40 + x;
	}
	// 设置数据地址指针
	LcdWriteCmd(0x80 | addr);
}

void LcdWriteData(unsigned char dat) {
	LcdWaitReady();
	LCD1602_RW = 0;
	LCD1602_RS = 1;
	LCD1602_DB = dat;
	LCD1602_E = 1;
	LCD1602_E = 0;
}

void LcdShowStr(unsigned char x, unsigned char y, unsigned char *str, unsigned char len) {
	LcdSetCursor(x, y);
	while (*str != '\0' && len > 0) {
		LcdWriteData(*str);
		str++;
		len--;
	}
}

void InterruptTimer0() interrupt 1 {
	TH0 = T0RH;
	TL0 = T0RL;
	
	InterruptCount++;
	if (InterruptCount >= 50) {
		InterruptCount = 0;
		flag500ms = 1;
	}
}
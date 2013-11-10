#ifndef _A13_GPIO_H
#define _A13_GPIO_H

/*
 * sunxi has 9 banks of gpio, they are:
 * PA0 - PA17 | PB0 - PB23 | PC0 - PC24
 * PD0 - PD27 | PE0 - PE31 | PF0 - PF5
 * PG0 - PG9  | PH0 - PH27 | PI0 - PI12
 */

#define SUNXI_GPIO_A    0
#define SUNXI_GPIO_B    1
#define SUNXI_GPIO_C    2
#define SUNXI_GPIO_D    3
#define SUNXI_GPIO_E    4
#define SUNXI_GPIO_F    5
#define SUNXI_GPIO_G    6
#define SUNXI_GPIO_H    7
#define SUNXI_GPIO_I    8

struct sunxi_gpio {
   unsigned int cfg[4];
   unsigned int dat;
   unsigned int drv[2];
   unsigned int pull[2];
};//size = 36(0x24) bytes

/* gpio interrupt control */
struct sunxi_gpio_int {
   unsigned int cfg[3];
   unsigned int ctl;
   unsigned int sta;
   unsigned int deb;         /* interrupt debounce */
};

struct sunxi_gpio_reg {
   struct sunxi_gpio gpio_bank[9];
   unsigned char res[0xbc];
   struct sunxi_gpio_int gpio_int;
};

struct sunxi_gpio_address_info {
	unsigned long pBankAddress;			//bank startup address containing pin control and data values
	unsigned long ulPinConfigAddress;		//adress of config buffer[0-3] for selected bank
	/* TODO: add config for PULLand drv configs */
};

#endif /* _A13_GPIO_H */

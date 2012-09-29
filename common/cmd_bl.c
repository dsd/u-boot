/*
 * cmd_bl.c - just like `bl` command
 *
 * Copyright (c) 2008 Analog Devices Inc.
 *
 * Licensed under the GPL-2 or later.
 */

#include <config.h>
#include <common.h>
#include <command.h>
#include <asm/arch/gpio.h>

#define	CON_BACKLIGHT	S5P_GPL2_CON
#define	DAT_BACKLIGHT	S5P_GPL2_DAT
#define	PUD_BACKLIGHT	S5P_GPL2_PUD
#define	BIT_BACKLIGHT	4

#define	GPIO_SET_CON(func, val)	CON_##func = (CON_##func & ~(0xF << (BIT_##func * 4))) | (((val) & 0xF) << (BIT_##func * 4))
#define	GPIO_SET_PUD(func, val)	PUD_##func = (PUD_##func & ~(0x3 << (BIT_##func * 2))) | (((val) & 0x3) << (BIT_##func * 2))
#define	GPIO_SET_DAT(func, val)	DAT_##func = (DAT_##func & ~(0x1 << (BIT_##func * 1))) | (((val) & 0x1) << (BIT_##func * 1))

#define	GPIO_GET_DAT(func)	((DAT_##func >> BIT_##func) & 1)

int bl_current = 0;

void bl_control(int status)
{
	unsigned long *reg = 0x11000;

	GPIO_SET_CON(BACKLIGHT, 1);

	if (status) {
			
		#ifdef CONFIG_LOGO_DISPLAY
		Exynos_LCD_turnon();
		exynos_display_pic(2);
		#endif
		//printf("backlight on\n");
		GPIO_SET_DAT(BACKLIGHT, 1);
		bl_current = 1;
	} else {
		//printf("backlight off\n");
		GPIO_SET_DAT(BACKLIGHT, 0);
		bl_current = 0;
	}
}

int do_bl(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	if (argc != 2 && argc != 3)
		return cmd_usage(cmdtp);

	if (!strcmp(argv[1], "on"))
		bl_control(1);
	else
	if (!strcmp(argv[1], "off"))
		bl_control(0);
	else
	if (!strcmp(argv[1], "blink")) {
		int interval = 1000;
		int status = 1;
		if (argc == 3)
			interval = simple_strtoul(argv[2], NULL, 16);
		do {
			int time = interval;
			bl_control(status);
			status = status ? 0 : 1;
			while(time--) {
				udelay(900);//udelay(1000);
				if (ctrlc()) {
					interval = 0;
					break;
				}
			}
		} while(interval);
	}

	return 0;
}

U_BOOT_CMD(
	bl, 3, 1, do_bl,
	"backlight control",
	"{on|off|blink <msec>}\n"
	"    - backlight control"
);

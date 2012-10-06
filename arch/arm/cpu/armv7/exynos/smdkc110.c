/*
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * (C) Copyright 2002
 * David Mueller, ELSOFT AG, <d.mueller@elsoft.ch>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <regs.h>
#include <asm/io.h>


/* ------------------------------------------------------------------------- */
#define Outp32(addr, data)	(*(volatile u32 *)(addr) = (data))
#define Inp32(_addr)		readl(_addr)
#define SMC9115_Tacs	(0x0)	// 0clk		address set-up
#define SMC9115_Tcos	(0x4)	// 4clk		chip selection set-up
#define SMC9115_Tacc	(0xe)	// 14clk	access cycle
#define SMC9115_Tcoh	(0x1)	// 1clk		chip selection hold
#define SMC9115_Tah	(0x4)	// 4clk		address holding time
#define SMC9115_Tacp	(0x6)	// 6clk		page mode access cycle
#define SMC9115_PMC	(0x0)	// normal(1data)page mode configuration

#define SROM_DATA16_WIDTH(x)	(1<<((x*4)+0))
#define SROM_WAIT_ENABLE(x)	(1<<((x*4)+1))
#define SROM_BYTE_ENABLE(x)	(1<<((x*4)+2))

/* ------------------------------------------------------------------------- */
#define DM9000_Tacs	(0x0)	// 0clk		address set-up
#define DM9000_Tcos	(0x4)	// 4clk		chip selection set-up
#define DM9000_Tacc	(0xE)	// 14clk	access cycle
#define DM9000_Tcoh	(0x1)	// 1clk		chip selection hold
#define DM9000_Tah	(0x4)	// 4clk		address holding time
#define DM9000_Tacp	(0x6)	// 6clk		page mode access cycle
#define DM9000_PMC	(0x0)	// normal(1data)page mode configuration

#if 1/*add by huangyuxiang for updating images 2011.2.12*/
#define MY_RED 		0xf80000
//#define MY_BLUE 	0x1024C0
#define MY_BLUE 	0x0201f0
#define MY_GREEN 	0x07e000
#define MY_YELLOW 	0xffff00
#define MY_WHITE 	0xffffff
#define MY_BLACK 	0x000000
#define MY_LCD_HIGHT	768
#define MY_LCD_WIDTH	1024

#define ERROR_READ -4
#define ERROR_IGNORE -5
#define ERROR_ERASE -6
#define ERROR_WRITE -7
int stage_write=1;
#endif/*add end*/

static inline void delay(unsigned long loops)
{
	__asm__ volatile ("1:\n" "subs %0, %1, #1\n" "bne 1b":"=r" (loops):"0"(loops));
}

/*
 * Miscellaneous platform dependent initialisations
 */

static void dm9000_pre_init(void)
{
	unsigned int tmp;

#if defined(DM9000_16BIT_DATA)
	SROM_BW_REG &= ~(0xf << 20);
	SROM_BW_REG |= (0<<23) | (0<<22) | (0<<21) | (1<<20);

#else	
	SROM_BW_REG &= ~(0xf << 20);
	SROM_BW_REG |= (0<<19) | (0<<18) | (0<<16);
#endif
	SROM_BC5_REG = ((0<<28)|(1<<24)|(5<<16)|(1<<12)|(4<<8)|(6<<4)|(0<<0));

	tmp = MP01CON_REG;
	tmp &=~(0xf<<20);
	tmp |=(2<<20);
	MP01CON_REG = tmp;
}


int board_init(void)
{
	DECLARE_GLOBAL_DATA_PTR;
#ifdef CONFIG_DRIVER_SMC911X
	smc9115_pre_init();
#endif

#ifdef CONFIG_DRIVER_DM9000
	dm9000_pre_init();
#endif

	gd->bd->bi_arch_number = MACH_TYPE;
	gd->bd->bi_boot_params = (PHYS_SDRAM_1+0x100);

	return 0;
}

int dram_init(void)
{
	DECLARE_GLOBAL_DATA_PTR;

	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;

#if defined(PHYS_SDRAM_2)
	gd->bd->bi_dram[1].start = PHYS_SDRAM_2;
	gd->bd->bi_dram[1].size = PHYS_SDRAM_2_SIZE;
#endif

	return 0;
}

#ifdef BOARD_LATE_INIT
#if defined(CONFIG_BOOT_NAND)
int board_late_init (void)
{
	uint *magic = (uint*)(PHYS_SDRAM_1);
	char boot_cmd[100];

	if ((0x24564236 == magic[0]) && (0x20764316 == magic[1])) {
		sprintf(boot_cmd, "nand erase 0 40000;nand write %08x 0 40000", PHYS_SDRAM_1 + 0x8000);
		magic[0] = 0;
		magic[1] = 0;
		printf("\nready for self-burning U-Boot image\n\n");
		setenv("bootdelay", "0");
		setenv("bootcmd", boot_cmd);
	}

	return 0;
}
#elif defined(CONFIG_BOOT_MOVINAND)
int board_late_init (void)
{
	uint *magic = (uint*)(PHYS_SDRAM_1);
	char boot_cmd[100];
	int hc;

	hc = (magic[2] & 0x1) ? 1 : 0;

	if ((0x24564236 == magic[0]) && (0x20764316 == magic[1])) {
		sprintf(boot_cmd, "movi init %d %d;movi write u-boot %08x", magic[3], hc, PHYS_SDRAM_1 + 0x8000);
		magic[0] = 0;
		magic[1] = 0;
		printf("\nready for self-burning U-Boot image\n\n");
		setenv("bootdelay", "0");
		setenv("bootcmd", boot_cmd);
	}

	return 0;
}
#else
int board_late_init (void)
{
	return 0;
}
#endif
#endif

#ifdef CONFIG_DISPLAY_BOARDINFO
int checkboard(void)
{
#ifdef CONFIG_MCP_SINGLE
#if defined(CONFIG_VOGUES)
	printf("\nBoard:   VOGUESV210\n");
#else
	printf("\nBoard:   SMDKV210\n");
#endif //CONFIG_VOGUES
#else
	printf("\nBoard:   SMDKC110\n");
#endif
	return (0);
}
#endif

#ifdef CONFIG_ENABLE_MMU

#ifdef CONFIG_MCP_SINGLE
ulong virt_to_phy_smdkc110(ulong addr)
{
	if ((0xc0000000 <= addr) && (addr < 0xd0000000))
		return (addr - 0xc0000000 + 0x20000000);
	else
		printf("The input address don't need "\
			"a virtual-to-physical translation : %08lx\n", addr);

	return addr;
}
#else
ulong virt_to_phy_smdkc110(ulong addr)
{
	if ((0xc0000000 <= addr) && (addr < 0xd0000000))
		return (addr - 0xc0000000 + 0x30000000);
	else if ((0x30000000 <= addr) && (addr < 0x50000000))
		return addr;
	else
		printf("The input address don't need "\
			"a virtual-to-physical translation : %08lx\n", addr);

	return addr;
}
#endif

#endif

#if defined(CONFIG_CMD_NAND) && defined(CFG_NAND_LEGACY)
#include <linux/mtd/nand.h>
extern struct nand_chip nand_dev_desc[CFG_MAX_NAND_DEVICE];
void nand_init(void)
{
	nand_probe(CFG_NAND_BASE);
        if (nand_dev_desc[0].ChipID != NAND_ChipID_UNKNOWN) {
                print_size(nand_dev_desc[0].totlen, "\n");
        }
}
#endif

#if 1/*add by huangyuxiang for update images 2011.2.10*/
void DrawLogo_bk(u32 * pVideoBuffer, u32 uWidth, u32 uHeight,u32 colour)
{
	int i;
	u32 *pvBuff;

	pvBuff = pVideoBuffer;
	for (i = 0; i < uWidth * uHeight ; i++, pvBuff++)
		*pvBuff = colour;
}

void Draw_line(u32 *  pVideoBuffer, u32 x,u32 y, u32 l_Width, u32 l_Height,u32 colour)
{
	int i,j;
	u32 W=MY_LCD_WIDTH;
	u32 H=MY_LCD_HIGHT;
	u32 *p=pVideoBuffer;
	u32 *tmp_p;
	u32 start;
	
	for(i=1;i<=l_Height;i++)
	{
		start=W*(y+i-1)+x;
		tmp_p=p+start;
		for(j=1;j<=l_Width;j++,tmp_p++)
			*tmp_p=colour;
	}
		
}

void Draw_rectangle(u32 *  pVideoBuffer, u32 x,u32 y, u32 l_Width,u32 T_Height,u32 colour)
{
	u32 T_Width=l_Width*7+454;
	
	Draw_line((u32 * )CFG_LCD_FBUFFER, x, y, T_Width, l_Width,colour);
	Draw_line((u32 * )CFG_LCD_FBUFFER, x, y+T_Height+l_Width, T_Width, l_Width,colour);
	
	Draw_line((u32 * )CFG_LCD_FBUFFER, x, y+l_Width, l_Width, T_Height,colour);
	Draw_line((u32 * )CFG_LCD_FBUFFER, x+12+l_Width,y+l_Width,l_Width, T_Height,colour);
	Draw_line((u32 * )CFG_LCD_FBUFFER, x+28+l_Width*2,y+l_Width,l_Width, T_Height,colour);
	Draw_line((u32 * )CFG_LCD_FBUFFER, x+48+l_Width*3,y+l_Width,l_Width, T_Height,colour);
#if 0
	Draw_line((u32 * )CFG_LCD_FBUFFER, x+70+l_Width*4,y+l_Width,l_Width, T_Height,colour);
	Draw_line((u32 * )CFG_LCD_FBUFFER, x+91+l_Width*5,y+l_Width,l_Width, T_Height,colour);
	Draw_line((u32 * )CFG_LCD_FBUFFER, x+454+l_Width*6,y+l_Width,l_Width, T_Height,colour);
#else

	Draw_line((u32 * )CFG_LCD_FBUFFER, x+128+l_Width*4,y+l_Width,l_Width, T_Height,colour);
	Draw_line((u32 * )CFG_LCD_FBUFFER, x+149+l_Width*5,y+l_Width,l_Width, T_Height,colour);
	Draw_line((u32 * )CFG_LCD_FBUFFER, x+449+5+l_Width*6,y+l_Width,l_Width, T_Height,colour);
#endif
}

void Draw_bar(u32 * const pVideoBuffer,u32 ignore,u32 x,u32 y,u32 w,u32 H)
{
	int i;
	static int right=0;
	
	if(ignore)
	{
		if(ignore==1)
			Draw_line((u32 * )CFG_LCD_FBUFFER, x, y, w, H,MY_WHITE);
		else if(ignore==2)
			Draw_line((u32 * )CFG_LCD_FBUFFER, x, y, w, H,MY_RED);
		else if(ignore==3)
			Draw_line((u32 * )CFG_LCD_FBUFFER, x, y, w, H,MY_GREEN);
	}
	else
	{
		
		if(right<=w-3)
		{
			Draw_line((u32 * )CFG_LCD_FBUFFER, x, y, right, H,MY_GREEN);
			right++;
		}
	}
	
	if(right>=w-3)
		right=0;
		
}

void Draw_stage(u32 lastpix,size_t written,size_t len)
{
	int process;
	switch(stage_write)
	{
		case 1:
			if(lastpix==1)
			Draw_bar((u32 * )CFG_LCD_FBUFFER, 3, 265, 419, 12, 40);
			else if(lastpix==2)
			{
				Draw_bar((u32 * )CFG_LCD_FBUFFER, 2, 265, 419, 12, 40);
				while(1);
			}
			else
			Draw_bar((u32 * )CFG_LCD_FBUFFER, 0, 265, 419, 12, 40);
		break;

		case 2:
			if(lastpix==1)
			Draw_bar((u32 * )CFG_LCD_FBUFFER, 3, 282, 419, 16, 40);
			else if(lastpix==2)
			{
				Draw_bar((u32 * )CFG_LCD_FBUFFER, 2, 282, 419, 16, 40);
				while(1);
			}
			else
			Draw_bar((u32 * )CFG_LCD_FBUFFER, 0, 282, 419, 16, 40);
		break;

		case 3:
			if(lastpix==1)
			Draw_bar((u32 * )CFG_LCD_FBUFFER, 3, 303, 419, 20, 40);
			else if(lastpix==2)
			{
				Draw_bar((u32 * )CFG_LCD_FBUFFER, 2, 303, 419, 20, 40);
				while(1);
			}
			else
			Draw_bar((u32 * )CFG_LCD_FBUFFER, 0, 303, 419, 20, 40);
		break;
#if 0
		case 4:
			if(lastpix==1)
			Draw_bar((u32 * )CFG_LCD_FBUFFER, 3, 328, 419, 22, 40);
			else if(lastpix==2)
			{
				Draw_bar((u32 * )CFG_LCD_FBUFFER, 2, 328, 419, 22, 40);
				while(1);
			}
			else
			Draw_bar((u32 * )CFG_LCD_FBUFFER, 0, 328, 419, 22, 40);
		break;

		case 5:
			if(lastpix==1)
			Draw_bar((u32 * )CFG_LCD_FBUFFER, 3,355 , 419, 21, 40);
			else if(lastpix==2)
			{
				Draw_bar((u32 * )CFG_LCD_FBUFFER, 2,355 , 419, 21, 40);
				while(1);
			}
			else
			Draw_bar((u32 * )CFG_LCD_FBUFFER, 0,355 , 419, 21, 40);
		break;

		case 6:
			if(lastpix==1)
			Draw_bar((u32 * )CFG_LCD_FBUFFER,3,381 , 419, 363, 40);
			else if(lastpix==2)
			{
				Draw_bar((u32 * )CFG_LCD_FBUFFER,2, 381, 419, 363, 40);
				while(1);
			}
			else
			{
				
				process=((written*4)/(len/25))+1;
				printf("###%3d\n",((written*4)/(len/25))+1);
				Draw_bar((u32 * )CFG_LCD_FBUFFER,1, 381, 419, (3*process+process/2), 40);
			}
		break;
#else

		case 4:
			if(lastpix==1)
			Draw_bar((u32 * )CFG_LCD_FBUFFER, 3, 328, 419, 80, 40);
			else if(lastpix==2)
			{
				Draw_bar((u32 * )CFG_LCD_FBUFFER, 2, 328, 419, 80, 40);
				while(1);
			}
			else
			{
				process=((written*4)/(len/25))+1;
			//	printf("###%3d\n",((written*4)/(len/25))+1);
				Draw_bar((u32 * )CFG_LCD_FBUFFER, 3, 328, 419, (process/5)*4, 40);
			}
		break;

		case 5:
			if(lastpix==1)
			Draw_bar((u32 * )CFG_LCD_FBUFFER, 3,413 , 419, 21, 40);
			else if(lastpix==2)
			{
				Draw_bar((u32 * )CFG_LCD_FBUFFER, 2,413 , 419, 21, 40);
				while(1);
			}
			else
			Draw_bar((u32 * )CFG_LCD_FBUFFER, 0,413 , 419, 21, 40);
		break;

		case 6:
			if(lastpix==1)
			Draw_bar((u32 * )CFG_LCD_FBUFFER,3,424+15 , 419, 305, 40);
			else if(lastpix==2)
			{
				Draw_bar((u32 * )CFG_LCD_FBUFFER,2, 424+15, 419, 305, 40);
				while(1);
			}
			else
			{
				
				process=((written*4)/(len/25))+1;
			//	printf("###%3d\n",((written*4)/(len/25))+1);
				Draw_bar((u32 * )CFG_LCD_FBUFFER,3, 424+15, 419, (3*process+process/20), 40);
			}
		break;




#endif			
	}
}

#if 0
void Draw_card_nofat(void)
{
	Draw_bar((u32 * )CFG_LCD_FBUFFER, 2, 265, 419, 12, 40);
	Draw_bar((u32 * )CFG_LCD_FBUFFER, 2, 282, 419, 16, 40);
	Draw_bar((u32 * )CFG_LCD_FBUFFER, 2, 303, 419, 20, 40);
	Draw_bar((u32 * )CFG_LCD_FBUFFER, 2, 328, 419, 368, 40);
	//Draw_bar((u32 * )CFG_LCD_FBUFFER, 2, 572, 419, 21, 40);
	//Draw_bar((u32 * )CFG_LCD_FBUFFER,2, 598, 419, 15, 40);
	while(1);
}
#endif

void Draw_ignore(u32 ret,u32 x,u32 y,u32 w,u32 H)
{
	if(ret==ERROR_WRITE)	
	{
		Draw_bar((u32 * )CFG_LCD_FBUFFER, 2, x, y, w, H);
		stage_write+=1;
//		while(1);
	}
	else if(ret==ERROR_ERASE)
	{
		Draw_bar((u32 * )CFG_LCD_FBUFFER, 2, x, y, w, H);
		stage_write+=1;
//		while(1);
	}
	else if(ret==ERROR_IGNORE)	
	{
		Draw_bar((u32 * )CFG_LCD_FBUFFER, 1, x, y, w, H);
		stage_write+=1;
	}
	
}

void Draw_first_logo(void)
{
	int max_x;
	run_command("onenand read 0x30000000 0x100000 0x200000",0);
	unsigned long i,j;
	unsigned long* pBuffer = (unsigned long*)CFG_LCD_FBUFFER;
//	unsigned char   * pBitmap = (unsigned char *)(0x30000000+54);
	unsigned short* pBitmap = (unsigned short*)(0x30000000+54);
	unsigned long iBitmapData;
	for(i=0;i<768;i++)
	{

		max_x=1024;
#if 1
		for(j=0;j<max_x;j++)
		{
			iBitmapData = 0xFF<<24;
			iBitmapData |= ((*pBitmap>>8)&0xF8)<<16;
			iBitmapData |= ((*pBitmap>>5)&0x1F)<<11;
			iBitmapData |= ((*pBitmap<<3)&0xF8);
			*(pBuffer+(i*1024)+j) = iBitmapData;
			pBitmap++;
		}
#else  

		for(j=0;j<max_x;j++)
		{
			iBitmapData = 0x0;
			iBitmapData = 0xFF<<24;
			iBitmapData |= ((*(pBitmap+2))&0xFF)<<16;
			iBitmapData |= ((*(pBitmap+1))&0xFF)<<8;
			iBitmapData |= ((*pBitmap<<0)&0xFF);
			*(pBuffer+(i*1024)+j) = iBitmapData;
			pBitmap+=3;
		}

#endif
	}
	LCD_turnon(1);

}

void s5p_lcd_word( unsigned char * buf,int width,int height)
{
	char *tmp_point=buf;
	unsigned long i, j, sx, sy ;
	unsigned long* pBuffer = (unsigned long*)CFG_LCD_FBUFFER;
	unsigned short* pBitmap = tmp_point;//(unsigned short*)buf;
	unsigned long iBitmapData;

	sx = (900-width)>>1;
	sy = (768-height)>>1;
	sy -= 20;
	sx += 50;

	for (i=sy; i<sy+height; i++)
	{
		for (j=sx; j<sx+width; j++)
		{
		#if 0/*modified by huangyuxiang 2011.2.16*/
			iBitmapData = 0xFF<<24;
			iBitmapData |= ((*pBitmap>>8)&0xF8)<<16;
			iBitmapData |= ((*pBitmap>>5)&0x3F)<<10;
			iBitmapData |= ((*pBitmap<<3)&0xF8);
		#else
		//	*pBitmap=0x1117;
			iBitmapData = 0xFF<<24;
	//		printf("#######0x%x  %x%x\n",*pBitmap,*buf,*(buf++));
		//	udelay(1000000);
		//	iBitmapData |= ((*pBitmap>>8)&0xF8)<<16;
			iBitmapData |= ((*pBitmap>>11)&0x1f)<<16;
			iBitmapData |= ((*pBitmap>>5)&0x3F)<<8;
			iBitmapData |= ((*pBitmap<<3)&0xF8);
		#endif/*modify end */
			*(pBuffer+(i*1024)+j) = iBitmapData;
			pBitmap++;
		}
	}
}





int do_keyboot(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
#if 1
	extern unsigned char word[23040];
	int picture_color[5]={MY_RED,MY_BLUE,MY_GREEN,MY_WHITE,MY_BLACK};
	int picture_pri,picture_next;
	extern int global_color;
	unsigned int gpio_state_CA,color_flag=0;
	unsigned int gpio_state_VD;
	int key_val;
#if 0
	volatile unsigned int * GPH3CON=(volatile unsigned int*)0xe0200c60;
	volatile unsigned int * GPH2CON=(volatile unsigned int*)0xe0200c40;
	volatile unsigned int * GPH3DAT=(volatile unsigned int*)0xe0200c64;
	volatile unsigned int * GPH2DAT=(volatile unsigned int*)0xe0200c44;

	*GPH2CON=0x00000010;
	*GPH2DAT=0x00000000;
	*GPH3CON=0x00000000;
	*GPH3DAT=0x00000002;
	gpio_state_VD=(*GPH3DAT)&1;
	gpio_state_CA=(*GPH3DAT)>>1;
#endif
	writel((readl(ELFIN_GPIO_BASE+GPH2CON_OFFSET)&(~0xFFF)), ELFIN_GPIO_BASE+GPH2CON_OFFSET);
	writel(readl(ELFIN_GPIO_BASE+GPH2PUD_OFFSET)&(~0x3F)|(0xa<<0), ELFIN_GPIO_BASE+GPH2PUD_OFFSET);	
	writel(readl(ELFIN_GPIO_BASE+GPH3CON_OFFSET)&(~0xFFFF)|(0x1<<0)|(0x1<<4), ELFIN_GPIO_BASE+GPH3CON_OFFSET);
	
	writel((readl(ELFIN_GPIO_BASE+GPH3DAT_OFFSET)&(~(0x3<<0))), ELFIN_GPIO_BASE+GPH3DAT_OFFSET);

	key_val=(readl(ELFIN_GPIO_BASE+GPH2DAT_OFFSET)&0x3);
	if ((key_val==0x1)||(key_val==0x2))
//	if(1)
	if(key_val==0x1)
	{
	//	printf("START updating  systems\n");
		LCD_turnon(1);
		DrawLogo_bk((u32*)CFG_LCD_FBUFFER, 768, 1024,MY_BLUE);
		Draw_line((u32 *)CFG_LCD_FBUFFER,0,767,1024,1,MY_RED);
		Draw_rectangle((u32 * )CFG_LCD_FBUFFER,260,414,5,40,MY_BLACK);
		run_command("sdfuse flashall",0);
	}
	else if(key_val==0x2)
	{
		//DrawLogo_bk((u32*)CFG_LCD_FBUFFER, 768, 1024,MY_RED);
			
		DrawLogo_bk((u32*)CFG_LCD_FBUFFER, 768, 1024,MY_BLUE);
		LCD_turnon(1);
		s5p_lcd_word(&word[0],465,25);
		udelay(1000000);
		run_command("sdfuse erase userdata",0);
		run_command("sdfuse erase cache",0);
	}
	if((key_val==0x1)||(key_val==0x2))
		run_command("reset",0);
#if 0
	else
	{
		void LCD_turnon(int flag);
		LCD_turnon(1);
		printf("lcd_test");
		while(1)
		{
			
		//	udelay(50000000);
			picture_pri=(*GPH3DAT)&1;
			picture_next=((*GPH3DAT)>>1)&1;

			printf("####pri=%d\n",picture_pri);
			printf("\n#####next=%d\n",picture_next);
			if(!(picture_pri&1))
			{
				udelay(10000);
				picture_pri=(*GPH3DAT)&1;
				while(!(picture_pri&1))
					picture_pri=(*GPH3DAT)&1;
				{
					if(color_flag==4)
						color_flag=0;
					else
						color_flag++;
				}
			}
			else if(!(picture_next&1))
			{
			//	udelay(10000);
				picture_next=((*GPH3DAT)>>1)&1;
				 while(!(picture_next&1))
					picture_next=((*GPH3DAT)>>1)&1;
				{
					if(color_flag==0)
						color_flag=4;
					else
						color_flag--;
				}
			}
			DrawLogo_bk((u32*)CFG_LCD_FBUFFER, 768, 1024,picture_color[color_flag]);
		}
		while(1);
	}
#endif
#else
	DrawLogo_bk((u32*)CFG_LCD_FBUFFER, 768, 1366,MY_BLUE);
	Draw_rectangle((u32 * )CFG_LCD_FBUFFER,260,414,5,40,MY_BLACK);
	run_command("sdfuse flashall",0);
#endif
	return 0;
}

U_BOOT_CMD(
        keyboot, 1, 0, do_keyboot, NULL, NULL
);
#endif/*add end*/


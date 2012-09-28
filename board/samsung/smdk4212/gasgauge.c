/*
 * (C) Copyright 2012 Coasia Co. Ltd
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <common.h>
#include <asm/arch/pmic.h>
#include <asm/arch/gpio.h>


#define Outp32(addr, data) (*(volatile u32 *)(addr) = (data))
#define Inp32(addr) ((*(volatile u32 *)(addr)))

#define GPA0CON		*(volatile unsigned long *)(0x11400000)
#define GPA0DAT		*(volatile unsigned long *)(0x11400004)
#define GPA0PUD		*(volatile unsigned long *)(0x11400008)
#define GPA0DRV		*(volatile unsigned long *)(0x1140000C)

#define IIC2_ESCL_X4	GPA0DRV |= (0x3<<14)
#define IIC2_ESDA_X4	GPA0DRV |= (0x3<<12)

#define IIC2_ESCL_Hi	GPA0DAT |= (0x1<<7)
#define IIC2_ESCL_Lo	GPA0DAT &= ~(0x1<<7)
#define IIC2_ESDA_Hi	GPA0DAT |= (0x1<<6)
#define IIC2_ESDA_Lo	GPA0DAT &= ~(0x1<<6)

#define IIC2_ESDA_INP	GPA0CON &= ~(0xf<<24)
#define IIC2_ESDA_OUTP	GPA0CON = (GPA0CON & ~(0xf<<24))|(0x1<<24)
#define IIC2_ESCL_INP	GPA0CON &= ~(0xf<<28)
#define IIC2_ESCL_OUTP	GPA0CON = (GPA0CON & ~(0xf<<28))|(0x1<<28)

extern void Delay(void);

void IIC2_SCLH_SDAH(void)
{
	IIC2_ESCL_Hi;
	IIC2_ESDA_Hi;
	Delay();
}

void IIC2_SCLH_SDAL(void)
{
	IIC2_ESCL_Hi;
	IIC2_ESDA_Lo;
	Delay();
}

void IIC2_SCLL_SDAH(void)
{
	IIC2_ESCL_Lo;
	IIC2_ESDA_Hi;
	Delay();
}

void IIC2_SCLL_SDAL(void)
{
	IIC2_ESCL_Lo;
	IIC2_ESDA_Lo;
	Delay();
}


void IIC2_ELow(void)
{
	IIC2_SCLL_SDAL();
	IIC2_SCLH_SDAL();
	IIC2_SCLH_SDAL();
	IIC2_SCLL_SDAL();
}

void IIC2_EHigh(void)
{
	IIC2_SCLL_SDAH();
	IIC2_SCLH_SDAH();
	IIC2_SCLH_SDAH();
	IIC2_SCLL_SDAH();
}

void IIC2_EStart(void)
{
	IIC2_SCLH_SDAH();
	IIC2_SCLH_SDAL();
	Delay();
	IIC2_SCLL_SDAL();
}

void IIC2_EEnd(void)
{
	IIC2_SCLL_SDAL();
	IIC2_SCLH_SDAL();
	Delay();
	IIC2_SCLH_SDAH();
}

void IIC2_EAck_write(void)
{
	unsigned long ack;

	IIC2_ESDA_INP;			// Function <- Input

	IIC2_ESCL_Lo;
	Delay();
	IIC2_ESCL_Hi;
	Delay();
	ack = GPA0DAT;
	IIC2_ESCL_Hi;
	Delay();
	IIC2_ESCL_Hi;
	Delay();

	IIC2_ESDA_OUTP;			// Function <- Output (SDA)

	ack = (ack>>6)&0x1;
//	while(ack!=0);

	IIC2_SCLL_SDAL();
}

void IIC2_EAck_read(void)
{
	IIC2_ESDA_OUTP;			// Function <- Output

	IIC2_ESCL_Lo;
	IIC2_ESCL_Lo;
	//IIC2_ESDA_Lo;
	IIC2_ESDA_Hi;
	IIC2_ESCL_Hi;
	IIC2_ESCL_Hi;

	IIC2_ESDA_INP;			// Function <- Input (SDA)

	IIC2_SCLL_SDAL();
}

void IIC2_ESetport(void)
{
	GPA0PUD &= ~(0xf<<6);	// Pull Up/Down Disable	SCL, SDA

	IIC2_ESCL_X4;
	IIC2_ESDA_X4;

	IIC2_ESCL_Hi;
	IIC2_ESDA_Hi;
	IIC2_ESCL_OUTP;		// Function <- Output (SCL)
	IIC2_ESDA_OUTP;		// Function <- Output (SDA)

	Delay();
}

void IIC2_EWrite (unsigned char ChipId, unsigned char IicAddr, unsigned char IicData)
{
	unsigned long i;

	IIC2_EStart();

////////////////// write chip id //////////////////
	for(i = 7; i>0; i--)
	{
		if((ChipId >> i) & 0x0001)
			IIC2_EHigh();
		else
			IIC2_ELow();
	}

	IIC2_ELow();	// write

	IIC2_EAck_write();	// ACK

////////////////// write reg. addr. //////////////////
	for(i = 8; i>0; i--)
	{
		if((IicAddr >> (i-1)) & 0x0001)
			IIC2_EHigh();
		else
			IIC2_ELow();
	}

	IIC2_EAck_write();	// ACK

////////////////// write reg. data. //////////////////
	for(i = 8; i>0; i--)
	{
		if((IicData >> (i-1)) & 0x0001)
			IIC2_EHigh();
		else
			IIC2_ELow();
	}

	IIC2_EAck_write();	// ACK

	IIC2_EEnd();
}

void IIC2_ERead (unsigned char ChipId, unsigned char IicAddr, unsigned char *IicData)
{
	unsigned long i, reg;
	unsigned char data = 0;

	IIC2_EStart();

////////////////// write chip id //////////////////
	for(i = 7; i>0; i--)
	{
		if((ChipId >> i) & 0x0001)
			IIC2_EHigh();
		else
			IIC2_ELow();
	}

	IIC2_ELow();	// write

	IIC2_EAck_write();	// ACK

////////////////// write reg. addr. //////////////////
	for(i = 8; i>0; i--)
	{
		if((IicAddr >> (i-1)) & 0x0001)
			IIC2_EHigh();
		else
			IIC2_ELow();
	}

	IIC2_EAck_write();	// ACK

	IIC2_EStart();

////////////////// write chip id //////////////////
	for(i = 7; i>0; i--)
	{
		if((ChipId >> i) & 0x0001)
			IIC2_EHigh();
		else
			IIC2_ELow();
	}

	IIC2_EHigh();	// read

	IIC2_EAck_write();	// ACK

////////////////// read reg. data. //////////////////
	IIC2_ESDA_INP;

	IIC2_ESCL_Lo;
	IIC2_ESCL_Lo;
	Delay();

	for(i = 8; i>0; i--)
	{
		IIC2_ESCL_Lo;
		IIC2_ESCL_Lo;
		Delay();
		IIC2_ESCL_Hi;
		IIC2_ESCL_Hi;
		Delay();
		reg = GPA0DAT;
		IIC2_ESCL_Hi;
		IIC2_ESCL_Hi;
		Delay();
		IIC2_ESCL_Lo;
		IIC2_ESCL_Lo;
		Delay();

		reg = (reg >> 6) & 0x1;
		data |= reg << (i-1);
	}

	IIC2_EAck_read();	// ACK
	IIC2_ESDA_OUTP;

	IIC2_EEnd();

	*IicData = data;
}


void CheckBatteryLow(void)
{
	int SOC_percent = 0;
	u8 read_dataH=1;
	u8 read_dataL=2;
	int charger_status = 0;
	int gpio_status = 0;

	GPIO_Init();

	//#GPIO_PM2301_LP
	GPIO_SetFunctionEach(eGPIO_X1, eGPIO_7, 0);
	GPIO_SetPullUpDownEach(eGPIO_X1, eGPIO_7,0);
	//GPIO_CHARGER_ONLINE
	GPIO_SetFunctionEach(eGPIO_X0, eGPIO_7, 0);
	GPIO_SetPullUpDownEach(eGPIO_X0, eGPIO_7,0);

	gpio_status = GPIO_GetDataAll(eGPIO_X1);
	gpio_status = GPIO_GetDataAll(eGPIO_X2);
	gpio_status = GPIO_GetDataAll(eGPIO_X0);
	charger_status = GPIO_GetDataEach(eGPIO_X0, eGPIO_7);
	printf("charger_status=%d\n",charger_status);

	IIC2_ESetport();
	/* read ID */
	IIC2_ERead(0x6C, 0x04, &read_dataH);
	IIC2_ERead(0x6C, 0x05, &read_dataL);
	SOC_percent = ((read_dataH << 8) + read_dataL) / 512;
	printf("SOC_percent=%d\n",SOC_percent);
	/* Check batt percent is 0% and not plug in DC, then power off devcies */
	if(SOC_percent<1 && charger_status == 1) {
		//PS_HOLD pull low to turn off PMIC
		Outp32(0x1002330C, 0x5200);
		while(1);
	}
}

/**
 * Mupen64 - pif.c
 * Copyright (C) 2002 Hacktarux
 *
 * Mupen64 homepage: http://mupen64.emulation64.com
 * email address: hacktarux@yahoo.fr
 *
 * If you want to contribute to the project please contact
 * me first (maybe someone is already making what you are
 * planning to do).
 *
 *
 * This program is free software; you can redistribute it and/
 * or modify it under the terms of the GNU General Public Li-
 * cence as published by the Free Software Foundation; either
 * version 2 of the Licence, or any later version.
 *
 * This program is distributed in the hope that it will be use-
 * ful, but WITHOUT ANY WARRANTY; without even the implied war-
 * ranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public Licence for more details.
 *
 * You should have received a copy of the GNU General Public
 * Licence along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139,
 * USA.
 *
**/

//#include "../config.h"
#ifdef _DEBUG
//#define DEBUG_PIF //don't define if you don't need spam
#endif

#include <stdio.h>
#include <stdlib.h>
#ifndef __WIN32__
#include "../main/winlnxdefs.h"
#else
#include <windows.h>
#endif

#include "memory.h"
#include "pif.h"
#include "pif2.h"
#include "../r4300/r4300.h"
#include "../r4300/interupt.h"
#include "../main/plugin.h"
#include "../main/guifuncs.h"
#include "../main/vcr.h"
#include "../main/savestates.h"
#include "../main/win/Config.h"
#include "../lua/LuaConsole.h"

unsigned char eeprom[0x800];
unsigned char mempack[4][0x8000];

void check_input_sync(unsigned char* value);

//#define DEBUG_PIF
#ifdef DEBUG_PIF
void print_pif()
{
	int i;
	for (i = 0; i < (64 / 8); i++)
		printf("%x %x %x %x | %x %x %x %x\n",
			PIF_RAMb[i * 8 + 0], PIF_RAMb[i * 8 + 1], PIF_RAMb[i * 8 + 2], PIF_RAMb[i * 8 + 3],
			PIF_RAMb[i * 8 + 4], PIF_RAMb[i * 8 + 5], PIF_RAMb[i * 8 + 6], PIF_RAMb[i * 8 + 7]);
	//getchar();
}
#endif

void EepromCommand(BYTE* Command)
{
	switch (Command[2])
	{
	case 0: // check
		if (Command[1] != 3)
		{
			Command[1] |= 0x40;
			if ((Command[1] & 3) > 0)
				Command[3] = 0;
			if ((Command[1] & 3) > 1)
				Command[4] = ROM_SETTINGS.eeprom_16kb == 0 ? 0x80 : 0xc0;
			if ((Command[1] & 3) > 2)
				Command[5] = 0;
		}
		else
		{
			Command[3] = 0;
			Command[4] = ROM_SETTINGS.eeprom_16kb == 0 ? 0x80 : 0xc0;
			Command[5] = 0;
		}
		break;
	case 4: // read
	{
		char* filename;
		FILE* f;
		int i;
		filename = (char*)malloc(strlen(get_savespath()) +
			strlen(ROM_SETTINGS.goodname) + 4 + 1);
		strcpy(filename, get_savespath());
		strcat(filename, ROM_SETTINGS.goodname);
		strcat(filename, ".eep");
		f = fopen(filename, "rb");
		if (f)
		{
			fread(eeprom, 1, 0x800, f);
			fclose(f);
		}
		else for (i = 0; i < 0x800; i++) eeprom[i] = 0;
		free(filename);
		memcpy(&Command[4], eeprom + Command[3] * 8, 8);
	}
	break;
	case 5: // write
	{
		char* filename;
		FILE* f;
		int i;
		filename = (char*)malloc(strlen(get_savespath()) +
			strlen(ROM_SETTINGS.goodname) + 4 + 1);
		strcpy(filename, get_savespath());
		strcat(filename, ROM_SETTINGS.goodname);
		strcat(filename, ".eep");
		f = fopen(filename, "rb");
		if (f)
		{
			fread(eeprom, 1, 0x800, f);
			fclose(f);
		}
		else for (i = 0; i < 0x800; i++) eeprom[i] = 0;
		memcpy(eeprom + Command[3] * 8, &Command[4], 8);
		f = fopen(filename, "wb");
		fwrite(eeprom, 1, 0x800, f);
		fclose(f);
		free(filename);
	}
	break;
	default:
		printf("unknown command in EepromCommand : %x\n", Command[2]);
	}
}

void format_mempacks()
{
	unsigned char init[] =
	{
   0x81,0x01,0x02,0x03, 0x04,0x05,0x06,0x07, 0x08,0x09,0x0a,0x0b, 0x0c,0x0d,0x0e,0x0f,
   0x10,0x11,0x12,0x13, 0x14,0x15,0x16,0x17, 0x18,0x19,0x1a,0x1b, 0x1c,0x1d,0x1e,0x1f,
   0xff,0xff,0xff,0xff, 0x05,0x1a,0x5f,0x13, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
   0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff, 0xff,0xff,0x01,0xff, 0x66,0x25,0x99,0xcd,
   0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
   0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
   0xff,0xff,0xff,0xff, 0x05,0x1a,0x5f,0x13, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
   0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff, 0xff,0xff,0x01,0xff, 0x66,0x25,0x99,0xcd,
   0xff,0xff,0xff,0xff, 0x05,0x1a,0x5f,0x13, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
   0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff, 0xff,0xff,0x01,0xff, 0x66,0x25,0x99,0xcd,
   0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
   0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
   0xff,0xff,0xff,0xff, 0x05,0x1a,0x5f,0x13, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
   0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff, 0xff,0xff,0x01,0xff, 0x66,0x25,0x99,0xcd,
   0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
   0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
   0x00,0x71,0x00,0x03, 0x00,0x03,0x00,0x03, 0x00,0x03,0x00,0x03, 0x00,0x03,0x00,0x03
	};
	int i, j;
	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 0x8000; j += 2)
		{
			mempack[i][j] = 0;
			mempack[i][j + 1] = 0x03;
		}
		memcpy(mempack[i], init, 272);
	}
}

unsigned char mempack_crc(unsigned char* data)
{
	int i;
	unsigned char CRC = 0;
	for (i = 0; i <= 0x20; i++)
	{
		int mask;
		for (mask = 0x80; mask >= 1; mask >>= 1)
		{
			int xor_tap = (CRC & 0x80) ? 0x85 : 0x00;
			CRC <<= 1;
			if (i != 0x20 && (data[i] & mask)) CRC |= 1;
			CRC ^= xor_tap;
		}
	}
	return CRC;
}

int frame_advancing = 0;

void internal_ReadController(int Control, BYTE* Command)
{
	switch (Command[2])
	{
	case 1:
		if (Controls[Control].Present)
		{
			BUTTONS Keys;
#ifdef VCR_SUPPORT
			VCR_getKeys(Control, &Keys);
#else
			getKeys(Control, &Keys);
#endif
			* ((unsigned long*)(Command + 3)) = Keys.Value;
#ifdef COMPARE_CORE
			check_input_sync(Command + 3);
#endif
		}
		break;
	case 2: // read controller pack
		if (Controls[Control].Present)
		{
			if (Controls[Control].Plugin == PLUGIN_RAW)
				if (controllerCommand) readController(Control, Command);
		}
		break;
	case 3: // write controller pack
		if (Controls[Control].Present)
		{
			if (Controls[Control].Plugin == PLUGIN_RAW)
				if (controllerCommand) readController(Control, Command);
		}
		break;
	}
}

void internal_ControllerCommand(int Control, BYTE* Command)
{
	switch (Command[2])
	{
	case 0x00: // check
	case 0xFF:
		if ((Command[1] & 0x80))
			break;
		if (Controls[Control].Present)
		{
			Command[3] = 0x05;
			Command[4] = 0x00;
			switch (Controls[Control].Plugin)
			{
			case PLUGIN_MEMPAK:
				Command[5] = 1;
				break;
			case PLUGIN_RAW:
				Command[5] = 1;
				break;
			default:
				Command[5] = 0;
				break;
			}
		}
		else
			Command[1] |= 0x80;
		break;
	case 0x01:
		if (!Controls[Control].Present)
			Command[1] |= 0x80;
		break;
	case 0x02: // read controller pack
		if (Controls[Control].Present)
		{
			switch (Controls[Control].Plugin)
			{
			case PLUGIN_MEMPAK:
			{
				int address = (Command[3] << 8) | Command[4];
				if (address == 0x8001)
				{
					memset(&Command[5], 0, 0x20);
					Command[0x25] = mempack_crc(&Command[5]);
				}
				else
				{
					address &= 0xFFE0;
					if (address <= 0x7FE0)
					{
						char* filename;
						FILE* f;
						filename = (char*)malloc(strlen(get_savespath()) +
							strlen(ROM_SETTINGS.goodname) + 4 + 1);
						strcpy(filename, get_savespath());
						strcat(filename, ROM_SETTINGS.goodname);
						strcat(filename, ".mpk");
						f = fopen(filename, "rb");
						if (f)
						{
							fread(mempack[0], 1, 0x8000, f);
							fread(mempack[1], 1, 0x8000, f);
							fread(mempack[2], 1, 0x8000, f);
							fread(mempack[3], 1, 0x8000, f);
							fclose(f);
						}
						else format_mempacks();
						free(filename);
						memcpy(&Command[5], &mempack[Control][address], 0x20);
					}
					else
					{
						memset(&Command[5], 0, 0x20);
					}
					Command[0x25] = mempack_crc(&Command[5]);
				}
			}
			break;
			case PLUGIN_RAW:
				if (controllerCommand) controllerCommand(Control, Command);
				break;
			default:
				memset(&Command[5], 0, 0x20);
				Command[0x25] = 0;
			}
		}
		else
			Command[1] |= 0x80;
		break;
	case 0x03: // write controller pack
		if (Controls[Control].Present)
		{
			switch (Controls[Control].Plugin)
			{
			case PLUGIN_MEMPAK:
			{
				int address = (Command[3] << 8) | Command[4];
				if (address == 0x8001)
					Command[0x25] = mempack_crc(&Command[5]);
				else
				{
					address &= 0xFFE0;
					if (address <= 0x7FE0)
					{
						char* filename;
						FILE* f;
						filename = (char*)malloc(strlen(get_savespath()) +
							strlen(ROM_SETTINGS.goodname) + 4 + 1);
						strcpy(filename, get_savespath());
						strcat(filename, ROM_SETTINGS.goodname);
						strcat(filename, ".mpk");
						f = fopen(filename, "rb");
						if (f)
						{
							fread(mempack[0], 1, 0x8000, f);
							fread(mempack[1], 1, 0x8000, f);
							fread(mempack[2], 1, 0x8000, f);
							fread(mempack[3], 1, 0x8000, f);
							fclose(f);
						}
						else format_mempacks();
						memcpy(&mempack[Control][address], &Command[5], 0x20);
						f = fopen(filename, "wb");
						fwrite(mempack[0], 1, 0x8000, f);
						fwrite(mempack[1], 1, 0x8000, f);
						fwrite(mempack[2], 1, 0x8000, f);
						fwrite(mempack[3], 1, 0x8000, f);
						fclose(f);
						free(filename);
					}
					Command[0x25] = mempack_crc(&Command[5]);
				}
			}
			break;
			case PLUGIN_RAW:
				if (controllerCommand) controllerCommand(Control, Command);
				break;
			default:
				Command[0x25] = mempack_crc(&Command[5]);
			}
		}
		else
			Command[1] |= 0x80;
		break;
	}
}

void update_pif_write()
{
	int i = 0, channel = 0;
/*#ifdef DEBUG_PIF
	if (input_delay) {
		printf("------------- write -------------\n");
	}
	else {
		printf("---------- before write ---------\n");
	}
	print_pif();
	printf("---------------------------------\n");
#endif*/
	if (PIF_RAMb[0x3F] > 1)
	{
		switch (PIF_RAMb[0x3F])
		{
		case 0x02:
			for (i = 0; i < sizeof(pif2_lut) / 32; i++)
			{
				if (!memcmp(PIF_RAMb + 64 - 2 * 8, pif2_lut[i][0], 16))
				{
					memcpy(PIF_RAMb + 64 - 2 * 8, pif2_lut[i][1], 16);
					return;
				}
			}
			printf("unknown pif2 code:\n");
			for (i = (64 - 2 * 8) / 8; i < (64 / 8); i++)
				printf("%x %x %x %x | %x %x %x %x\n",
					PIF_RAMb[i * 8 + 0], PIF_RAMb[i * 8 + 1], PIF_RAMb[i * 8 + 2], PIF_RAMb[i * 8 + 3],
					PIF_RAMb[i * 8 + 4], PIF_RAMb[i * 8 + 5], PIF_RAMb[i * 8 + 6], PIF_RAMb[i * 8 + 7]);
			break;
		case 0x08:
			PIF_RAMb[0x3F] = 0;
			break;
		default:
			printf("error in update_pif_write : %x\n", PIF_RAMb[0x3F]);
		}
		return;
	}
	while (i < 0x40)
	{
		switch (PIF_RAMb[i])
		{
		case 0x00:
			channel++;
			if (channel > 6) i = 0x40;
			break;
		case 0xFF:
			break;
		default:
			if (!(PIF_RAMb[i] & 0xC0))
			{
				if (channel < 4)
				{
					if (Controls[channel].Present &&
						Controls[channel].RawData)
						controllerCommand(channel, &PIF_RAMb[i]);
					else
						internal_ControllerCommand(channel, &PIF_RAMb[i]);
				}
				else if (channel == 4)
					EepromCommand(&PIF_RAMb[i]);
				else
					printf("channel >= 4 in update_pif_write\n");
				i += PIF_RAMb[i] + (PIF_RAMb[(i + 1)] & 0x3F) + 1;
				channel++;
			}
			else
				i = 0x40;
		}
		i++;
	}
	//PIF_RAMb[0x3F] = 0;
	controllerCommand(-1, NULL);
/*#ifdef DEBUG_PIF
	if (!one_frame_delay) {
		printf("---------- after write ----------\n");
	}
	print_pif();
	if (!one_frame_delay) {
		printf("---------------------------------\n");
	}
#endif*/
}

void sleep_while_emu_paused()
{
	extern int emu_paused;
	while (emu_paused)
	{
		#ifdef LUA_EMUPAUSED_WORK	
		Sleep(10);	
		AtIntervalLuaCallback();
		GetLuaMessage();
		if (LUA_double_buffered)
			LuaDCUpdate(1);
		#endif
	}
}

void update_pif_read(bool stcheck)
{
	//printf("pif entry\n");
	int i = 0, channel = 0;
	extern int emu_paused; //if you get error here, this means you're compiling for linux, and linux version 
						   //doesn't have pausing. But this also means linux version is unusable for tasing.
						   //If you really want it for some reason, define emu_paused.
	bool once = emu_paused | frame_advancing; //used to pause only once during controller routine
	bool stAllowed = true; //used to disallow .st being loaded after any controller has already been read
#ifdef DEBUG_PIF
	printf("---------- before read ----------\n");
	print_pif();
	printf("---------------------------------\n");
#endif
	while (i < 0x40)
	{
		switch (PIF_RAMb[i])
		{
		case 0x00:
			channel++;
			if (channel > 6) i = 0x40;
			break;
		case 0xFE:
			i = 0x40;
			break;
		case 0xFF:
			break;
		case 0xB4:
		case 0x56:
		case 0xB8:
			break;
		default:
			//01 04 01 is read controller 4 bytes
			if (!(PIF_RAMb[i] & 0xC0)) //mask error bits (isn't this wrong? error bits are on i+1???)
			{
				if (channel < 4)
				{
					static int controllerRead = 999;

					// frame advance - pause before every 'frame of input',
					// which is manually resumed to enter 1 input and emulate until being
					// paused here again before the next input
					if (once && channel <= controllerRead && (&PIF_RAMb[i])[2] == 1)
					{
						once = false;
						extern void pauseEmu(BOOL quiet);
						frame_advancing = 0;
						pauseEmu(TRUE);
						if (!input_delay)
						{
							while (emu_paused)
							{
#ifdef LUA_EMUPAUSED_WORK	
								Sleep(10);
								AtIntervalLuaCallback();
								GetLuaMessage();
								if(LUA_double_buffered)
								LuaDCUpdate(1);
#endif
								//should this be before or after? idk
								if (savestates_job & LOADSTATE && stAllowed)
								{
									savestates_load(false);
									savestates_job &= ~LOADSTATE;
								}
							}
						}
					}
					if (stcheck && !input_delay) {
						if (savestates_job & SAVESTATE && stAllowed)
						{
							savestates_save();
							savestates_job &= ~SAVESTATE;
						}
					}
					if (savestates_job & LOADSTATE && stAllowed) {
						savestates_load(false);
						savestates_job &= ~LOADSTATE;
					}
					extern bool old_st;
					if (old_st) { //if old savestate, don't fetch controller (matches old behaviour), makes delay fix not work for that st but syncs all m64s
						printf("old st detected\n");
						old_st = false;
						return;
					}
					stAllowed = false;
					controllerRead = channel;
					if (Controls[channel].Present &&
						Controls[channel].RawData
#ifdef VCR_SUPPORT
						&& VCR_isIdle()
#endif
						) {
						readController(channel, &PIF_RAMb[i]);
#ifdef LUA_JOYPAD
						lastInputLua[channel] = *(DWORD*)&PIF_RAMb[i + 3];
						AtInputLuaCallback(channel);
						if (0 <= channel && channel < 4) {
							if (rewriteInputFlagLua[channel]) {
								*(DWORD*)&PIF_RAMb[i + 3] =
									lastInputLua[channel] =
									rewriteInputLua[channel];
								rewriteInputFlagLua[channel] = false;
							}
						}
#endif

					}
					else
						internal_ReadController(channel, &PIF_RAMb[i]);
				}
				i += PIF_RAMb[i] + (PIF_RAMb[(i + 1)] & 0x3F) + 1;
				channel++;
			}
			else
				i = 0x40;
		}
		i++;
	}
	readController(-1, NULL);

#ifdef DEBUG_PIF
	printf("---------- after read -----------\n");
	print_pif();
	printf("---------------------------------\n");
#endif
	//printf("pif exit\n");
}

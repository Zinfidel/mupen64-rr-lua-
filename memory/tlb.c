/**
 * Mupen64 - tlb.c
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

#include "tlb.h"
#include "memory.h"
#include "../r4300/r4300.h"
#include "../r4300/exception.h"
#include "../r4300/macros.h"

unsigned long tlb_LUT_r[0x100000];
unsigned long tlb_LUT_w[0x100000];
extern unsigned long interp_addr;
unsigned long virtual_to_physical_address(unsigned long addresse, int w)
{
   if (addresse >= 0x7f000000 && addresse < 0x80000000) // golden eye hack (it uses TLB a lot)
     {
	if (ROM_HEADER->CRC1 == sl(0xDCBC50D1)) // US
	  return 0xb0034b30 + (addresse & 0xFFFFFF);
	if (ROM_HEADER->CRC1 == sl(0x0414CA61)) // E
	  return 0xb00329f0 + (addresse & 0xFFFFFF);
	if (ROM_HEADER->CRC1 == sl(0xA24F4CF1)) // J
	  return 0xb0034b70 + (addresse & 0xFFFFFF);
     }
   if (w == 1)
     {
	if (tlb_LUT_w[addresse>>12])
	  return (tlb_LUT_w[addresse>>12]&0xFFFFF000)|(addresse&0xFFF);
     }
   else
     {
	if (tlb_LUT_r[addresse>>12])
	  return (tlb_LUT_r[addresse>>12]&0xFFFFF000)|(addresse&0xFFF);
     }
   //printf("tlb exception !!! @ %x, %x, add:%x\n", addresse, w, interp_addr);
   //getchar();
   TLB_refill_exception(addresse,w);
   //return 0x80000000;
   return 0x00000000;
   /*int i;
   for (i=0; i<32; i++)
     {
	if ((tlb_e[i].vpn2 & ~(tlb_e[i].mask))
	    == ((addresse >> 13) & ~(tlb_e[i].mask)))
	  {
	     if (tlb_e[i].g || (tlb_e[i].asid == (EntryHi & 0xFF)))
	       {
		  if (addresse & tlb_e[i].check_parity_mask)
		    {
		       if (tlb_e[i].v_odd)
			 {
			    if (tlb_e[i].d_odd && w)
			      {
				 TLB_mod_exception();
				 return 0;
			      }
			    return ((addresse & ((tlb_e[i].mask << 12)|0xFFF))
				    | ((tlb_e[i].pfn_odd << 12) &
				       ~((tlb_e[i].mask << 12)|0xFFF)) 
				    | 0x80000000);
			 }
		       else
			 {
			    TLB_invalid_exception();
			    return 0;
			 }
		    }
		  else
		    {
		       if (tlb_e[i].v_even)
			 {
			    if (tlb_e[i].d_even && w)
			      {
				 TLB_mod_exception();
				 return 0;
			      }
			    return ((addresse & ((tlb_e[i].mask << 12)|0xFFF))
				    | ((tlb_e[i].pfn_even << 12) &
				       ~((tlb_e[i].mask << 12)|0xFFF))
				    | 0x80000000);
			 }
		       else
			 {
			    TLB_invalid_exception();
			    return 0;
			 }
		    }
	       }
	     else
	       {
		  printf("tlb refill inconnu\n");
		  TLB_refill_exception(addresse,w);
	       }
	  }
     }
   BadVAddr = addresse;
   TLB_refill_exception(addresse,w);
   //printf("TLB refill exception\n");
   return 0x80000000;*/
}

int probe_nop(unsigned long address)
{
   unsigned long a;
   if (address < 0x80000000 || address > 0xc0000000)
     {
	if (tlb_LUT_r[address>>12])
	  a = (tlb_LUT_r[address>>12]&0xFFFFF000)|(address&0xFFF);
	else
	  return 0;
     }
   else
     a = address;
   
   if (a >= 0xa4000000 && a < 0xa4001000)
     {
	if (!SP_DMEM[(a&0xFFF)/4]) return 1;
	else return 0;
     }
   else if (a >= 0x80000000 && a < 0x80800000)
     {
	if (!rdram[(a&0x7FFFFF)/4]) return 1;
	else return 0;
     }
   else return 0;
}

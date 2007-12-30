/* ES40 emulator.
 * Copyright (C) 2007 by the ES40 Emulator Project
 *
 * Website: http://sourceforge.net/projects/es40
 * E-mail : camiel@camicom.com
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * 
 * Although this is not required, the author would appreciate being notified of, 
 * and receiving any modifications you may make to the source code that might serve
 * the general public.
 */

/**
 * \file 
 * Contains the code for the emulated Typhoon Chipset devices.
 *
 * X-1.45       Camiel Vanderhoeven                             30-DEC-2007
 *      Print file id on initialization.
 *
 * X-1.44       Camiel Vanderhoeven                             29-DEC-2007
 *      Fix memory-leak.
 *
 * X-1.43       Camiel Vanderhoeven                             28-DEC-2007
 *      Throw exceptions rather than just exiting when errors occur.
 *
 * X-1.42       Camiel Vanderhoeven                             28-DEC-2007
 *      Keep the compiler happy.
 *
 * X-1.41       Camiel Vanderhoeven                             20-DEC-2007
 *      Close files and free memory when the emulator shuts down.
 *
 * X-1.40       Camiel Vanderhoeven                             17-DEC-2007
 *      SaveState file format 2.1
 *
 * X-1.39       Camiel Vanderhoeven                             14-DEC-2007
 *      Commented out SRM IDE READ replacement; doesn't work with SCSI!
 *
 * X-1.38       Camiel Vanderhoeven                             10-DEC-2007
 *      Added get_cpu
 *
 * X-1.37       Camiel Vanderhoeven                             10-DEC-2007
 *      Use configurator.
 *
 * X-1.36       Camiel Vanderhoeven                             6-DEC-2007
 *      Report references to unused PCI space.
 *
 * X-1.35       Camiel Vanderhoeven                             2-DEC-2007
 *      Avoid misprobing of unused PCI configuration space.
 *
 * X-1.34       Camiel Vanderhoeven                             2-DEC-2007
 *      Added support for code profiling, and for direct operations on the
 *      Tsunami/Typhoon's interrupt registers.
 *
 * X-1.33       Brian Wheeler                                   1-DEC-2007
 *   1. Ignore address bits 35- 42 in the physical address; this is
 *      correct according to the Tsunami/Typhoon HRM; which states that
 *       "  The system address space is divided into two parts: system 
 *        memory and PIO. This division is indicated by physical memory bit
 *        <43> = 1 for PIO accesses from the CPU [...] In general, bits 
 *        <42:35> are don�t cares if bit <43> is asserted. [...] The 
 *        Typhoon Cchip supports 32GB of system memory (35 bits total).  "
 *   2. Added support for Ctrl+C and panic.
 *
 * X-1.32       Camiel Vanderhoeven                             17-NOV-2007
 *      Use CHECK_ALLOCATION.
 *
 * X-1.31       Camiel Vanderhoeven                             16-NOV-2007
 *      Replaced PCI_ReadMem and PCI_WriteMem with PCI_Phys.
 *
 * X-1.30       Camiel Vanderhoeven                             05-NOV-2007
 *      Put slow-to-fast clock ratio into #define CLOCK_RATIO. Increased 
 *      this to 100,000.
 *
 * X-1.29       Camiel Vanderhoeven                             18-APR-2007
 *      Decompressed ROM image is now identical between big- and small-
 *      endian platforms (put endian_64 around PALbase and PC).
 *
 * X-1.28       Camiel Vanderhoeven                             18-APR-2007
 *      Faster lockstep mechanism (send info 50 cpu cycles at a time)
 *
 * X-1.27       Camiel Vanderhoeven                             16-APR-2007
 *      Remove old address range if a new one is registered (same device/
 *      same index)
 *
 * X-1.26       Camiel Vanderhoeven                             16-APR-2007
 *      Allow configuration strings with spaces in them.
 *
 *
 * X-1.25       Camiel Vanderhoeven                             11-APR-2007
 *      Moved all data that should be saved to a state file to a structure
 *      "state".
 *
 * X-1.24	Camiel Vanderhoeven				10-APR-2007
 *	New mechanism for SRM replacements. Where these need to be executed,
 *	CSystem::LoadROM() puts a special opcode (a CALL_PAL instruction
 *	with an otherwise illegal operand of 0x01234xx) in memory. 
 *	CAlphaCPU::DoClock() recognizes these opcodes and performs the SRM
 *	action.
 *
 * X-1.23       Camiel Vanderhoeven                             10-APR-2007
 *      Extended ROM-handling code to favor loading decompressed ROM code
 *      over loading compressed code, and to save decompressed ROM code
 *      during the first time the emulator is run.
 *
 * X-1.22       Camiel Vanderhoeven                             10-APR-2007
 *      Removed obsolete ROM-handling code.
 *
 * X-1.21       Brian Wheeler                                   31-MAR-2007
 *      Removed ; after #endif to avoid compiler warnings.
 *
 * X-1.20       Camiel Vanderhoeven                             26-MAR-2007
 *      Show references to unknown memory regions when DEBUG_UNKMEM is 
 *	defined.
 *
 * X-1.19	Camiel Vanderhoeven				1-MAR-2007
 *	Changes for Solaris/SPARC port:
 *   a)	All $-signs in variable names are replaced with underscores.
 *   b) Some functions now get a const char * argument i.s.o. char * to avoid
 *	compiler warnings.
 *   c) If ALIGN_MEM_ACCESS is defined, memory accesses are checked for natural
 *	alignment. If access is not naturally aligned, it is performed one byte
 *	at a time.
 *   d) Accesses to main-memory are byte-swapped on a big-endian architecture.
 *	This is done through the endian_xx macro's, that differ according to
 *	the endianness of the architecture.
 *
 * X-1.18	Camiel Vanderhoeven				28-FEB-2007
 *	In the lockstep-versions of the emulator, perform lockstep 
 *	synchronisation for every clock tick.
 *
 * X-1.17	Camiel Vanderhoeven				27-FEB-2007
 *	Removed an unreachable "return 0;" line.
 *
 * X-1.16	Camiel Vanderhoeven				18-FEB-2007
 *	Keep track of the cycle-counter in single-step mode (using the
 *	iSSCycles variable.
 *
 * X-1.15	Camiel Vanderhoeven				16-FEB-2007
 *   a) Provide slow and fast clocks for devices. Typical fast-clocked 
 *	devices are the CPU(s); most other devices that need a clock should 
 *	probably be slow clock devices.
 *   b) DoClock() was replaced with Run(), which runs until one of the 
 *	connected devices returns something other than 0; and SingleStep().
 *   c) Corrected some signed/unsigned integer comparison warnings.
 *
 * X-1.14	Brian Wheeler					13-FEB-2007
 *   a) Corrected some typo's in printf statements.
 *   b) Fixed some compiler warnings (assignment inside if()).
 *
 * X-1.13	Camiel Vanderhoeven				12-FEB-2007
 *	Removed error messages when accessing unknown memory.
 *
 * X-1.12       Camiel Vanderhoeven                             12-FEB-2007
 *      Corrected a signed/unsigned integer comparison warning.
 *
 * X-1.11       Camiel Vanderhoeven                             9-FEB-2007
 *      Added comments.
 *
 * X-1.10	Brian Wheeler					7-FEB-2007
 *	Remove FindConfig function, and load configuration file from the
 *	constructor.
 *
 * X-1.9	Camiel Vanderhoeven				7-FEB-2007
 *   a)	CTraceEngine is no longer instantiated as a member of CSystem.
 *   b)	Calls to trace_dev now use the TRC_DEVx macro's.
 *
 * X-1.8	Camiel Vanderhoeven				3-FEB-2007
 *   a) Removed last conditional for supporting another system than an ES40
 *      (#ifdef DS15)
 *   b) FindConfig() now returns the default value rather than crashing 
 *	when none of the standard configuration files can be found.
 *
 * X-1.7        Brian Wheeler                                   3-FEB-2007
 *      Formatting.
 *
 * X-1.6	Brian Wheeler					3-FEB-2007
 *	Replaced several 64-bit values in 0x... syntax with X64(...).
 *
 * X-1.5	Brian Wheeler					3-FEB-2007
 *	Added possibility to load a configuration file.
 *
 * X-1.4	Brian Wheeler					3-FEB-2007
 *	Replaced 1i64 with X64(1) in two instances.
 *
 * X-1.3        Brian Wheeler                                   3-FEB-2007
 *      Scanf and printf statements made compatible with Linux/GCC/glibc.
 *      
 * X-1.2        Brian Wheeler                                   3-FEB-2007
 *      Includes are now case-correct (necessary on Linux)
 *
 * X-1.1        Camiel Vanderhoeven                             19-JAN-2007
 *      Initial version in CVS.
 *
 * \author Camiel Vanderhoeven (camiel@camicom.com / http://www.camicom.com)
 **/

#include "StdAfx.h"
#include "System.h"
#include "AlphaCPU.h"
#include "lockstep.h"

#include <ctype.h>
#include <stdlib.h>
#include <signal.h>


#define CLOCK_RATIO 10000

#if defined(LS_MASTER) || defined(LS_SLAVE)
char debug_string[10000] = "";
char * dbg_strptr = debug_string;
#endif
 
/**
 * Constructor.
 **/

CSystem::CSystem(CConfigurator * cfg)
{
  if (theSystem != 0)
    FAILURE("More than one system!!");
  theSystem = this;
  myCfg = cfg;

  iNumComponents = 0;
  iNumFastClocks = 0;
  iNumSlowClocks = 0;
  iNumMemories = 0;
  iNumCPUs = 0;
  iNumMemoryBits = myCfg->get_int_value("memory.bits",27);
//  iNumConfig = 0;

#if defined(IDB)
  iSingleStep = 0;
  iSSCycles = 0;
#endif

  state.c_MISC = X64(0000000800000000);
  state.c_DIM[0] = 0;
  state.c_DIM[1] = 0;
  state.c_DIM[2] = 0;
  state.c_DIM[3] = 0;
  state.c_DRIR = 0;
  state.c_CSC = X64(0042444014157803);
  state.c_TRR = X64(0000000000003103);
  state.c_TDR = X64(F37FF37FF37FF37F);

  state.d_STR = X64(2525252525252525);

  state.p_PCTL[0] = X64(0000104401440081);
  state.p_PCTL[1] = X64(0000504401440081);

  state.p_PERRMASK[0] = 0;
  state.p_PERRMASK[1] = 0;
  state.p_PLAT[0] = 0;
  state.p_PLAT[1] = 0;
  state.p_TBA [0][0] = 0;
  state.p_WSBA[0][0] = 0;
  state.p_WSM [0][0] = 0;
  state.p_TBA [0][1] = 0;
  state.p_WSBA[0][1] = 0;
  state.p_WSM [0][1] = 0;
  state.p_TBA [0][2] = 0;
  state.p_WSBA[0][2] = 0;
  state.p_WSM [0][2] = 0;
  state.p_TBA [0][3] = 0;
  state.p_WSBA[0][3] = 2;
  state.p_WSM [0][3] = 0;
  state.p_TBA [1][0] = 0;
  state.p_WSBA[1][0] = 0;
  state.p_WSM [1][0] = 0;
  state.p_TBA [1][1] = 0;
  state.p_WSBA[1][1] = 0;
  state.p_WSM [1][1] = 0;
  state.p_TBA [1][2] = 0;
  state.p_WSBA[1][2] = 0;
  state.p_WSM [1][2] = 0;
  state.p_TBA [1][3] = 0;
  state.p_WSBA[1][3] = 2;
  state.p_WSM [1][3] = 0;

  state.tig_FwWrite = 0;
  state.tig_HaltA   = 0;
  state.tig_HaltB   = 0;

  CHECK_ALLOCATION(memory = calloc(1<<iNumMemoryBits,1));

  printf("%s: $Id: System.cpp,v 1.45 2007/12/30 15:12:55 iamcamiel Exp $\n");
}

/**
 * Destructor.
 **/

CSystem::~CSystem()
{
  int i;

  printf("Freeing memory in use by system...\n");

  for (i=0;i<iNumComponents;i++)
    delete acComponents[i];

  for (i=0;i<iNumMemories;i++)
    free(asMemories[i]);

  free(memory);

}

void CSystem::ResetMem(unsigned int membits) {
  free(memory);
  iNumMemoryBits=membits;
  CHECK_ALLOCATION(memory = calloc(1<<iNumMemoryBits,1));
}

int CSystem::RegisterComponent(CSystemComponent *component)
{
  acComponents[iNumComponents] = component;
  iNumComponents++;
  return 0;
}

unsigned int CSystem::get_memory_bits()
{
  return iNumMemoryBits;
}

char * CSystem::PtrToMem(u64 address)
{
  return &(((char*)memory)[(int)address]);
}

int CSystem::RegisterCPU(class CAlphaCPU * cpu)
{
  if (iNumCPUs>=4)
    return -1;
  acCPUs[iNumCPUs] = cpu;
  iNumCPUs++;
  return iNumCPUs - 1;
}

int CSystem::RegisterClock(CSystemComponent *component, bool slow)
{
  if (slow)
  {
    acSlowClocks[iNumSlowClocks] = component;
    iNumSlowClocks++;
  }
  else
  {
    acFastClocks[iNumFastClocks] = component;
    iNumFastClocks++;
  }
  return 0;
}

int CSystem::RegisterMemory(CSystemComponent *component, int index, u64 base, u64 length)
{
  struct SMemoryUser * m;
  int i;
  for (i=0;i<iNumMemories;i++)
  {
    if ((asMemories[i]->component == component) && (asMemories[i]->index == index))
    {
      asMemories[i]->base = base;
      asMemories[i]->length = length;
      return 0;
    }
  }

  CHECK_ALLOCATION(m = (struct SMemoryUser*) malloc(sizeof(struct SMemoryUser)));
  m->component = component;
  m->base = base;
  m->length = length;
  m->index = index;

  asMemories[iNumMemories] = m;
  iNumMemories++;
  return 0;
}

int got_sigint = 0;
void sigint_handler (int signum) 
{
  got_sigint=1;
}

int CSystem::Run()
{
  int i,j,k;
  int result;

  /* catch CTRL-C and shutdown gracefully */
  signal(SIGINT,&sigint_handler);

  for(k=0;!got_sigint;k++)
  {
    for(j=0;j<CLOCK_RATIO;j++)
    {
      for(i=0;i<iNumFastClocks;i++)
      {
        if (result = acFastClocks[i]->DoClock())
	     return result;
      }
    }
	 
    for(i=0;i<iNumSlowClocks;i++)
    {
      if (result = acSlowClocks[i]->DoClock())
    	return result;
    }
#if !defined(HIDE_COUNTER)
#if defined(PROFILE)
    printf("%d | %016" LL "x | %" LL "d profiled instructions.  \r",k,acCPUs[0]->get_pc(),profiled_insts);
#else
    printf("%d | %016" LL "x\r",k,acCPUs[0]->get_pc());
#endif
#endif
  }

  printf("%%SYS-W-SHUTDOWN: CTRL-C or Device Failed\n");
  return 1;
}

int CSystem::SingleStep()
{
  int i;
  int result;

//#if defined(LS_MASTER) || defined(LS_SLAVE)
//  lockstep_sync_m2s("sync1");
//  lockstep_sync_s2m("sync2");
//#endif

  for(i=0;i<iNumFastClocks;i++)
  {
    result = acFastClocks[i]->DoClock();
    if (result)
      return result;
  }

  iSingleStep++;

#if defined(LS_MASTER) || defined(LS_SLAVE)
  if (!(iSingleStep % 50))
  {
     lockstep_sync_m2s("sync1");
     *dbg_strptr='\0';
     lockstep_compare(debug_string);
     dbg_strptr = debug_string;
     *dbg_strptr='\0';
  }
#endif
  if (iSingleStep >= CLOCK_RATIO)
  {
     iSingleStep = 0;
     for(i=0;i<iNumSlowClocks;i++)
     {
        result = acSlowClocks[i]->DoClock();
	if (result)
	  return result;
     }
#ifdef IDB
     iSSCycles++;
#if !defined(LS_SLAVE)
     if (bHashing)
#endif
       printf("%d | %016" LL "x\r",iSSCycles,acCPUs[0]->get_pc());
#endif
  }
  return 0;
}


#ifdef DEBUG_PORTACCESS
u64 lastport;
#endif


void CSystem::WriteMem(u64 address, int dsize, u64 data)
{
  u64 a;
  int i;
  u8 * p;
#if defined(ALIGN_MEM_ACCESS)
  u64 t64;
  u32 t32;
  u16 t16;
#endif

  a = address & X64(00000807ffffffff);

  if (a>>iNumMemoryBits)
    {

      // non-memory...
      for(i=0;i<iNumMemories;i++) {
	if (   (a >= asMemories[i]->base)
	       && (a < asMemories[i]->base + asMemories[i]->length) ) {
	  asMemories[i]->component->WriteMem(asMemories[i]->index, a-asMemories[i]->base, dsize, data);
	  return;
	}
      }

      switch (a)
	{
        case X64(00000801a0000000): // CSC
	  state.c_CSC        &= ~X64(0777777fff3f0000);
	  state.c_CSC |= (data & X64(0777777fff3f0000));
	  return;
	case X64(00000801a0000080): // MISC
	  state.c_MISC |= (data & X64(00000f0000f0f000));	// W1S
	  state.c_MISC &=~(data & X64(0000000010000ff0));	// W1C
	  if       (data & X64(0000000001000000))
	    state.c_MISC &=~        X64(0000000000ff0000);	//Arbitration Clear
	  if    (!(state.c_MISC & X64(00000000000f0000)))
	    state.c_MISC |= (data & X64(00000000000f0000));	//Arbitration try

	  // stop interval timer interrupt
	  if        (data & X64(00000000000000f0))
	    {
	      for (i=0;i<iNumCPUs;i++)
		{
		  if (data & (X64(10)<<i))
                    {
		      acCPUs[i]->irq_h(2,false);
		      //                        printf("*** TIMER interrupt cleared for CPU %d\n",i);
                    }
		}
	    }			
	  return;

	case X64(00000801a0000200):
	  state.c_DIM[0] = data;
	  return;
	case X64(00000801a0000240):
	  state.c_DIM[1] = data;
	  return;
	case X64(00000801a0000600):
	  state.c_DIM[2] = data;
	  return;
	case X64(00000801a0000640):
	  state.c_DIM[3] = data;
	  return;

        case X64(0000080180000000):
	  state.p_WSBA[0][0] = data & X64(00000000fff00003);
	  return;
        case X64(0000080180000040):
	  state.p_WSBA[0][1] = data & X64(00000000fff00003);
	  return;
        case X64(0000080180000080):
	  state.p_WSBA[0][2] = data & X64(00000000fff00003);
	  return;
        case X64(00000801800000c0):
	  state.p_WSBA[0][3] = data & X64(00000080fff00001) | 2;
	  return;
        case X64(0000080380000000):
	  state.p_WSBA[1][0] = data & X64(00000000fff00003);
	  return;
        case X64(0000080380000040):
	  state.p_WSBA[1][1] = data & X64(00000000fff00003);
	  return;
        case X64(0000080380000080):
	  state.p_WSBA[1][2] = data & X64(00000000fff00003);
	  return;
        case X64(00000803800000c0):
	  state.p_WSBA[1][3] = data & X64(00000080fff00001) | 2;
	  return;
        case X64(0000080180000100):
	  state.p_WSM[0][0] = data & X64(00000000fff00000);
	  return;
        case X64(0000080180000140):
	  state.p_WSM[0][1] = data & X64(00000000fff00000);
	  return;
        case X64(0000080180000180):
	  state.p_WSM[0][2] = data & X64(00000000fff00000);
	  return;
        case X64(00000801800001c0):
	  state.p_WSM[0][3] = data & X64(00000000fff00000);
	  return;
        case X64(0000080380000100):
	  state.p_WSM[1][0] = data & X64(00000000fff00000);
	  return;
        case X64(0000080380000140):
	  state.p_WSM[1][1] = data & X64(00000000fff00000);
	  return;
        case X64(0000080380000180):
	  state.p_WSM[1][2] = data & X64(00000000fff00000);
	  return;
        case X64(00000803800001c0):
	  state.p_WSM[1][3] = data & X64(00000000fff00000);
	  return;
        case X64(0000080180000200):
	  state.p_TBA[0][0] = data & X64(00000007fffffc00);
	  return;
        case X64(0000080180000240):
	  state.p_TBA[0][1] = data & X64(00000007fffffc00);
	  return;
        case X64(0000080180000280):
	  state.p_TBA[0][2] = data & X64(00000007fffffc00);
	  return;
        case X64(00000801800002c0):
	  state.p_TBA[0][3] = data & X64(00000007fffffc00);
	  return;
        case X64(0000080380000200):
	  state.p_TBA[1][0] = data & X64(00000007fffffc00);
	  return;
        case X64(0000080380000240):
	  state.p_TBA[1][1] = data & X64(00000007fffffc00);
	  return;
        case X64(0000080380000280):
	  state.p_TBA[1][2] = data & X64(00000007fffffc00);
	  return;
        case X64(00000803800002c0):
	  state.p_TBA[1][3] = data & X64(00000007fffffc00);
	  return;
	case X64(0000080180000300):
	  state.p_PCTL[0] &=         X64(ffffe300f0300000);
	  state.p_PCTL[0] |= (data & X64(00001cff0fcfffff));
	  return;
	case X64(0000080380000300):
	  state.p_PCTL[1] &=         X64(ffffe300f0300000);
	  state.p_PCTL[1] |= (data & X64(00001cff0fcfffff));
	  return;
	case X64(0000080180000340):
	  state.p_PLAT[0] = data;
	  return;
	case X64(0000080380000340):
	  state.p_PLAT[1] = data;
	  return;		
	case X64(0000080180000400):
	  state.p_PERRMASK[0] = data;
	  return;
	case X64(0000080380000400):
	  state.p_PERRMASK[1] = data;
	  return;
	case X64(00000801300003c0):
	  state.tig_HaltA = (u8)(data&0xff);
	  return;
	case X64(00000801300005c0):
	  state.tig_HaltB = (u8)(data&0xff);
	  return;
	case X64(0000080130000040):
	  state.tig_FwWrite = (u8)(data&0xff);
	  return;
	case X64(0000080130000100):
	  // soft reset
	  printf("Soft reset: %02x\n",(int)data);
	  return;
	  //PERROR registers
	case X64(00000801800003c0):
	case X64(00000803800003c0):
	  //TLBIA
	case X64(00000801800004c0):
	case X64(00000803800004c0):
          return;
	  // PCI reset
	case X64(0000080180000800):
	case X64(0000080380000800):
          for(i=0;i<iNumComponents;i++)
            acComponents[i]->ResetPCI();
	  return;
	default:
      if (a>=X64(801fc000000) && a<X64(801fe000000))
      {
        // Unused PCI I/O space
//        printf("Write to unknown IO port %"LL"x on PCI 0   \n",a & X64(1ffffff));
        return;
      }

      if (a>=X64(803fc000000) && a<X64(803fe000000))
      {
        // Unused PCI I/O space
        printf("Write to unknown IO port %"LL"x on PCI 1   \n",a & X64(1ffffff));
        return;
      }

      if (a>=X64(80000000000) && a<X64(80100000000))
      {
        // Unused PCI memory space
        printf("Write to unknown memory %"LL"x on PCI 0   \n",a & X64(ffffffff));
        return;
      }

      if (a>=X64(80200000000) && a<X64(80300000000))
      {
        // Unused PCI memory space
        printf("Write to unknown memory %"LL"x on PCI 1   \n",a & X64(ffffffff));
        return;
      }

#ifdef DEBUG_UNKMEM
	printf("%%MEM-I-WRUNKNWN: Attempt to write %d bytes (%016" LL "x) from unknown address %011" LL "x\n",dsize/8,data,a);
#endif
	  return;
	}
    }


  p = (u8*)memory + a;


  switch(dsize)
    {
    case 8:
      *((u8 *) p) = (u8) data;
      break;
    case 16:
#if defined(ALIGN_MEM_ACCESS)
      if (address&1)
      {
        t64 = endian_64( (u64) data );
        *p     = (t64 >> 8) & 0xff;
        *(p+1) =  t64       & 0xff;
      }  
      else
#endif
      *((u16 *) p) = endian_16( (u16) data );
      break;
    case 32:
#if defined(ALIGN_MEM_ACCESS)
      if (address&3)
      {
        t32 = endian_32( (u32) data );
        *p     = (t32 >> 24) & 0xff;
        *(p+1) = (t32 >> 16) & 0xff;
        *(p+2) = (t32 >>  8) & 0xff;
        *(p+3) =  t32        & 0xff;
      }  
      else
#endif
        *((u32 *) p) = endian_32( (u32) data );
      break;
    default:
#if defined(ALIGN_MEM_ACCESS)
      if (address&7)
      {
        t64 = endian_64( (u64) data );
        *p     = (t64 >> 56) & 0xff;
        *(p+1) = (t64 >> 48) & 0xff;
        *(p+2) = (t64 >> 40) & 0xff;
        *(p+3) = (t64 >> 32) & 0xff;
        *(p+4) = (t64 >> 24) & 0xff;
        *(p+5) = (t64 >> 16) & 0xff;
        *(p+6) = (t64 >>  8) & 0xff;
        *(p+7) =  t64        & 0xff;
      }  
      else
#endif
        *((u64 *) p) = endian_64( (u64) data );
    }
}

u64 CSystem::ReadMem(u64 address, int dsize)
{
  u64 a;
  int i;
  u8 * p;

  a = address & X64(00000807ffffffff);
  if (a>>iNumMemoryBits)
    {
      // Non memory
      for(i=0;i<iNumMemories;i++) {
	if (   (a >= asMemories[i]->base)
	       && (a < asMemories[i]->base + asMemories[i]->length) ) {
	  return asMemories[i]->component->ReadMem(asMemories[i]->index, a-asMemories[i]->base, dsize);
	}
      }

      switch (a)
	{

	  // PError registers
	case X64(00000801800003c0):
	case X64(00000803800003c0):

        case X64(00000801a0000000):
	  return state.c_CSC;
	case X64(00000801a0000080):
	  return state.c_MISC;
	case X64(00000801a0000100):
	  return   ((u64)(iNumMemoryBits-23)<<12); //size
	case X64(00000801a0000140):
	case X64(00000801a0000180):
	case X64(00000801a00001c0):
	  return 0;
	  // WE PUT ALL OUR MEMORY IN A SINGLE ARRAY FOR NOW...
	  // memory arrays
	  //			return   0x300				// twice-split
	  //				   | ((iNumMemoryBits-25)<<12) //size
	  //				   | ( (((a>>6)&3)<<(iNumMemoryBits-2)) & 0x7ff000000);	// address

	case X64(00000801a0000200):
	  return state.c_DIM[0];
	case X64(00000801a0000240):
	  return state.c_DIM[1];
	case X64(00000801a0000600):
	  return state.c_DIM[2];
	case X64(00000801a0000640):
	  return state.c_DIM[3];
        case X64(00000801a0000280):
	  return state.c_DRIR & state.c_DIM[0];
        case X64(00000801a00002c0):
	  return state.c_DRIR & state.c_DIM[1];
        case X64(00000801a0000680):
	  return state.c_DRIR & state.c_DIM[2];
        case X64(00000801a00006c0):
	  return state.c_DRIR & state.c_DIM[3];
        case X64(00000801a0000300):
	  return state.c_DRIR;

	case X64(0000080180000300):
	  return state.p_PCTL[0];
	case X64(0000080380000300):
	  return state.p_PCTL[1];
	case X64(0000080180000400):
	  return state.p_PERRMASK[0];
	case X64(0000080380000400):
	  return state.p_PERRMASK[1];

        case X64(0000080180000000):
	  return state.p_WSBA[0][0];
        case X64(0000080180000040):
	  return state.p_WSBA[0][1];
        case X64(0000080180000080):
	  return state.p_WSBA[0][2];
        case X64(00000801800000c0):
	  return state.p_WSBA[0][3];
        case X64(0000080380000000):
	  return state.p_WSBA[1][0];
        case X64(0000080380000040):
	  return state.p_WSBA[1][1];
        case X64(0000080380000080):
	  return state.p_WSBA[1][2];
        case X64(00000803800000c0):
	  return state.p_WSBA[1][3];
        case X64(0000080180000100):
	  return state.p_WSM[0][0];
        case X64(0000080180000140):
	  return state.p_WSM[0][1];
        case X64(0000080180000180):
	  return state.p_WSM[0][2];
        case X64(00000801800001c0):
	  return state.p_WSM[0][3];
        case X64(0000080380000100):
	  return state.p_WSM[1][0];
        case X64(0000080380000140):
	  return state.p_WSM[1][1];
        case X64(0000080380000180):
	  return state.p_WSM[1][2];
        case X64(00000803800001c0):
	  return state.p_WSM[1][3];
        case X64(0000080180000200):
	  return state.p_TBA[0][0];
        case X64(0000080180000240):
	  return state.p_TBA[0][1];
        case X64(0000080180000280):
	  return state.p_TBA[0][2];
        case X64(00000801800002c0):
	  return state.p_TBA[0][3];
        case X64(0000080380000200):
	  return state.p_TBA[1][0];
        case X64(0000080380000240):
	  return state.p_TBA[1][1];
        case X64(0000080380000280):
	  return state.p_TBA[1][2];
        case X64(00000803800002c0):
	  return state.p_TBA[1][3];

	case X64(00000801b0000880):
	  // DCHIP revisions
	  return X64(0101010101010101);

	case X64(0000080138000180):
	  // Arbiter revision
	  return 0xfe;
	case X64(0000080130000040):
	  return state.tig_FwWrite;
	case X64(00000801300003c0):
	  return state.tig_HaltA;
	case X64(00000801300005c0):
	  return state.tig_HaltB;

	default:
      if (   (a>=X64(801fe000000) && a<X64(801ff000000))
          || (a>=X64(803fe000000) && a<X64(803ff000000)) )
      {
        // Unused PCI configuration space
        return make_mask_64(dsize-1,0);
      }

      if (a>=X64(800000c0000) && a<X64(801000e0000))
      {
        // Unused PCI ROM BIOS space
        return 0;
      }

      if (a>=X64(801fc000000) && a<X64(801fe000000))
      {
        // Unused PCI I/O space
   //     printf("Read from unknown IO port %"LL"x on PCI 0   \n",a & X64(1ffffff));
        return 0;
      }

      if (a>=X64(803fc000000) && a<X64(803fe000000))
      {
        // Unused PCI I/O space
        printf("Read from unknown IO port %"LL"x on PCI 1   \n",a & X64(1ffffff));
        return 0;
      }

      if (a>=X64(80000000000) && a<X64(80100000000))
      {
        // Unused PCI memory space
        printf("Read from unknown memory %"LL"x on PCI 0   \n",a & X64(ffffffff));
        return 0;
      }

      if (a>=X64(80200000000) && a<X64(80300000000))
      {
        // Unused PCI memory space
        printf("Read from unknown memory %"LL"x on PCI 1   \n",a & X64(ffffffff));
        return 0;
      }

#ifdef DEBUG_UNKMEM
	printf("%%MEM-I-RDUNKNWN: Attempt to read %d bytes from unknown address %011" LL "x\n",dsize/8,a);
#endif
	return 0x00;

	  //			return 0x77; // 7f
	}
    }

  p = (u8*)memory + a;

  switch(dsize)
    {
    case 8:
      return *((u8 *) p);
    case 16:
#if defined(ALIGN_MEM_ACCESS)
      if (address&1)
        return endian_16( *p | ( *(p+1) << 8 ) );
      else
#endif
        return endian_16( *((u16 *) p) );
    case 32:
#if defined(ALIGN_MEM_ACCESS)
      if (address&3)
        return endian_32( *p | 
                          ( *(p+1) << 8 )  | 
                          ( *(p+2) << 16 ) | 
                          ( *(p+3) << 24 ) );
      else
#endif
        return endian_32( *((u32 *) p) );
    default:
#if defined(ALIGN_MEM_ACCESS)
      if (address&7)
        return endian_64(   (u64)(*p) | 
                          ( (u64)(*(p+1)) <<  8 ) | 
                          ( (u64)(*(p+2)) << 16 ) | 
                          ( (u64)(*(p+3)) << 24 ) |
                          ( (u64)(*(p+4)) << 32 ) | 
                          ( (u64)(*(p+5)) << 40 ) | 
                          ( (u64)(*(p+6)) << 48 ) | 
                          ( (u64)(*(p+7)) << 56 ) );
      else
#endif
        return endian_64( *((u64 *) p) );
    }
}

int CSystem::LoadROM()
{
  FILE * f;
  char * buffer;
  int i;
  int j;
  u64 temp;
  u32 scratch;

  f = fopen(myCfg->get_text_value("rom.decompressed","decompressed.rom"),"rb");
  if (!f)
  {
    f = fopen(myCfg->get_text_value("rom.srm","cl67srmrom.exe"),"rb");
    if (!f)
    {
      printf("%%SYS-F-NOROM: No original or decompressed ROM image found!\n");
      return -1;
    }
    printf("%%SYS-I-READROM: Reading original ROM image from %s.\n", myCfg->get_text_value("rom.srm","cl67srmrom.exe"));
    for(i=0;i<0x240;i++)
    {
      if (feof(f)) break;
      fread(&scratch,1,1,f);
    }
    if (feof(f))
    {
      printf("%%SYS-F-2SMALL: File is too short to be a ROM image!\n");
      return -1;
    }
    buffer = PtrToMem(0x900000);
    while (!feof(f))
      fread(buffer++,1,1,f);
    fclose(f);

    printf("%%SYS-I-DECOMP: Decompressing ROM image.\n0%%");
    acCPUs[0]->set_pc(0x900001);
    acCPUs[0]->set_PAL_BASE(0x900000);

    j = 0;
    while (acCPUs[0]->get_clean_pc() > 0x200000)
    {
      for(i=0;i<1800000;i++)
      {
        SingleStep();
        if (acCPUs[0]->get_clean_pc() < 0x200000)
          break;
      }
      j++;
      if (((j%5)==0) && (j<50))
        printf("%d%%",j*2);
      else
        printf(".");
    }
    printf("100%%\n");

    f = fopen(myCfg->get_text_value("rom.decompressed","decompressed.rom"),"wb");
    if (!f)
    {
      printf("%%SYS-W-NOWRITE: Couldn't write decompressed rom to %s.\n", myCfg->get_text_value("rom.decompressed","decompressed.rom"));
    }
    else
    {
      printf("%%SYS-I-ROMWRT: Writing decompressed rom to %s.\n", myCfg->get_text_value("rom.decompressed","decompressed.rom"));
      temp = endian_64(acCPUs[0]->get_pc());
      fwrite(&temp,1,sizeof(u64),f);
      temp = endian_64(acCPUs[0]->get_pal_base());
      fwrite(&temp,1,sizeof(u64),f);
      buffer = PtrToMem(0);
      fwrite(buffer,1,0x200000,f);
      fclose(f);
    }
  }
  else
  {
    printf("%%SYS-I-READROM: Reading decompressed ROM image from %s.\n", myCfg->get_text_value("rom.decompressed","decompressed.rom"));
    fread(&temp,1,sizeof(u64),f);
    acCPUs[0]->set_pc(endian_64(temp));
    fread(&temp,1,sizeof(u64),f);
    acCPUs[0]->set_PAL_BASE(endian_64(temp));
    buffer = PtrToMem(0);
    fread(buffer,1,0x200000,f);
    fclose(f);
  }
#if !defined(SRM_NO_SPEEDUPS) || !defined(SRM_NO_IDE)
  printf("%%SYM-I-PATCHROM: Patching ROM for speed.\n");
#endif

#if !defined(SRM_NO_SPEEDUPS)
  WriteMem(X64(14248),32,0xe7e00000);       // e7e00000 = BEQ r31, +0
  WriteMem(X64(14288),32,0xe7e00000);       
  WriteMem(X64(142c8),32,0xe7e00000);       
  WriteMem(X64(68320),32,0xe7e00000);       
  WriteMem(X64(8bb78),32,0xe7e00000);       // memory test (aa)
  WriteMem(X64(8bc0c),32,0xe7e00000);       // memory test (bb)
  WriteMem(X64(8bc94),32,0xe7e00000);       // memory test (00)
#endif


  // THIS SRM REPLACEMENT IS INCOMPATIBLE WITH SCSI!!
#if !defined(SRM_NO_IDE)
//  WriteMem(X64(b66c0),32,0x00123401);       // SRM_READ_IDE_DISK
//  WriteMem(X64(b66c4),32,0x6bfa8001);       // JMP r31, r26
#endif

  printf("%%SYS-I-ROMLOADED: ROM Image loaded successfully!\n");
  return 0;
}
 
void CSystem::interrupt(int number, bool assert)
{
  int i;

  if (number==-1)
    {
      // timer int...
      state.c_MISC |= 0xf0;
      for(i=0;i<iNumCPUs;i++)
	acCPUs[i]->irq_h(2,true);
    }
  else if (assert)
    {
      //        if (!(state.c_DRIR & (1i64<<number)))
      //            printf("%%TYP-I-INTERRUPT: Interrupt %d asserted.\n",number);
      state.c_DRIR |= (X64(1)<<number);
    }
  else
    {
      //        if (state.c_DRIR & (1i64<<number))
      //            printf("%%TYP-I-INTERRUPT: Interrupt %d deasserted.\n",number);
      state.c_DRIR &= ~(X64(1)<<number);
    }
  for (i=0;i<iNumCPUs;i++)
    {
      if (state.c_DRIR & state.c_DIM[i] & X64(00ffffffffffffff))
	acCPUs[i]->irq_h(1,true);
      else
	acCPUs[i]->irq_h(1,false);

      if (state.c_DRIR & state.c_DIM[i] & X64(fc00000000000000))
	acCPUs[i]->irq_h(0,true);
      else
	acCPUs[i]->irq_h(0,false);
    }

}

u64 CSystem::PCI_Phys(int pcibus, u64 address)
{
  u64 a;
  int j;

//      printf("-------------- PCI MEMORY ACCESS FOR PCI HOSE %d --------------\n", pcibus);
//  //Step through windows
//  	for(j=0;j<4;j++)
//  	{
//        printf("WSBA%d: %016" LL "x WSM: %016" LL "x TBA: %016" LL "x\n",
//          j,state.p_WSBA[pcibus][j], state.p_WSM[pcibus][j], state.p_TBA[pcibus][j]);
//      }
//    printf("--------------------------------------------------------------\n");
    
  //Step through windows
  for(j=0;j<4;j++)
  {
    if (      (state.p_WSBA[pcibus][j] & 1)									// window enabled...
      && ! ((address ^ state.p_WSBA[pcibus][j]) & 0xfff00000 & ~state.p_WSM[pcibus][j]))	// address in range...
	{
      if (state.p_WSBA[pcibus][j] & 2)
        FAILURE("Don't know how to handle scatter-gather yet!!");
	  a = (address & ((state.p_WSM[pcibus][j] & X64(fff00000)) | 0xfffff)) + (state.p_TBA[pcibus][j] & X64(3fffc0000));
//	  printf("PCI memory address %08x translated to %016"LL "x\n",address, a);
      return a;
	}
  }
  return X64(80000000000) | (pcibus * X64(200000000)) | (u64)address;
}

void CSystem::SaveState(char *fn)
{
  FILE * f;
  int i;
  unsigned int m;
  unsigned int j;
  int * mem = (int*) memory;
  int int0 = 0;
  unsigned int memints = (1<<iNumMemoryBits)/sizeof(int);
  u32 temp_32;

  f = fopen(fn,"wb");
  if (f)
    {
      temp_32 = 0xa1fae540;     // MAGIC NUMBER (ALFAES40 ==> A1FAE540 )
      fwrite(&temp_32,sizeof(u32),1,f);
      temp_32 = 0x00020001;     // File Format Version 2.1
      fwrite(&temp_32,sizeof(u32),1,f);

      // memory
      for (m=0 ; m<memints ;m++)
	{
	  if (mem[m])
	    {
	      fwrite(&(mem[m]),1,sizeof(int),f);
	    }
	  else
	    {
	      j = 0;
	      m++;
	      while (!mem[m] && (m<memints))
		{
		  m++;
		  j++;
		  if ((int)j == -1)
		    break;
		}
	      if (mem[m]) 
		m--;
	      fwrite(&int0,1,sizeof(int),f);
	      fwrite(&j,1,sizeof(int),f);
	    }
	}

      fwrite(&state, sizeof(state), 1, f);
      
      // components
      //
      //  Components should also save any non-initial memory-registrations and re-register upon restore!
      //

      for (i=0;i<iNumComponents;i++)
	    acComponents[i]->SaveState(f);
      fclose(f);
    }
}

void CSystem::RestoreState(char *fn)
{
  FILE * f;
  int i;
  unsigned int m;
  unsigned int j;
  int * mem = (int*) memory;
  unsigned int memints = (1<<iNumMemoryBits)/sizeof(int);
  u32 temp_32;

  f = fopen(fn,"rb");
  if (!f)
    {
      printf("%%SYS-F-NOFILE: Can't open restore file %s\n",fn);
      return;
    }
  fread(&temp_32,sizeof(u32),1,f);
  if (temp_32 != 0xa1fae540)    // MAGIC NUMBER (ALFAES40 ==> A1FAE540 )
  {
    if (temp_32 == 0x40e5faa1)
      printf("%%SYS-F-ENDIAN: State file %s can't be restored because it was created on a"
             " machine of different endianness.\n",fn);     
    else
      printf("%%SYS-F-FORMAT: %s does not appear to be a state file.\n",fn);
    return;
  }

  fread(&temp_32,sizeof(u32),1,f);
  
  if (temp_32 != 0x00020001)     // File Format Version 2.1
  {
    printf("%%SYS-I-VERSION: State file %s is a different version.\n",fn);
    return;
  }

  // memory
  for (m=0;m<memints;m++)
    {
      fread(&(mem[m]),1,sizeof(int),f);
      if (!mem[m])
	{
	  fread(&j,1,sizeof(int),f);
	  while (j--)
	    {
	      mem[++m] = 0;
	    }
	}
    }

  fread(&state, sizeof(state), 1, f);
  
  // components
  //
  //  Components should also save any non-initial memory-registrations and re-register upon restore!
  //

  for (i=0;i<iNumComponents;i++)
  {
   if (acComponents[i]->RestoreState(f))
    throw((int)1);
  }
  fclose(f);
}

void CSystem::DumpMemory(unsigned int filenum)
{

  char file[100];
  int x;
  int * mem = (int*) memory;
  FILE * f;

  sprintf(file,"memory_%012d.dmp",filenum);
  f = fopen(file,"wb");

  x = (1<<iNumMemoryBits)/sizeof(int)/2;

  while (!mem[x-1])
    x--;

  fwrite(mem,1,x*sizeof(int),f);
  fclose(f);
}

/**
 *  Dump system state to stdout for debugging purposes.
 **/
void CSystem::panic(char *message, int flags) 
{
  int cpunum,i;
  CAlphaCPU *cpu;
  printf("\n******** SYSTEM PANIC *********\n");
  printf("* %s\n",message);
  printf("*******************************\n");
  for(cpunum=0;cpunum<iNumCPUs;cpunum++) {
    cpu = acCPUs[cpunum];
    printf("\n==================== STATE OF CPU %d ====================\n",cpunum);

    printf("PC: %016" LL "x\n", cpu->get_pc());
#ifdef IDB
    printf("Physical PC: %016" LL "x\n", cpu->get_current_pc_physical());
    printf("Instruction Count: %" LL "d\n", cpu->get_instruction_count());
#endif
    printf("\n");


    for(i=0;i<32;i++)
      {
	if (i<10) printf("R");
	printf("%d:%016" LL "x", i, cpu->get_r(i,false));
	if (i%4==3) printf("\n"); else printf(" ");
      }
    printf("\n");
    for(i=4;i<8;i++)
      {
	if (i<10) printf("S");
	printf("%d:%016" LL "x", i, cpu->get_r(i+32,false));
	if (i%4==3) printf("\n"); else printf(" ");
      }
    for(i=20;i<24;i++)
      {
	if (i<10) printf("S");
	printf("%d:%016" LL "x", i, cpu->get_r(i+32,false));
	if (i%4==3) printf("\n"); else printf(" ");
      }
    printf("\n");
    for(i=0;i<32;i++)
      {
	if (i<10) printf("F");
        printf("%d:%016" LL "x", i, cpu->get_f(i));
        if (i%4==3) printf("\n"); else printf(" ");
      }
  }
  printf("\n");
#ifdef IDB
  if(flags & PANIC_LISTING) {
    u64 start,end,xpc;
    start = cpu->get_pc() - 64;
    end = start + 128;
    cpu->listing(start,end,cpu->get_pc());
  }
#endif

  if(flags & PANIC_ASKSHUTDOWN) {
    printf("Stop Emulation? ");
    int c = getc(stdin);
    if(c == 'y' || c=='Y')
      flags |= PANIC_SHUTDOWN;
  }

  if(flags & PANIC_SHUTDOWN) {
    throw((int)1);
  }

  return;
}

void CSystem::clear_clock_int(int ProcNum)
{
  state.c_MISC &=~(X64(10)<<ProcNum);
  acCPUs[ProcNum]->irq_h(2,false);
}

#if defined(PROFILE)
u64 profile_buckets[PROFILE_BUCKETS];
u64 profiled_insts;
bool profile_started = false;
#endif

CSystem * theSystem = 0;

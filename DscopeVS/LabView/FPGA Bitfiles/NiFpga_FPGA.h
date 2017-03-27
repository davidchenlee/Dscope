/*
 * Generated with the FPGA Interface C API Generator 16.0.0
 * for NI-RIO 16.0.0 or later.
 */

#ifndef __NiFpga_FPGA_h__
#define __NiFpga_FPGA_h__

#ifndef NiFpga_Version
   #define NiFpga_Version 1600
#endif

#include "NiFpga.h"

/**
 * The filename of the FPGA bitfile.
 *
 * This is a #define to allow for string literal concatenation. For example:
 *
 *    static const char* const Bitfile = "C:\\" NiFpga_FPGA_Bitfile;
 */
#define NiFpga_FPGA_Bitfile "NiFpga_FPGA.lvbitx"

/**
 * The signature of the FPGA bitfile.
 */
static const char* const NiFpga_FPGA_Signature = "CCD4442164BFF4893290A3570C014A36";

typedef enum
{
   NiFpga_FPGA_ControlBool_start = 0x810E,
} NiFpga_FPGA_ControlBool;

typedef enum
{
   NiFpga_FPGA_ControlU32_DigitalFIFOdelayTicks = 0x8110,
   NiFpga_FPGA_ControlU32_LoopPeriodtick = 0x8114,
} NiFpga_FPGA_ControlU32;

typedef enum
{
   NiFpga_FPGA_HostToTargetFifoBool_DOFIFO = 0,
} NiFpga_FPGA_HostToTargetFifoBool;

typedef enum
{
   NiFpga_FPGA_HostToTargetFifoI16_A0FIFO = 1,
} NiFpga_FPGA_HostToTargetFifoI16;

#endif

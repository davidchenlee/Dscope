/*
 * Generated with the FPGA Interface C API Generator 16.0.0
 * for NI-RIO 16.0.0 or later.
 */

#ifndef __NiFpga_FPGAvi_h__
#define __NiFpga_FPGAvi_h__

#ifndef NiFpga_Version
   #define NiFpga_Version 1600
#endif

#include "NiFpga.h"

/**
 * The filename of the FPGA bitfile.
 *
 * This is a #define to allow for string literal concatenation. For example:
 *
 *    static const char* const Bitfile = "C:\\" NiFpga_FPGAvi_Bitfile;
 */
#define NiFpga_FPGAvi_Bitfile "NiFpga_FPGAvi.lvbitx"

/**
 * The signature of the FPGA bitfile.
 */
static const char* const NiFpga_FPGAvi_Signature = "C42DAFADB79FBF29E5DC2ED189D3517C";

typedef enum
{
   NiFpga_FPGAvi_IndicatorBool_RSisRunning = 0x4E,
} NiFpga_FPGAvi_IndicatorBool;

typedef enum
{
   NiFpga_FPGAvi_IndicatorI16_RSvoltageMon_I16 = 0x66,
} NiFpga_FPGAvi_IndicatorI16;

typedef enum
{
   NiFpga_FPGAvi_IndicatorU16_RescanGalvoMon = 0x4A,
   NiFpga_FPGAvi_IndicatorU16_ScanGalvoMon = 0x1A,
} NiFpga_FPGAvi_IndicatorU16;

typedef enum
{
   NiFpga_FPGAvi_ControlBool_AsyncTrigger = 0x56,
   NiFpga_FPGAvi_ControlBool_FIFOINtrigger = 0x9E,
   NiFpga_FPGAvi_ControlBool_FIFOOUTgateEnable = 0x46,
   NiFpga_FPGAvi_ControlBool_LineclockInputSelector = 0x52,
   NiFpga_FPGAvi_ControlBool_PcTrigger = 0x76,
   NiFpga_FPGAvi_ControlBool_PhotocounterInputSelector = 0x32,
   NiFpga_FPGAvi_ControlBool_PockelsAutoOffEnable = 0x1E,
   NiFpga_FPGAvi_ControlBool_RSrun = 0x86,
   NiFpga_FPGAvi_ControlBool_ShutterFidelity = 0x7A,
   NiFpga_FPGAvi_ControlBool_ShutterVision = 0x7E,
   NiFpga_FPGAvi_ControlBool_VTback = 0x8E,
   NiFpga_FPGAvi_ControlBool_VTforward = 0x92,
   NiFpga_FPGAvi_ControlBool_VTstart = 0x8A,
} NiFpga_FPGAvi_ControlBool;

typedef enum
{
   NiFpga_FPGAvi_ControlU8_MainTriggerSelector = 0x26,
   NiFpga_FPGAvi_ControlU8_Nchannels = 0x9A,
   NiFpga_FPGAvi_ControlU8_nPMTsim = 0x12,
} NiFpga_FPGAvi_ControlU8;

typedef enum
{
   NiFpga_FPGAvi_ControlI16_Nframes = 0x6E,
   NiFpga_FPGAvi_ControlI16_NlinesPerFrame = 0x72,
   NiFpga_FPGAvi_ControlI16_Npreframes = 0x42,
   NiFpga_FPGAvi_ControlI16_RSvoltage_I16 = 0x82,
} NiFpga_FPGAvi_ControlI16;

typedef enum
{
   NiFpga_FPGAvi_ControlI32_FIFOtimeout_tick = 0x60,
   NiFpga_FPGAvi_ControlI32_NlinesAll = 0x94,
} NiFpga_FPGAvi_ControlI32;

typedef enum
{
   NiFpga_FPGAvi_ControlU32_DOdelay_tick = 0x5C,
   NiFpga_FPGAvi_ControlU32_LinegateTimeout_tick = 0x68,
   NiFpga_FPGAvi_ControlU32_PockelsFirstFrameDelay_tick = 0x58,
   NiFpga_FPGAvi_ControlU32_PockelsFrameDelay_tick = 0x3C,
   NiFpga_FPGAvi_ControlU32_PostsequenceTimer_tick = 0x38,
   NiFpga_FPGAvi_ControlU32_PreframeclockRescanGalvo_tick = 0x28,
   NiFpga_FPGAvi_ControlU32_PreframeclockScanGalvo_tick = 0x2C,
   NiFpga_FPGAvi_ControlU32_StageDebouncerTimer_tick = 0x20,
   NiFpga_FPGAvi_ControlU32_StageTrigAcqDelay_tick = 0x34,
} NiFpga_FPGAvi_ControlU32;

typedef enum
{
   NiFpga_FPGAvi_ControlArrayBool_PMTsimArray = 0x14,
} NiFpga_FPGAvi_ControlArrayBool;

typedef enum
{
   NiFpga_FPGAvi_ControlArrayBoolSize_PMTsimArray = 20,
} NiFpga_FPGAvi_ControlArrayBoolSize;

typedef enum
{
   NiFpga_FPGAvi_TargetToHostFifoU32_FIFOOUTa = 1,
   NiFpga_FPGAvi_TargetToHostFifoU32_FIFOOUTb = 0,
} NiFpga_FPGAvi_TargetToHostFifoU32;

typedef enum
{
   NiFpga_FPGAvi_HostToTargetFifoU32_FIFOIN = 2,
} NiFpga_FPGAvi_HostToTargetFifoU32;

#endif

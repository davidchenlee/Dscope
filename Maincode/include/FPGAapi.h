#pragma once
#include <iostream>
#include <string>	//For std::to_string
#include "NiFpga_FPGAvi.h"
#include "Const.h"

using namespace Const;
using namespace Parameters;

/*Define the full path of the bitfile. The bitfile is the FPGA code*/
static const char* Bitfile = "D:\\OwnCloud\\Codes\\MESOscope\\LabView\\FPGA Bitfiles\\" NiFpga_FPGAvi_Bitfile;

namespace FPGAfunctions
{
	U16 convertUs2tick(const double t_us);
	I16 convertVolt2I16(const double voltage_V);
	U32 packU32(const U16 t_tick, const U16 val);
	U32 packAnalogSinglet(const double t_us, const double val);
	U32 packDigitalSinglet(const double t_us, const bool DO);
	U32 packPixelclockSinglet(const double t_us, const bool DO);
}

class FPGAapi
{	
	NiFpga_Session mSession;
public:
	FPGAapi();
	~FPGAapi();
	void initialize() const;
	void writeFIFO(VQU32 &vectorqueues) const;
	void triggerRT() const;
	void flushFIFO() const;
	void close(const bool reset) const;
	NiFpga_Session getSession() const;
};

void checkFPGAstatus(char functionName[], NiFpga_Status status);

class FPGAexception : public std::runtime_error
{
public:
	FPGAexception(const std::string& message) : std::runtime_error(message.c_str()) {}
};
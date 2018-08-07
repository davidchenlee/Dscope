#pragma once
//#include <iostream>
//#include <string>		//std::to_string
#include <windows.h>	//Sleep. Also the PI stages
#include "NiFpga_FPGAvi.h"
#include "Const.h"
#include "Utilities.h"
using namespace Constants;

namespace FPGAns
{
	U16 convertUsTotick(const double t);
	I16 convertVoltToI16(const double voltage_V);
	double convertI16toVolt(const int input);
	U32 packU32(const U16 t_tick, const U16 AO_U16);
	U32 packAnalogSinglet(const double timeStep, const double AO_V);
	U32 packDigitalSinglet(const double timeStep, const bool DO);
	U32 packPixelclockSinglet(const double timeStep, const bool DO);
	void checkStatus(char functionName[], NiFpga_Status status);

	//Establish a connection to the FPGA
	class FPGA
	{	
		NiFpga_Session mFpgaHandle;											//FPGA handle. Non-const to let the FPGA API assign the handle
		const std::string mBitfile = bitfilePath + NiFpga_FPGAvi_Bitfile;	//FPGA bitfile location

		void FIFOOUTpcGarbageCollector_() const;
		void flushBRAMs_() const;
	public:
		FPGA();
		~FPGA();
		void initialize() const;
		void writeFIFOINpc(const VQU32 &vectorqueues) const;
		void triggerRT() const;
		void close(const bool reset) const;
		NiFpga_Session getFpgaHandle() const;
	};

	//Create a realtime sequence and pixelclock
	class RTsequence
	{
		const FPGAns::FPGA &mFpga;
		VQU32 mVectorOfQueues;

		void concatenateQueues_(QU32& receivingQueue, QU32& givingQueue) const;

		//Private subclass
		class Pixelclock
		{
			const FPGAns::FPGA &mFpga;
			QU32 mPixelclockQ;					//Queue containing the pixel-clock sequence
			const int mLatency_tick = 2;		//Latency at detecting the line clock. Calibrate the latency with the oscilloscope
			void pushUniformDwellTimes(const int calibFine_tick, const double dwellTime_us);
		public:
			Pixelclock(const FPGAns::FPGA &fpga);
			~Pixelclock();
			QU32 readPixelclock() const;
		};

	public:
		RTsequence(const FPGAns::FPGA &fpga);
		RTsequence(const RTsequence&) = delete;				//Disable copy-constructor
		RTsequence& operator=(const RTsequence&) = delete;	//Disable assignment-constructor
		~RTsequence();
		void pushQueue(const RTchannel chan, QU32& queue);
		void clearQueue(const RTchannel chan);
		void pushDigitalSinglet(const RTchannel chan, double timeStep, const bool DO);
		void pushAnalogSinglet(const RTchannel chan, double timeStep, const double AO_V);
		void pushAnalogSingletFx2p14(const RTchannel chan, const double scalingFactor);
		void pushLinearRamp(const RTchannel chan, double timeStep, const double rampLength, const double Vi_V, const double Vf_V);
		void initializeRT() const;
		FPGAns::FPGA getSession() const;
	};

	class FPGAexception : public std::runtime_error
	{
	public:
		FPGAexception(const std::string& message) : std::runtime_error(message.c_str()) {}
	};
}
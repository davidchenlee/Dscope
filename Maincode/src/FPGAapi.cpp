#include "FPGAapi.h"

namespace FPGAfunc
{
	//Convert time to ticks
	U16 timeToTick(const double t)
	{
		const double t_tick{ t / us * g_tickPerUs };

		if (static_cast<U32>(t_tick) > 0x0000FFFF)
		{
			std::cerr << "WARNING in " << __FUNCTION__ << ": Time step overflow. Time step cast to the max: " << std::fixed << (std::numeric_limits<U16>::max)() * g_usPerTick << " us\n";
			return (std::numeric_limits<U16>::max)();
		}
		else if (static_cast<U32>(t_tick) < g_tMin_tick)
		{
			std::cerr << "WARNING in " << __FUNCTION__ << ": Time step underflow. Time step cast to the min: " << std::fixed << g_tMin_tick * g_usPerTick << " us\n";
			return g_tMin_tick;
		}
		else
			return static_cast<U16>(t_tick);
	}

	//Convert voltage (-10V to 10V) to I16 (-32768 to 32767)
	//Examples of I16 numbers:
	//0x7FFF = 32767
	//0xFFFF = -1
	//0x8000 = -32768
	I16 voltageToI16(const double voltage)
	{
		//Positive case
		if (voltage >= 0)
		{
			if (voltage > g_AOmax)
			{
				std::cerr << "WARNING in " << __FUNCTION__ << ": Voltage overflow. Voltage clipeed to the max: " + std::to_string(g_AOmax / V) + " V\n";
				return static_cast<I16>((std::numeric_limits<I16>::max)());
			}
			else
				return static_cast<I16>(voltage / g_AOmax * (std::numeric_limits<I16>::max)());
		}
		else //Negative case
		{
			if (voltage < -g_AOmax)
			{
				std::cerr << "WARNING in " << __FUNCTION__ << ": Voltage underflow. Voltage clipped to the min: " + std::to_string(-g_AOmax / V) + " V\n";
				return static_cast<I16>((std::numeric_limits<I16>::min)());
			}
			else
				return static_cast<I16>(voltage / g_AOmax * -(std::numeric_limits<I16>::min)());
		}
	}

	//Convert an int in the range -32768 to 32767 to voltage -10V to 10V
	double intToVoltage(const int input)
	{
		//Positive case
		if (input >= 0)
		{
			//Check for overflow
			if (input > (std::numeric_limits<I16>::max)())
			{
				std::cerr << "WARNING in " << __FUNCTION__ << ": Input int overflow, _I16_MAX used instead\n";
				return g_AOmax / V;
			}
			else
				return 1. * input / (std::numeric_limits<I16>::max)() * g_AOmax;
		}
		else //Negative case
		{
			//Check for underoverflow
			if (input < (std::numeric_limits<I16>::min)())
			{
				std::cerr << "WARNING in " << __FUNCTION__ << ": Input int underflow, _I16_MIN used instead\n";
				return -g_AOmax / V;
			}
			else
				return -1. * input / (std::numeric_limits<I16>::min)()  * g_AOmax;
		}
	}


	//Pack t in MSB and x in LSB. Time t and analog output AO_U16 are encoded in 16 bits each.
	U32 packU32(const U16 t_tick, const U16 AO_U16)
	{
		return (t_tick << 16) | (0x0000FFFF & AO_U16);
	}

	//Send out an analog instruction, where the analog output 'AO' is held for 'timeStep'
	U32 packAnalogSinglet(const double timeStep, const double AO)
	{
		const U16 AOlatency_tick{ 2 };	//To calibrate, run AnalogLatencyCalib(). I think the latency comes from the memory block, which takes 2 cycles to read
		return packU32(timeToTick(timeStep) - AOlatency_tick, voltageToI16(AO / V));
	}

	//Send out a single digital instruction, where 'DO' is held LOW or HIGH for the amount of time 'timeStep'. The DOs in Connector1 are rated at 10MHz, Connector0 at 80MHz.
	U32 packDigitalSinglet(const double timeStep, const bool DO)
	{
		const U16 DOlatency_tick{ 2 };	//To calibrate, run DigitalLatencyCalib(). I think the latency comes from the memory block, which takes 2 cycles to read
		return packU32(timeToTick(timeStep) - DOlatency_tick, static_cast<U16>(DO));
	}

	//Generate a single pixel-clock instruction, where 'DO' is held LOW or HIGH for the amount of time 'timeStep'
	U32 packPixelclockSinglet(const double timeStep, const bool DO)
	{
		const U16 PixelclockLatency_tick{ 1 };//The pixel-clock is implemented using a SCTL. I think the latency comes from reading the LUT buffer
		return packU32(timeToTick(timeStep) - PixelclockLatency_tick, static_cast<U16>(DO));
	}

	void checkStatus(char functionName[], NiFpga_Status status)
	{
		if (status < 0)
			throw  FPGAexception((std::string)functionName + " with FPGA code " + std::to_string(status));
		if (status > 0)
			std::cerr << "A warning has ocurred in " << functionName << " with FPGA code " << status << "\n";
	}

	void linearRamp(QU32 &queue, double timeStep, const double rampLength, const double Vi, const double Vf)
	{
		if (timeStep < g_tMinAO)
		{
			std::cerr << "WARNING in " << __FUNCTION__ << ": Time step too small. Time step cast to " << g_tMinAO / us << " us\n";
			timeStep = g_tMinAO;		//Analog Out time increment in us
		}

		const int nPoints{ static_cast<int>(rampLength / timeStep) };		//Number of points

		if (nPoints <= 1)
			throw std::invalid_argument((std::string)__FUNCTION__ + ": Not enought points to generate a linear ramp");

		//For debugging
		//std::cout << "nPoints: " << nPoints << "\n";
		//std::cout << "time \tticks \tv\n";

		for (int ii = 0; ii < nPoints; ii++)
		{
			const double V{ Vi + (Vf - Vi)*ii / (nPoints - 1) };
			queue.push_back(FPGAfunc::packAnalogSinglet(timeStep, V));

			//std::cout << (ii + 1) * timeStep << "\t" << (ii + 1) * timeToTick(timeStep) << "\t" << V << "\t\n";	//For debugging
		}
		//getchar();	//For debugging
	}
}//namespace

#pragma region "FPGA"
FPGA::FPGA()
{
	//Must be called before any other FPGA calls
	FPGAfunc::checkStatus(__FUNCTION__, NiFpga_Initialize());

	//Opens a session, uploads the bitfile to the FPGA. 1=no run, 0=run
	FPGAfunc::checkStatus(__FUNCTION__, NiFpga_Open(mBitfile.c_str(), NiFpga_FPGAvi_Signature, "RIO0", 0, &mHandle));

	//Set up the FPGA parameters
	initializeFpga_();
}

FPGA::~FPGA()
{
	//std::cout << "FPGA destructor was called\n";
};

//The object has to be closed explicitly because of the exception catching
void FPGA::close(const FPGARESET reset) const
{
	//Closes the session to the FPGA. The FPGA resets (Re-downloads the FPGA bitstream to the target, the outputs go to zero)
	//unless either another session is still open or you use the NiFpga_CloseAttribute_NoResetIfLastSession attribute.
	uint32_t resetFlag;
	switch (reset)
	{
	case FPGARESET::EN:
		resetFlag = 0;
		break;
	case FPGARESET::DIS:
		resetFlag = 1;
	}
	FPGAfunc::checkStatus(__FUNCTION__, NiFpga_Close(mHandle, resetFlag));	//Arg of NiFpga_Close(): 0 to resets, 1 does not reset

	if (reset == FPGARESET::EN)
		std::cout << "The FPGA has been successfully reset\n";

	//You must call this function after all other function calls if NiFpga_Initialize succeeds. This function unloads the NiFpga library.
	FPGAfunc::checkStatus(__FUNCTION__, NiFpga_Finalize());
}

NiFpga_Session FPGA::handle() const
{
	return mHandle;
}

//Lineclock: resonant scanner (RS) or function generator (FG)
void FPGA::setLineclock(const LINECLOCK lineclockInput) const
{
	FPGAfunc::checkStatus(__FUNCTION__, NiFpga_WriteBool(mHandle, NiFpga_FPGAvi_ControlBool_LineclockInputSelector, static_cast<bool>(lineclockInput)));
}

//Establish a connection between FIFOOUTpc and FIFOOUTfpga and. Optional according to NI
void FPGA::startFIFOOUTpc() const
{
	FPGAfunc::checkStatus(__FUNCTION__, NiFpga_StartFifo(mHandle, NiFpga_FPGAvi_TargetToHostFifoU32_FIFOOUTa));
	FPGAfunc::checkStatus(__FUNCTION__, NiFpga_StartFifo(mHandle, NiFpga_FPGAvi_TargetToHostFifoU32_FIFOOUTb));
}

//Configure FIFOOUTpc. Optional according to NI
void FPGA::configureFIFOOUTpc(const U32 depth) const
{
	U32 actualDepth;
	FPGAfunc::checkStatus(__FUNCTION__, NiFpga_ConfigureFifo2(mHandle, NiFpga_FPGAvi_TargetToHostFifoU32_FIFOOUTa, depth, &actualDepth));
	FPGAfunc::checkStatus(__FUNCTION__, NiFpga_ConfigureFifo2(mHandle, NiFpga_FPGAvi_TargetToHostFifoU32_FIFOOUTb, depth, &actualDepth));
	std::cout << "ActualDepth a: " << actualDepth << "\t" << "ActualDepth b: " << actualDepth << "\n";
}

//Flush the residual data in FIFOOUTpc from the previous run, if any
void FPGA::collectFIFOOUTpcGarbage_() const
{
	const U32 timeout_ms{ 100 };
	const U32 bufSize{ 10000 };

	U32 dummy;
	U32* garbage{ new U32[bufSize] };
	U32 nElemToReadA{ 0 }, nElemToReadB{ 0 };			//Elements to read from FIFOOUTpc A and B
	int nElemTotalA{ 0 }, nElemTotalB{ 0 }; 			//Total number of elements read from FIFOOUTpc A and B
	while (true)
	{
		//Check if there are elements in FIFOOUTpc
		FPGAfunc::checkStatus(__FUNCTION__, NiFpga_ReadFifoU32(mHandle, NiFpga_FPGAvi_TargetToHostFifoU32_FIFOOUTa, garbage, 0, timeout_ms, &nElemToReadA));
		FPGAfunc::checkStatus(__FUNCTION__, NiFpga_ReadFifoU32(mHandle, NiFpga_FPGAvi_TargetToHostFifoU32_FIFOOUTb, garbage, 0, timeout_ms, &nElemToReadB));
		//std::cout << "FIFOOUTpc cleanup A/B: " << nElemToReadA << "/" << nElemToReadB << "\n";
		//getchar();

		if (nElemToReadA == 0 && nElemToReadB == 0)
			break;

		if (nElemToReadA > 0)
		{
			nElemToReadA = (std::min)(bufSize, nElemToReadA);	//Min between bufSize and nElemToReadA
			FPGAfunc::checkStatus(__FUNCTION__, NiFpga_ReadFifoU32(mHandle, NiFpga_FPGAvi_TargetToHostFifoU32_FIFOOUTa, garbage, nElemToReadA, timeout_ms, &dummy));	//Retrieve the elements in FIFOOUTpc
			nElemTotalA += nElemToReadA;
		}
		if (nElemToReadB > 0)
		{
			nElemToReadB = (std::min)(bufSize, nElemToReadB);	//Min between bufSize and nElemToReadB
			FPGAfunc::checkStatus(__FUNCTION__, NiFpga_ReadFifoU32(mHandle, NiFpga_FPGAvi_TargetToHostFifoU32_FIFOOUTb, garbage, nElemToReadB, timeout_ms, &dummy));	//Retrieve the elements in FIFOOUTpc
			nElemTotalB += nElemToReadB;
		}
	}
	if (nElemTotalA > 0 || nElemTotalB > 0)
		std::cout << "FIFOOUTpc garbage collector called. Number of elements cleaned up in FIFOOUTpc A/B: " << nElemTotalA << "/" << nElemTotalB << "\n";
}

//Stop the connection between FIFOOUTpc and FIFOOUTfpga. Optional according to NI
void FPGA::stopFIFOOUTpc() const
{
	FPGAfunc::checkStatus(__FUNCTION__, NiFpga_StopFifo(mHandle, NiFpga_FPGAvi_TargetToHostFifoU32_FIFOOUTa));
	FPGAfunc::checkStatus(__FUNCTION__, NiFpga_StopFifo(mHandle, NiFpga_FPGAvi_TargetToHostFifoU32_FIFOOUTb));
	//std::cout << "stopFIFO called\n";
}

//Trigger the AOs of the FPGA externally instead of using the lineclock and frameclock (see the LV implementation)
void FPGA::triggerAOext() const
{
	FPGAfunc::checkStatus(__FUNCTION__, NiFpga_WriteBool(mHandle, NiFpga_FPGAvi_ControlBool_TriggerAODOexternal, true));
	FPGAfunc::checkStatus(__FUNCTION__, NiFpga_WriteBool(mHandle, NiFpga_FPGAvi_ControlBool_TriggerAODOexternal, false));
}

//Enable the FPGA to push the photocounts to FIFOOUTfpga. Disabled when debugging
void FPGA::enableFIFOOUTfpga(const FIFOOUTfpga enableFIFOOUTfpga) const
{
	if (enableFIFOOUTfpga == FIFOOUTfpga::EN)
		FPGAfunc::checkStatus(__FUNCTION__, NiFpga_WriteBool(mHandle, NiFpga_FPGAvi_ControlBool_FIFOOUTgateEnable, true));
}

void FPGA::enablePockelsScaling() const
{
	FPGAfunc::checkStatus(__FUNCTION__, NiFpga_WriteBool(mHandle, NiFpga_FPGAvi_ControlBool_PockelsScalingFactorEnable, true));
}

//Enable the stage trigger the ctl&acq sequence
void FPGA::enableStageTrigAcq(const MAINTRIG mainTrigger) const
{
	switch (mainTrigger)	//Trigger selector 
	{
	case MAINTRIG::STAGEX:
		FPGAfunc::checkStatus(__FUNCTION__, NiFpga_WriteU8(mHandle, NiFpga_FPGAvi_ControlU8_MainTriggerSelector, static_cast<U8>(MAINTRIG::STAGEX)));
		break;
	case MAINTRIG::STAGEZ:
		FPGAfunc::checkStatus(__FUNCTION__, NiFpga_WriteU8(mHandle, NiFpga_FPGAvi_ControlU8_MainTriggerSelector, static_cast<U8>(MAINTRIG::STAGEZ)));
		break;
	default:
		FPGAfunc::checkStatus(__FUNCTION__, NiFpga_WriteU8(mHandle, NiFpga_FPGAvi_ControlU8_MainTriggerSelector, static_cast<U8>(MAINTRIG::PC)));
	}
}

//Disable the stage triggering the ctl&acq sequence
void FPGA::disableStageTrigAcq() const
{
	FPGAfunc::checkStatus(__FUNCTION__, NiFpga_WriteU8(mHandle, NiFpga_FPGAvi_ControlU8_MainTriggerSelector, static_cast<U8>(MAINTRIG::PC)));
}

//Set the delay for the stages triggering the ctl&acq sequence
void FPGA::setStageTrigAcqDelay(const MAINTRIG mainTrigger, const int heightPerBeamletPerFrame_pix, const SCANDIR scanDir) const
{
	double stageTrigAcqDelay{ 0 };
	switch (mainTrigger)
	{
	case MAINTRIG::STAGEX:
		stageTrigAcqDelay = g_STAGEXTrigAcqDelay;
		break;
	case MAINTRIG::STAGEZ:
		if (heightPerBeamletPerFrame_pix == 35)
		{
			switch (scanDir)
			{
			case SCANDIR::UPWARD:
				stageTrigAcqDelay = g_STAGEZtrigAcqDelayTopdown;
				break;
			case SCANDIR::DOWNWARD:
				stageTrigAcqDelay = g_STAGEZTrigAcqDelayBottomup;
				break;
			}
		}
		else if (heightPerBeamletPerFrame_pix >= 400)//Do nothing if mHeightPerFrame_pix is big enough
			;
		else //Output a warning if stageTrigAcqDelay is uncalibrated
		{
			std::cerr << "WARNING in " << __FUNCTION__ << ": The stage trigger delay has not been calibrated for the heightPerFrame = " << heightPerBeamletPerFrame_pix << " pix\n";
			std::cerr << "Press any key to continue or ESC to exit\n";

			if (_getch() == 27)
				throw std::runtime_error((std::string)__FUNCTION__ + ": Control sequence terminated");
		}
		break;
	default:
		double stageTrigAcqDelay = 0;
	}

	FPGAfunc::checkStatus(__FUNCTION__, NiFpga_WriteU32(mHandle, NiFpga_FPGAvi_ControlU32_StageTrigAcqDelay_tick, static_cast<U32>(stageTrigAcqDelay / us * g_tickPerUs)));
}

//Load the imaging parameters onto the FPGA. See 'Const.cpp' for the definition of each variable
void FPGA::initializeFpga_() const
{
	if (g_FIFOtimeout_tick < 0 || g_DOdelay_tick < 0 || g_pockelsFirstFrameDelay < 0 || g_pockelsSecondaryDelay < 0 || g_scanGalvoDelay < 0 || g_rescanGalvoDelay < 0 || g_linegateTimeout < 0)
		throw std::invalid_argument((std::string)__FUNCTION__ + ": One or more imaging parameters take negative values");

	//PMT simulator for debugging
	FPGAfunc::checkStatus(__FUNCTION__, NiFpga_WriteU8(mHandle, NiFpga_FPGAvi_ControlBool_PhotocounterInputSelector, static_cast<bool>(g_photocounterInput)));	//Use the PMT simulator as the input of the photocounters
	FPGAfunc::checkStatus(__FUNCTION__, NiFpga_WriteU8(mHandle, NiFpga_FPGAvi_ControlU8_nPMTsim, g_nPMTsim));													//Size of g_PMTsimArray
	FPGAfunc::checkStatus(__FUNCTION__, NiFpga_WriteArrayBool(mHandle, NiFpga_FPGAvi_ControlArrayBool_PMTsimArray, g_PMTsimArray, g_nPMTsim));					//Array that simulates the pulses from the PMTs

	//FIFOIN
	FPGAfunc::checkStatus(__FUNCTION__, NiFpga_WriteU16(mHandle, NiFpga_FPGAvi_ControlU16_Nchannels, static_cast<U16>(RTcontrol::RTCHAN::NCHAN)));				//Number of input channels
	FPGAfunc::checkStatus(__FUNCTION__, NiFpga_WriteBool(mHandle, NiFpga_FPGAvi_ControlBool_FIFOINtrigger, false));												//Trigger of the control sequence
	FPGAfunc::checkStatus(__FUNCTION__, NiFpga_WriteI32(mHandle, NiFpga_FPGAvi_ControlI32_FIFOtimeout_tick, static_cast<I32>(g_FIFOtimeout_tick)));				//FIFOIN timeout

	//TRIGGERS
	FPGAfunc::checkStatus(__FUNCTION__, NiFpga_WriteBool(mHandle, NiFpga_FPGAvi_ControlBool_PcTrigger, false));													//Pc trigger signal
	FPGAfunc::checkStatus(__FUNCTION__, NiFpga_WriteBool(mHandle, NiFpga_FPGAvi_ControlBool_TriggerAODOexternal, false));										//Trigger the AOs of the FPGA externally

	//DELAYS
	FPGAfunc::checkStatus(__FUNCTION__, NiFpga_WriteU32(mHandle, NiFpga_FPGAvi_ControlU32_DOdelay_tick, static_cast<U32>(g_DOdelay_tick)));												//Delay DO to sync it with AO
	FPGAfunc::checkStatus(__FUNCTION__, NiFpga_WriteU32(mHandle, NiFpga_FPGAvi_ControlU32_PockelsFirstFrameDelay_tick, static_cast<U32>(g_pockelsFirstFrameDelay / us * g_tickPerUs)));	//Pockels delay wrt the preframeclock (first frame only)
	FPGAfunc::checkStatus(__FUNCTION__, NiFpga_WriteU32(mHandle, NiFpga_FPGAvi_ControlU32_PockelsFrameDelay_tick, static_cast<U32>(g_pockelsSecondaryDelay / us * g_tickPerUs)));		//Pockels delay wrt the preframeclock
	FPGAfunc::checkStatus(__FUNCTION__, NiFpga_WriteI16(mHandle, NiFpga_FPGAvi_ControlI16_Npreframes, static_cast<I16>(g_nPreframes)));													//Number of lineclocks separating the preframeclock(preframegate) and the frameclock (framegate)
	FPGAfunc::checkStatus(__FUNCTION__, NiFpga_WriteU32(mHandle, NiFpga_FPGAvi_ControlU32_PreframeclockScanGalvo_tick, static_cast<U32>(g_scanGalvoDelay / us * g_tickPerUs)));			//Scan galvo delay wrt the preframeclock
	FPGAfunc::checkStatus(__FUNCTION__, NiFpga_WriteU32(mHandle, NiFpga_FPGAvi_ControlU32_PreframeclockRescanGalvo_tick, static_cast<U32>(g_rescanGalvoDelay / us * g_tickPerUs)));		//Rescan galvo delay wrt the preframeclock
	FPGAfunc::checkStatus(__FUNCTION__, NiFpga_WriteU32(mHandle, NiFpga_FPGAvi_ControlU32_StageDebouncerTimer_tick, static_cast<U32>(g_stageDebounceTimer / us * g_tickPerUs)));		//Stage motion monitor debouncer

	if (g_linegateTimeout <= 2 * g_lineclockHalfPeriod)
		throw std::invalid_argument((std::string)__FUNCTION__ + ": The linegate timeout must be greater than the lineclock period");
	FPGAfunc::checkStatus(__FUNCTION__, NiFpga_WriteU32(mHandle, NiFpga_FPGAvi_ControlU32_LinegateTimeout_tick, static_cast<U32>(g_linegateTimeout / us * g_tickPerUs)));				//Timeout the trigger of the control sequence

	//POCKELS CELLS
	FPGAfunc::checkStatus(__FUNCTION__, NiFpga_WriteBool(mHandle, NiFpga_FPGAvi_ControlBool_PockelsAutoOffEnable, pockelsAutoOff));														//Enable or disable gating the pockels by framegate. For debugging purposes
	FPGAfunc::checkStatus(__FUNCTION__, NiFpga_WriteBool(mHandle, NiFpga_FPGAvi_ControlBool_PockelsScalingFactorEnable, false));														//Enable or disable scaling the pockels output. Disabled by default

	//VIBRATOME
	FPGAfunc::checkStatus(__FUNCTION__, NiFpga_WriteBool(mHandle, NiFpga_FPGAvi_ControlBool_VTstart, false));
	FPGAfunc::checkStatus(__FUNCTION__, NiFpga_WriteBool(mHandle, NiFpga_FPGAvi_ControlBool_VTback, false));
	FPGAfunc::checkStatus(__FUNCTION__, NiFpga_WriteBool(mHandle, NiFpga_FPGAvi_ControlBool_VTforward, false));

	//Flush the RAM buffers on the FPGA as precaution. 
	FPGAfunc::checkStatus(__FUNCTION__, NiFpga_WriteBool(mHandle, NiFpga_FPGAvi_ControlBool_FlushTrigger, false));
	FPGAfunc::checkStatus(__FUNCTION__, NiFpga_WriteBool(mHandle, NiFpga_FPGAvi_ControlBool_FlushTrigger, true));
	FPGAfunc::checkStatus(__FUNCTION__, NiFpga_WriteBool(mHandle, NiFpga_FPGAvi_ControlBool_FlushTrigger, false));
	//std::cout << "flushBRAMs called\n";	//For debugging

	/*
	//SHUTTERS. Commented out to allow keeping the shutter on
	checkStatus(__FUNCTION__,  NiFpga_WriteBool(mHandle, NiFpga_FPGAvi_ControlBool_Shutter1, false));
	checkStatus(__FUNCTION__,  NiFpga_WriteBool(mHandle, NiFpga_FPGAvi_ControlBool_Shutter2, false));

	//RESONANT SCANNER. Commented out to allow keeping the RS on
	checkStatus(__FUNCTION__,  NiFpga_WriteI16(mHandle, NiFpga_FPGAvi_ControlI16_RScontrol_I16, false));	//Output voltage
	checkStatus(__FUNCTION__,  NiFpga_WriteBool(mHandle, NiFpga_FPGAvi_ControlBool_RSenable, false));		//Turn on/off
	*/
}
#pragma endregion "FPGA"

#pragma region "RTcontrol"
RTcontrol::Pixelclock::Pixelclock(const int widthPerFrame_pix, const double dwell) :
	mWidthPerFrame_pix{ widthPerFrame_pix },
	mDwell{ dwell }
{
	pushUniformDwellTimes_();
}

QU32 RTcontrol::Pixelclock::readPixelclock() const
{
	return mPixelclockQ;
}

RTcontrol::RTcontrol(const FPGA &fpga, const LINECLOCK lineclockInput, const MAINTRIG mainTrigger, const FIFOOUTfpga enableFIFOOUTfpga, const int widthPerFrame_pix, const int heightPerBeamletPerFrame_pix, const int nFrames) :
	mVec_queue{ static_cast<U8>(RTCHAN::NCHAN) },	//Initialize the size the vector containing the queues (= # of queues)
	mFpga{ fpga },
	mLineclockInput{ lineclockInput },
	mMainTrigger{ mainTrigger },
	mEnableFIFOOUTfpga{ enableFIFOOUTfpga },
	mPMT16Xchan{ determineRescannerSetpoint_() },
	mWidthPerFrame_pix{ widthPerFrame_pix },
	mHeightPerBeamletPerFrame_pix{ heightPerBeamletPerFrame_pix },
	mNframes{ nFrames },
	mHeightPerBeamletAllFrames_pix{ mHeightPerBeamletPerFrame_pix * mNframes },
	mNpixPerBeamletAllFrames{ mWidthPerFrame_pix * mHeightPerBeamletAllFrames_pix }
{
	mFpga.uploadImagingParameters(mHeightPerBeamletAllFrames_pix, mHeightPerBeamletPerFrame_pix, mNframes);

	mFpga.setLineclock(mLineclockInput);

	mBufferA = new U32[mNpixPerBeamletAllFrames];
	mBufferB = new U32[mNpixPerBeamletAllFrames];

	//Generate a pixelclock
	const Pixelclock pixelclock(mWidthPerFrame_pix, g_pixelDwellTime);
	mVec_queue.at(static_cast<U8>(RTCHAN::PIXELCLOCK)) = pixelclock.readPixelclock();
}

//This constructor is meant to be used with RTcontrol::setNumberOfFrames(const int nFrames)
RTcontrol::RTcontrol(const FPGA &fpga, const LINECLOCK lineclockInput, const MAINTRIG mainTrigger, const FIFOOUTfpga enableFIFOOUTfpga, const int widthPerFrame_pix, const int heightPerBeamletPerFrame_pix):
	mVec_queue{ static_cast<U8>(RTCHAN::NCHAN) },
	mFpga{ fpga },
	mLineclockInput{ lineclockInput },
	mMainTrigger{ mainTrigger },
	mEnableFIFOOUTfpga{ enableFIFOOUTfpga },
	mPMT16Xchan{ determineRescannerSetpoint_() },
	mWidthPerFrame_pix{ widthPerFrame_pix },
	mHeightPerBeamletPerFrame_pix{ heightPerBeamletPerFrame_pix }
{}

//Set the imaging parameters
void RTcontrol::setNumberOfFrames(const int nFrames)
{
	mNframes = nFrames;
	mHeightPerBeamletAllFrames_pix = mHeightPerBeamletPerFrame_pix * mNframes;
	mNpixPerBeamletAllFrames = mWidthPerFrame_pix * mHeightPerBeamletAllFrames_pix;
	mFpga.uploadImagingParameters(mHeightPerBeamletAllFrames_pix, mHeightPerBeamletPerFrame_pix, mNframes);

	mFpga.setLineclock(mLineclockInput);

	delete[] mBufferA;
	delete[] mBufferB;
	U32* newBufferA = new U32[mNpixPerBeamletAllFrames];
	U32* newBufferB = new U32[mNpixPerBeamletAllFrames];
	mBufferA = newBufferA;
	mBufferB = newBufferB;

	//Generate a pixelclock
	const Pixelclock pixelclock(mWidthPerFrame_pix, g_pixelDwellTime);
	mVec_queue.at(static_cast<U8>(RTCHAN::PIXELCLOCK)) = pixelclock.readPixelclock();
}

RTcontrol::~RTcontrol()
{
	//std::cout << "RTcontrol destructor called\n"; //For debugging

	//Before I implemented StopFIFOOUTpc_, the computer crashed every time the code was executed immediately after an exception.
	//I think this is because FIFOOUTpc used to remain open and clashed with the subsequent call
	mFpga.stopFIFOOUTpc();

	delete[] mBufferA;
	delete[] mBufferB;
}

void RTcontrol::pushQueue(const RTCHAN chan, QU32& queue)
{
	concatenateQueues_(mVec_queue.at(static_cast<U8>(chan)), queue);
}

void RTcontrol::clearQueue(const RTCHAN chan)
{
	mVec_queue.at(static_cast<U8>(chan)).clear();
}

void RTcontrol::pushDigitalSinglet(const RTCHAN chan, double timeStep, const bool DO)
{
	mVec_queue.at(static_cast<U8>(chan)).push_back(FPGAfunc::packDigitalSinglet(timeStep, DO));
}

void RTcontrol::pushAnalogSinglet(const RTCHAN chan, double timeStep, const double AO, const OVERRIDE override)
{
	if (timeStep < g_tMinAO)
	{
		std::cerr << "WARNING in " << __FUNCTION__ << ": Time step too small. Time step cast to " << g_tMinAO / us << " us\n";
		timeStep = g_tMinAO;
	}

	//Clear the current content
	if (override == OVERRIDE::EN)
		mVec_queue.at(static_cast<U8>(chan)).clear();

	mVec_queue.at(static_cast<U8>(chan)).push_back(FPGAfunc::packAnalogSinglet(timeStep, AO));
}

//Push a fixed-point number. For scaling the pockels cell output
void RTcontrol::pushAnalogSingletFx2p14(const RTCHAN chan, const double scalingFactor)
{
	mVec_queue.at(static_cast<U8>(chan)).push_back(static_cast<U32>(doubleToFx2p14(scalingFactor)));
}

void RTcontrol::pushLinearRamp(const RTCHAN chan, double timeStep, const double rampLength, const double Vi, const double Vf, const OVERRIDE override)
{
	//Clear the current content
	if (override == OVERRIDE::EN)
		mVec_queue.at(static_cast<U8>(chan)).clear();

	FPGAfunc::linearRamp(mVec_queue.at(static_cast<U8>(chan)), timeStep, rampLength, Vi, Vf);
}

//Ramp up or down the AO for the scanner and rescanner from the current voltage to the first value of the control sequence in mVec_queue to avoid jumps at the start of the sequence
void RTcontrol::presetScannerPosition() const
{
	//Read the current voltage of the AOs for the scanner and rescanner. See the LV implementation
	std::vector<I16> AOlastVoltage_I16(static_cast<U8>(RTCHAN::NCHAN), 0);
	FPGAfunc::checkStatus(__FUNCTION__, NiFpga_ReadI16(mFpga.handle(), NiFpga_FPGAvi_IndicatorU16_ScanGalvoMon, &AOlastVoltage_I16.at(static_cast<U8>(RTCHAN::SCANGALVO))));
	FPGAfunc::checkStatus(__FUNCTION__, NiFpga_ReadI16(mFpga.handle(), NiFpga_FPGAvi_IndicatorU16_RescanGalvoMon, &AOlastVoltage_I16.at(static_cast<U8>(RTCHAN::RESCANGALVO))));

	//Create a vector of queues for the ramps
	VQU32 vec_queue{ static_cast<U8>(RTCHAN::NCHAN) };
	for (int iterChan = 1; iterChan < static_cast<U8>(RTCHAN::NCHAN); iterChan++) //the pixelclock (chan = 0) is kept empty
	{
		if (mVec_queue.at(iterChan).size() != 0)
		{
			//Linear ramp the output to smoothly transition from the end point of the previous run to the start point of the next run
			if ((iterChan == static_cast<U8>(RTCHAN::SCANGALVO) || iterChan == static_cast<U8>(RTCHAN::RESCANGALVO)))	//Only do GALVO1 and GALVO2 for now
			{
				const double Vi = FPGAfunc::intToVoltage(AOlastVoltage_I16.at(iterChan));								//Current voltage of the AO outputs
				const double Vf = FPGAfunc::intToVoltage(static_cast<I16>(mVec_queue.at(iterChan).front()));			//First element of the new control sequence

				FPGAfunc::linearRamp(vec_queue.at(iterChan), 10 * us, 5 * ms, Vi, Vf);
				//For debugging
				//std::cout << Vi << "\n";
				//std::cout << Vf << "\n";
			}
		}
	}
	uploadFIFOIN_(vec_queue);	//Upload the initialization ramp to the FPGA
	mFpga.triggerAOext();		//Trigger the initialization ramp externally (not using the internal clocks)
}

//Upload the main control sequence to the FPGA
void RTcontrol::uploadControlSequence() const
{
	uploadFIFOIN_(mVec_queue);		
}

//Trigger the ctl&acq sequence
void RTcontrol::trigger() const
{
	FPGAfunc::checkStatus(__FUNCTION__, NiFpga_WriteBool(mFpga.handle(), NiFpga_FPGAvi_ControlBool_PcTrigger, true));
	FPGAfunc::checkStatus(__FUNCTION__, NiFpga_WriteBool(mFpga.handle(), NiFpga_FPGAvi_ControlBool_PcTrigger, false));
}

//Preset the parameters for the acquisition sequence
void RTcontrol::initialize(const SCANDIR stackScanDir)
{
	mFpga.enableFIFOOUTfpga(mEnableFIFOOUTfpga);	//Push data from the FPGA to FIFOOUTfpga. It is disabled when debugging
	mFpga.setPostSequenceTimer(mMainTrigger);

	iniStageContScan_(stackScanDir);				//Set the delay of the stage triggering the ctl&acq and specify the stack-saving order
	presetScannerPosition();						//Preset the scanner positions
	Sleep(10);										//Give the FPGA enough time to settle (> 5 ms) to avoid presetScannerPosition() clashing with the subsequent call of uploadControlSequence()
													//(I realized this after running VS in release-mode, which connects faster to the FPGA than in debug-mode)
	uploadControlSequence();						//Upload the control sequence to the FPGA

	mFpga.startFIFOOUTpc();							//Establish connection between FIFOOUTpc and FIFOOUTfpga to send the control sequence to the FGPA. Optional according to NI, but if not called, sometimes garbage is generated
	mFpga.collectFIFOOUTpcGarbage_();				//Clean up any residual data from the previous run
	mFpga.enableStageTrigAcq(mMainTrigger);			//Enable the stage triggering the ctl&acq sequence
	//Sleep(20);									//When continuous scanning, collectFIFOOUTpcGarbage() is being called late. Maybe this will fix it
}
	
//Scan a single frame
void RTcontrol::run()
{
	initialize();		//Preset the parameters for the acquisition sequence
	trigger();			//Trigger the ctl&acq sequence. If triggered too early, FIFOOUTfpga will probably overflow
	downloadData();		//Retrieve the data from the FPGA
}

//Retrieve the data from the FPGA
void RTcontrol::downloadData()
{
	if (mEnableFIFOOUTfpga == FIFOOUTfpga::EN)
	{
		try
		{
			readFIFOOUTpc_();			//Read the data received in FIFOOUTpc
		}
		catch (const ImageException &e)
		{
			std::cerr << "An ImageException has occurred in: " << e.what() << "\n";
			//throw;//Do not terminate the entire sequence. Notify the exception and continue with the next iteration
		}
	}
	correctInterleaved_();				//The RS scans bi-directionally. The pixel order has to be reversed either for the odd or even lines
										//In case of pipelining, remove it from RTcontrol::initialize() and call it separately

	mFpga.disableStageTrigAcq();		//Disable the stage triggering the ctl&acq sequence to allow positioning the stage after acquisition
}

U32* RTcontrol::dataBufferA() const
{
	return mBufferA;
}

U32* RTcontrol::dataBufferB() const
{
	return mBufferB;
}

//The pixel clock is triggered by the line clock (see the LV implementation) after an initial waiting time
void RTcontrol::Pixelclock::pushUniformDwellTimes_()
{
	//The pixel clock is triggered by the line clock (see the LV implementation), followed by a waiting time 'InitialWaitingTime'. At 160MHz, the clock increment is 6.25ns = 0.00625us
	//For example, for a dwell time = 125ns and 400 pixels, the initial waiting time is (g_lineclockHalfPeriod-400*125ns)/2

	const double initialWaitingTime{ (g_lineclockHalfPeriod - mWidthPerFrame_pix * mDwell) / 2 }; //Relative delay of the pixel clock wrt the line clock

	//Check if the pixelclock overflows the Lineclock
	if (initialWaitingTime <= 0)
		throw std::invalid_argument((std::string)__FUNCTION__ + ": Pixelclock overflow");

	mPixelclockQ.push_back(FPGAfunc::packU32(FPGAfunc::timeToTick(initialWaitingTime) + mCalibFine_tick - mLatency_tick, 0));	 //DO NOT use packDigitalSinglet because the pixelclock has a different latency from DO

	//Generate the pixel clock. When HIGH is pushed, the pixel clock switches its state, which corresponds to a pixel delimiter (boolean switching is implemented on the FPGA)
	//Npixels+1 because there is one more pixel delimiter than number of pixels. The last time step is irrelevant
	for (int pix = 0; pix < mWidthPerFrame_pix + 1; pix++)
		mPixelclockQ.push_back(FPGAfunc::packPixelclockSinglet(mDwell, 1));
}

//Push all the elements in 'tailQ' into 'headQ'
void RTcontrol::concatenateQueues_(QU32& receivingQueue, QU32& givingQueue) const
{
	while (!givingQueue.empty())
	{
		receivingQueue.push_back(givingQueue.front());
		givingQueue.pop_front();
	}
}

//Load the imaging parameters onto the FPGA
void FPGA::uploadImagingParameters(const int heightPerBeamletAllFrames_pix, const int heightPerBeamletPerFrame_pix, const int nFrames) const
{
	if (heightPerBeamletAllFrames_pix <= 0 || heightPerBeamletPerFrame_pix <= 0 || nFrames <= 0 )
		throw std::invalid_argument((std::string)__FUNCTION__ + ": One or more imaging parameters take negative values");

	//IMAGING PARAMETERS
	FPGAfunc::checkStatus(__FUNCTION__, NiFpga_WriteI32(mHandle, NiFpga_FPGAvi_ControlI32_NlinesAll, static_cast<I32>(heightPerBeamletAllFrames_pix)));		//Total number of lines per beamlet in all the frames
	FPGAfunc::checkStatus(__FUNCTION__, NiFpga_WriteU16(mHandle, NiFpga_FPGAvi_ControlI16_NlinesPerFrame, static_cast<I16>(heightPerBeamletPerFrame_pix)));	//Number of lines per beamlet in a frame
	FPGAfunc::checkStatus(__FUNCTION__, NiFpga_WriteI16(mHandle, NiFpga_FPGAvi_ControlI16_Nframes, static_cast<I16>(nFrames)));								//Number of frames to acquire
}

//Send every single queue in 'vec_queue' to the FPGA buffer
//For this, concatenate all the individual queues 'vec_queue.at(ii)' in the queue 'allQueues'.
//The data structure is allQueues = [# elements ch1| elements ch1 | # elements ch 2 | elements ch 2 | etc]. THE QUEUE POSITION DETERMINES THE TARGETED CHANNEL
//Then transfer all the elements in 'allQueues' to the vector FIFOIN to interface the FPGA
void RTcontrol::uploadFIFOIN_(const VQU32 &queue_vec) const
{
	{
		QU32 allQueues;		//Create a single long queue
		for (int chan = 0; chan < static_cast<U8>(RTCHAN::NCHAN); chan++)
		{
			allQueues.push_back(queue_vec.at(chan).size());					//Push the number of elements in each individual queue ii, 'vec_queue.at(ii)'	
			for (std::vector<int>::size_type iter = 0; iter != queue_vec.at(chan).size(); iter++)
				allQueues.push_back(queue_vec.at(chan).at(iter));			//Push vec_queue[i]
		}

		const int sizeFIFOINqueue{ static_cast<int>(allQueues.size()) };	//Total number of elements in all the queues 

		if (sizeFIFOINqueue > g_FIFOINmax)
			throw std::overflow_error((std::string)__FUNCTION__ + ": FIFOIN overflow");

		std::vector<U32> FIFOIN(sizeFIFOINqueue);							//Create a 1D array with the channels concatenated
		for (int ii = 0; ii < sizeFIFOINqueue; ii++)
		{
			FIFOIN.at(ii) = allQueues.front();								//Transfer the queue elements to the array
			allQueues.pop_front();
		}
		allQueues = {};					//Cleanup the queue C++11 style

		U32 r;							//Elements remaining

		//Send the data to the FPGA through FIFOIN. I measured a minimum time of 10 ms to execute
		FPGAfunc::checkStatus(__FUNCTION__, NiFpga_WriteFifoU32(mFpga.handle(), NiFpga_FPGAvi_HostToTargetFifoU32_FIFOIN, &FIFOIN[0], sizeFIFOINqueue, NiFpga_InfiniteTimeout, &r));

		//On the FPGA, transfer the commands from FIFOIN to the sub-channel buffers. 
		//This boolean serves as the master trigger for the entire control sequence
		FPGAfunc::checkStatus(__FUNCTION__, NiFpga_WriteBool(mFpga.handle(), NiFpga_FPGAvi_ControlBool_FIFOINtrigger, true));
		FPGAfunc::checkStatus(__FUNCTION__, NiFpga_WriteBool(mFpga.handle(), NiFpga_FPGAvi_ControlBool_FIFOINtrigger, false));
	}
}

//Determine the setpoint for the rescanner. mPMT16Xchan is called by the classes Galvo and Image
RTcontrol::PMT16XCHAN RTcontrol::determineRescannerSetpoint_()
{
	if (multibeam)
		return PMT16XCHAN::CENTERED;
	else
		return static_cast<RTcontrol::PMT16XCHAN>(g_PMT16Xchan_int);
}

void RTcontrol::iniStageContScan_(const SCANDIR stackScanDir)
{
	mScanDir = stackScanDir;															//Initialize mScanDir to set the stage-trigger delay and stack-saving order
	mFpga.setStageTrigAcqDelay(mMainTrigger, mHeightPerBeamletPerFrame_pix, mScanDir);	//Set the delay for the stage triggering the ctl&acq sequence
}

//Read the data in FIFOOUTpc
void RTcontrol::readFIFOOUTpc_()
{
	//TODO: save the data concurrently
	//I ran a test and found that two 32-bit FIFOOUTfpga have a larger bandwidth than a single 64 - bit FIFOOUTfpga
	//Test if the bandwidth can be increased by using 'NiFpga_AcquireFifoReadElementsU32'.Ref: http://zone.ni.com/reference/en-XX/help/372928G-01/capi/functions_fifo_read_acquire/
	//pass an array to a function: https://stackoverflow.com/questions/2838038/c-programming-malloc-inside-another-function
	//review of pointers and references in C++: https://www.ntu.edu.sg/home/ehchua/programming/cpp/cp4_PointerReference.html

	/*
	//Declare and start a stopwatch [2]
	double duration;
	auto t_start{ std::chrono::high_resolution_clock::now() };
	*/

	const int readFifoWaitingTime_ms{ 5 };				//Waiting time between each iteration
	int timeout_iter{ 200 };							//Timeout the whileloop if the data transfer fails
	int nullReadCounterA{ 0 }, nullReadCounterB{ 0 };	//Null reading counters

	int nElemTotalA{ 0 }, nElemTotalB{ 0 }; 			//Total number of elements read from FIFOOUTpc A and B
	while (nElemTotalA < mNpixPerBeamletAllFrames || nElemTotalB < mNpixPerBeamletAllFrames)
	{
		Sleep(readFifoWaitingTime_ms); //Wait till collecting big chuncks of data. Adjust the waiting time for max transfer bandwidth

		readChunk_(nElemTotalA, NiFpga_FPGAvi_TargetToHostFifoU32_FIFOOUTa, mBufferA, nullReadCounterA);	//FIFOOUTpc A
		readChunk_(nElemTotalB, NiFpga_FPGAvi_TargetToHostFifoU32_FIFOOUTb, mBufferB, nullReadCounterB);	//FIFOOUTpc B

		if (nullReadCounterA > timeout_iter && nullReadCounterB > timeout_iter)
			throw ImageException((std::string)__FUNCTION__ + ": FIFO null-reading timeout");

		//std::cout << "FIFO A: " << nElemTotalA << "\tFIFO B: " << nElemTotalB << "\n";	//For debugging
		//std::cout << "nullReadCounter A: " << nullReadCounterA << "\tnullReadCounter: " << nullReadCounterB << "\n";	//For debugging
	}

	/*
	//Stop the stopwatch
	duration = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - t_start).count();
	std::cout << "Elapsed time: " << duration << " ms" << "\n";
	std::cout << "FIFOOUT bandwidth: " << 2 * 32 * mRTcontrol.mNpixAllFrames / duration / 1000 << " Mbps" << "\n"; //2 FIFOOUTs of 32 bits each
	std::cout << "Total of elements read: " << nElemTotalA << "\t" << nElemTotalB << "\n"; //Print out the total number of elements read
	*/

	//If all the expected data is NOT read successfully
	if (nElemTotalA < mNpixPerBeamletAllFrames || nElemTotalB < mNpixPerBeamletAllFrames)
		throw ImageException((std::string)__FUNCTION__ + ": Received less FIFO elements than expected");
}

//Read a chunk of data in the FIFOpc
void RTcontrol::readChunk_(int &nElemRead, const NiFpga_FPGAvi_TargetToHostFifoU32 FIFOOUTpc, U32* buffer, int &nullReadCounter)
{
	U32 dummy;
	U32 nElemToRead{ 0 };				//Elements remaining in FIFOOUTpc
	const U32 timeout_ms{ 100 };		//FIFOOUTpc timeout

	if (nElemRead < mNpixPerBeamletAllFrames)		//Skip if all the data have already been transferred
	{
		//By requesting 0 elements from FIFOOUTpc, the function returns the number of elements available. If no data is available, nElemToRead = 0 is returned
		FPGAfunc::checkStatus(__FUNCTION__, NiFpga_ReadFifoU32(mFpga.handle(), FIFOOUTpc, buffer, 0, timeout_ms, &nElemToRead));
		//std::cout << "Number of elements remaining in FIFOOUT: " << nElemToRead << "\n";	//For debugging

		//If data available in FIFOOUTpc, retrieve it
		if (nElemToRead > 0)
		{
			//If more data than expected
			if (static_cast<int>(nElemRead + nElemToRead) > mNpixPerBeamletAllFrames)
				throw std::runtime_error((std::string)__FUNCTION__ + ": Received more FIFO elements than expected");

			//Retrieve the elements in FIFOOUTpc
			FPGAfunc::checkStatus(__FUNCTION__, NiFpga_ReadFifoU32(mFpga.handle(), FIFOOUTpc, buffer + nElemRead, nElemToRead, timeout_ms, &dummy));

			//Keep track of the total number of elements read
			nElemRead += nElemToRead;

			nullReadCounter = 0;	//Reset the iteration counter
		}
		else
			nullReadCounter++;		//keep track of the null reads
	}
}

//The RS scans bi-directionally. The pixel order has to be reversed either for the odd or even lines. Currently I reverse the EVEN lines so that the resulting image matches the orientation of the sample
//For cleaner coding, do not move this function to the Image class since it modify a member of the RTcontrol class
void RTcontrol::correctInterleaved_()
{
	//std::reverse(mBufferA + lineIndex * mRTcontrol.mWidthPerFrame_pix, mBufferA + (lineIndex + 1) * mRTcontrol.mWidthPerFrame_pix)
	//reverses all the pixels between and including the indices 'lineIndex * widthPerFrame_pix' and '(lineIndex + 1) * widthPerFrame_pix - 1'
	for (int lineIndex = 0; lineIndex < mHeightPerBeamletAllFrames_pix; lineIndex += 2)
	{
		std::reverse(mBufferA + lineIndex * mWidthPerFrame_pix, mBufferA + (lineIndex + 1) * mWidthPerFrame_pix);
		std::reverse(mBufferB + lineIndex * mWidthPerFrame_pix, mBufferB + (lineIndex + 1) * mWidthPerFrame_pix);
	}
}

//When the Z stage acts as the main trigger (for cont z scanning), the motion monitor of the Z stage bounces and therefore false-triggers new acquisitions
//Solution: after an acq sequence, wait a certain amount of time before the acq is triggered again (timer implemented in LV)
void FPGA::setPostSequenceTimer(const MAINTRIG mainTrigger) const
{
	double postSequenceTimer{ 0 };
	switch (mainTrigger)
	{
	case MAINTRIG::STAGEZ:
		postSequenceTimer = g_postSequenceTimer;
		//std::cout << "Z stage as the main trigger\n";
		break;
	case MAINTRIG::STAGEX:
		postSequenceTimer = g_postSequenceTimer;
		break;
	default:
		postSequenceTimer = 0;
	}
	FPGAfunc::checkStatus(__FUNCTION__, NiFpga_WriteU32(mHandle, NiFpga_FPGAvi_ControlU32_PostsequenceTimer_tick, static_cast<U32>(postSequenceTimer / us * g_tickPerUs)));
}
#pragma endregion "RTcontrol"


/* Functions for generating a non-uniform pixel clock

extern const double RSpkpk_um = 250 * um;					//Peak-to-peak amplitude of the resonant scanner. Needed for generating a non-uniform pixelclock

//Convert the spatial coordinate of the resonant scanner to time. x in [-RSpkpk_um/2, RSpkpk_um/2]
double RTcontrol::Pixelclock::convertSpatialCoordToTime_us(const double x) const
{
double arg = 2 * x / RSpkpk_um;
if (arg > 1)
throw std::invalid_argument((std::string)__FUNCTION__ + ": The argument of asin must be <=1");
else
return halfPeriodLineclock / us * asin(arg) / Constants::PI; //The returned value is in the range [-halfPeriodLineclock / us/PI, halfPeriodLineclock / us/PI]
}

//Discretize the spatial coordinate, then convert it to time
double RTcontrol::Pixelclock::getDiscreteTime_us(const int pix) const
{
const double dx = 0.5 * um;
return convertSpatialCoordToTime_us(dx * pix);
}

//Calculate the dwell time for the pixel
double RTcontrol::Pixelclock::calculateDwellTime_us(const int pix) const
{
return getDiscreteTime_us(pix + 1) - getDiscreteTime_us(pix);
}

//Calculate the practical dwell time of each pixel, considering that the FPGA has discrete time steps
double RTcontrol::Pixelclock::calculatePracticalDwellTime_us(const int pix) const
{
return round(calculateDwellTime_us(pix) * g_tickPerUs) / g_tickPerUs;		// 1/g_tickPerUs is the time step of the FPGA clock (microseconds per tick)
}


//Pixelclock with equal pixel size (spatial).
void RTcontrol::Pixelclock::pushCorrectedDwellTimes()
{

const int calibCoarse_tick = 2043;	//calibCoarse_tick: Look at the oscilloscope and adjust to center the pixel clock within a line scan
const int calibFine_tick = 10;

if (widthPerFrame_pix % 2 != 0)		//Throw exception if odd number of pixels (not supported yet)
throw std::invalid_argument((std::string)__FUNCTION__ + ": Odd number of pixels for the image width currently not supported");

//Relative delay of the pixel clock with respect to the line clock. DO NOT use packDigitalSinglet because the pixelclock has a different latency from DO
const U16 InitialWaitingTime_tick = static_cast<U16>(calibCoarse_tick + calibFine_tick);
mPixelclockQ.push_back(FPGAfunc::packU32(InitialWaitingTime_tick - mLatency_tick, 0));

//Generate the pixel clock. When HIGH is pushed, the pixel clock switches its state, which corresponds to a pixel delimiter (boolean switching is implemented on the FPGA)
for (int pix = -widthPerFrame_pix / 2; pix < widthPerFrame_pix / 2; pix++)
mPixelclockQ.push_back(FPGAfunc::packPixelclockSinglet(calculatePracticalDwellTime_us(pix), 1));

//Npixels+1 because there is one more pixel delimiter than number of pixels. The last time step is irrelevant
mPixelclockQ.push_back(FPGAfunc::packPixelclockSinglet(tMIN_us, 1));
}

*/
#include "FPGAapi.h"

namespace FPGAfunctions {
	//Convert microseconds to ticks
	U16 convertUs2tick(const double t_us)
	{
		const double t_tick = t_us * tickPerUs;

		if ((U32)t_tick > 0x0000FFFF)
		{
			std::cerr << "WARNING in " << __FUNCTION__ << ": Time step overflow. Time step cast to the max: " << std::fixed << _UI16_MAX * usPerTick << " us" << std::endl;
			return _UI16_MAX;
		}
		else if ((U32)t_tick < t_tick_MIN)
		{
			std::cerr << "WARNING in " << __FUNCTION__ << ": Time step underflow. Time step cast to the min: " << std::fixed << t_tick_MIN * usPerTick << " us" << std::endl;;
			return t_tick_MIN;
		}
		else
			return (U16)t_tick;
	}

	//Convert voltage to I16 [1]
	I16 convertVolt2I16(const double voltage_V)
	{
		const int VMAX = 10 * V;
		const int VMIN = -10 * V;

		if (voltage_V > 10)
		{
			std::cerr << "WARNING in " << __FUNCTION__ << ": Voltage overflow. Voltage cast to the max: " + std::to_string(VMAX) + " V" << std::endl;
			return (I16)_I16_MAX;
		}
		else if (voltage_V < -10)
		{
			std::cerr << "WARNING in " << __FUNCTION__ << ": Voltage underflow. Voltage cast to the min: " + std::to_string(VMIN) + " V" << std::endl;
			return (I16)_I16_MIN;
		}
		else
			return (I16)(voltage_V / 10 * _I16_MAX);
	}

	//Pack t in MSB and x in LSB. Time t and analog output x are encoded in 16 bits each.
	U32 packU32(const U16 t_tick, const U16 val)
	{
		return (t_tick << 16) | (0x0000FFFF & val);
	}

	//Send out an analog instruction, where the analog level 'val' is held for the amount of time 't_us'
	U32 packAnalogSinglet(const double t_us, const double val)
	{
		const U16 AOlatency_tick = 2;	//To calibrate, run AnalogLatencyCalib(). I think the latency comes from the memory block, which takes 2 cycles to read
		return packU32(convertUs2tick(t_us) - AOlatency_tick, convertVolt2I16(val));
	}


	//Send out a single digital instruction, where 'DO' is held LOW or HIGH for the amount of time 't_us'. The DOs in Connector1 are rated at 10MHz, Connector0 at 80MHz.
	U32 packDigitalSinglet(const double t_us, const bool DO)
	{
		const U16 DOlatency_tick = 2;	//To calibrate, run DigitalLatencyCalib(). I think the latency comes from the memory block, which takes 2 cycles to read
		return packU32(convertUs2tick(t_us) - DOlatency_tick, (U16)DO);
	}

	//Generate a single pixel-clock instruction, where 'DO' is held LOW or HIGH for the amount of time 't_us'
	U32 packPixelclockSinglet(const double t_us, const bool DO)
	{
		const U16 PixelclockLatency_tick = 1;//The pixel-clock is implemented using a SCTL. I think the latency comes from reading the LUT buffer
		return packU32(convertUs2tick(t_us) - PixelclockLatency_tick, (U16)DO);

	}
}//namespace


FPGAapi::FPGAapi()
{
	//Must be called before any other FPGA calls
	checkFPGAstatus(__FUNCTION__, NiFpga_Initialize());

	//Opens a session, downloads the bitstream. 1=no run, 0=run
	checkFPGAstatus(__FUNCTION__, NiFpga_Open(Bitfile, NiFpga_FPGAvi_Signature, "RIO0", 0, &mSession));
}

FPGAapi::~FPGAapi()
{
	//std::cout << "FPGAapi destructor was called" << std::endl;
};

void FPGAapi::initialize() const
{
	//Initialize the FPGA variables. See 'Const.cpp' for the definition of each variable
	checkFPGAstatus(__FUNCTION__, NiFpga_WriteU8(mSession, NiFpga_FPGAvi_ControlU8_PhotoncounterInputSelector, photoncounterInput));				//Debugger. Use the PMT-pulse simulator as the input of the photon-counter
	checkFPGAstatus(__FUNCTION__, NiFpga_WriteU8(mSession, NiFpga_FPGAvi_ControlU8_LineclockInputSelector, lineclockInput));						//Select the Line clock: resonant scanner or function generator
	checkFPGAstatus(__FUNCTION__, NiFpga_WriteBool(mSession, NiFpga_FPGAvi_ControlBool_FIFOINtrigger, 0));											//control-sequence trigger
	checkFPGAstatus(__FUNCTION__, NiFpga_WriteBool(mSession, NiFpga_FPGAvi_ControlBool_LinegateTrigger, 0));										//data-acquisition trigger
	checkFPGAstatus(__FUNCTION__, NiFpga_WriteBool(mSession, NiFpga_FPGAvi_ControlBool_FlushTrigger, 0));
	checkFPGAstatus(__FUNCTION__, NiFpga_WriteU16(mSession, NiFpga_FPGAvi_ControlU16_FIFOtimeout, (U16)FIFOtimeout_tick));
	checkFPGAstatus(__FUNCTION__, NiFpga_WriteU16(mSession, NiFpga_FPGAvi_ControlU16_Nchannels, (U16)Nchan));
	checkFPGAstatus(__FUNCTION__, NiFpga_WriteU16(mSession, NiFpga_FPGAvi_ControlU16_SyncDOtoAO, (U16)syncDOtoAO_tick));
	checkFPGAstatus(__FUNCTION__, NiFpga_WriteU16(mSession, NiFpga_FPGAvi_ControlU16_SyncAODOtoLinegate, (U16)syncAODOtoLinegate_tick));
	checkFPGAstatus(__FUNCTION__, NiFpga_WriteU16(mSession, NiFpga_FPGAvi_ControlU16_NlinesAll, (U16)(nLinesAllFrames + nFrames * nLinesSkip)));		//Total number of lines in all the frames, including the skipped lines
	checkFPGAstatus(__FUNCTION__, NiFpga_WriteU16(mSession, NiFpga_FPGAvi_ControlU16_NlinesPerFrame, (U16)heightPerFrame_pix));							//Number of lines in a frame, without including the skipped lines
	checkFPGAstatus(__FUNCTION__, NiFpga_WriteU16(mSession, NiFpga_FPGAvi_ControlU16_NlinesPerFramePlusSkips, (U16)(heightPerFrame_pix + nLinesSkip)));	//Number of lines in a frame including the skipped lines

	//Shutters
	//checkFPGAstatus(__FUNCTION__,  NiFpga_WriteBool(mSession, NiFpga_FPGAvi_ControlBool_Shutter1, 0));
	//checkFPGAstatus(__FUNCTION__,  NiFpga_WriteBool(mSession, NiFpga_FPGAvi_ControlBool_Shutter2, 0));

	//Vibratome control
	checkFPGAstatus(__FUNCTION__, NiFpga_WriteBool(mSession, NiFpga_FPGAvi_ControlBool_VT_start, 0));
	checkFPGAstatus(__FUNCTION__, NiFpga_WriteBool(mSession, NiFpga_FPGAvi_ControlBool_VT_back, 0));
	checkFPGAstatus(__FUNCTION__, NiFpga_WriteBool(mSession, NiFpga_FPGAvi_ControlBool_VT_forward, 0));
	checkFPGAstatus(__FUNCTION__, NiFpga_WriteBool(mSession, NiFpga_FPGAvi_ControlBool_VT_NC, 0));

	//Resonant scanner
	//checkFPGAstatus(__FUNCTION__,  NiFpga_WriteI16(mSession, NiFpga_FPGAvi_ControlI16_RS_voltage, 0));	//Output voltage
	//checkFPGAstatus(__FUNCTION__,  NiFpga_WriteBool(mSession, NiFpga_FPGAvi_ControlBool_RS_ON_OFF, 0));	//Turn on/off

	//PockelsID cells
	checkFPGAstatus(__FUNCTION__,  NiFpga_WriteI16(mSession, NiFpga_FPGAvi_ControlI16_PC1_voltage, 0));
	checkFPGAstatus(__FUNCTION__, NiFpga_WriteBool(mSession, NiFpga_FPGAvi_ControlBool_PC1_selectTrigger, 0));
	checkFPGAstatus(__FUNCTION__, NiFpga_WriteBool(mSession, NiFpga_FPGAvi_ControlBool_PC1_manualOn, 0));

	//Debugger
	checkFPGAstatus(__FUNCTION__, NiFpga_WriteArrayBool(mSession, NiFpga_FPGAvi_ControlArrayBool_Pulsesequence, pulseArray, nPulses));
	checkFPGAstatus(__FUNCTION__, NiFpga_WriteBool(mSession, NiFpga_FPGAvi_ControlBool_FIFOOUTdebug, 0));	//FIFO OUT
}

//Send every single queue in VectorOfQueue to the FPGA buffer
//For this, concatenate all the single queues in a single long queue. THE QUEUE POSITION DETERMINES THE TARGETED CHANNEL	
//Then transfer the elements in the long queue to an array to interface the FPGA
//Improvement: the single queues VectorOfQueues[i] could be transferred directly to the FIFO array
void FPGAapi::writeFIFO(VQU32 &vectorQueues) const
{
	QU32 allQueues;											//Create a single long queue
	for (int i = 0; i < Nchan; i++)
	{
		allQueues.push_back(vectorQueues[i].size());		//Push the number of elements in VectorOfQueues[i] (individual queue)

		//New version: Non-destructive. Randomly access the elements in VectorOfQueues[i] and push them to allQueues
		for (size_t iter = 0; iter < vectorQueues[i].size(); iter++)
			allQueues.push_back(vectorQueues[i].at(iter));

		/*Old version. Destructive
		while (!vectorQueues[i].empty())
		{
			allQueues.push_back(vectorQueues[i].front());	//Push all the elements in VectorOfQueues[i] to allQueues
			vectorQueues[i].pop_front();
		}
		*/
	}

	const int sizeFIFOqueue = allQueues.size();		//Total number of elements in all the queues 

	if (sizeFIFOqueue > FIFOINmax)
		throw std::overflow_error((std::string)__FUNCTION__ + ": FIFO IN overflow");

	U32* FIFO = new U32[sizeFIFOqueue];				//Create an array for interfacing the FPGA	
	for (int i = 0; i < sizeFIFOqueue; i++)
	{
		FIFO[i] = allQueues.front();				//Transfer the queue elements to the array
		allQueues.pop_front();
	}
	allQueues = {};									//Cleanup the queue (C++11 style)

	const U32 timeout_ms = -1;		// in ms. A value -1 prevents the FIFO from timing out
	U32 r;						//empty elements remaining

	checkFPGAstatus(__FUNCTION__, NiFpga_WriteFifoU32(mSession, NiFpga_FPGAvi_HostToTargetFifoU32_FIFOIN, FIFO, sizeFIFOqueue, timeout_ms, &r)); //Send the data to the FPGA through the FIFO

	delete[] FIFO;		//cleanup the array
}

//Execute the commands
void FPGAapi::triggerRT() const
{
	checkFPGAstatus(__FUNCTION__, NiFpga_WriteBool(mSession, NiFpga_FPGAvi_ControlBool_LinegateTrigger, 1));
	checkFPGAstatus(__FUNCTION__, NiFpga_WriteBool(mSession, NiFpga_FPGAvi_ControlBool_LinegateTrigger, 0));
}

//Flush the block RAMs used for buffering the pixelclock, AO, and DO 
void FPGAapi::flushBRAMs() const
{
	checkFPGAstatus(__FUNCTION__, NiFpga_WriteBool(mSession, NiFpga_FPGAvi_ControlBool_FlushTrigger, 1));
	checkFPGAstatus(__FUNCTION__, NiFpga_WriteBool(mSession, NiFpga_FPGAvi_ControlBool_FlushTrigger, 0));
	//std::cout << "flushBRAMs called\n";
}

//The FPGAapi object has to be closed explicitly (in opposition to using the destructor) because it lives in main()
void FPGAapi::close(const bool reset) const
{
	//Closes the session to the FPGA. The FPGA resets (Re-downloads the FPGA bitstream to the target, the outputs go to zero)
	//unless either another session is still open or you use the NiFpga_CloseAttribute_NoResetIfLastSession attribute.
	//0 resets, 1 does not reset
	checkFPGAstatus(__FUNCTION__, NiFpga_Close(mSession, (U32)!reset));

	//You must call this function after all other function calls if NiFpga_Initialize succeeds. This function unloads the NiFpga library.
	checkFPGAstatus(__FUNCTION__, NiFpga_Finalize());
}

NiFpga_Session FPGAapi::getSession() const
{
	return mSession;
}

void checkFPGAstatus(char functionName[], NiFpga_Status status)
{
	if (status < 0)
		throw FPGAexception((std::string)functionName + " with FPGA code " + std::to_string(status));
	if (status > 0)
		std::cerr << "A warning has ocurred in " << functionName << " with FPGA code " << status << std::endl;
}





/*COMMENTS

[1]
converts voltage (range: -10V to 10V) to a signed int 16 (range: -32768 to 32767)
0x7FFFF = 0d32767
0xFFFF = -1
0x8000 = -32768



*/

/*
int i;
for(i=0; i<size; i=i+1){
data[i] = i*5000;


for(i=0; i<size; i=i+1){
printf("%i\n",data[i]);
}
getchar();
*/

/*
int16_t val = -32769;
char hex[16];
sprintf(hex, "%x", ((val + (1 << 16)) % (1 << 16)) );
puts(hex);
getchar();*/


/*the AO reads a I16, specifically
0x7FFF = 32767
0xFFFF = -1
0x8000 = -32768*/

/*
printf("%i\n", VOUT(10));
getchar();*/


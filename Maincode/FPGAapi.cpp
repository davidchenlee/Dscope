#include "FPGAapi.h"

namespace GenericFPGAfunctions {

	void printHex(int input)
	{
		std::cout << std::hex << std::uppercase << input << std::nouppercase << std::dec << std::endl;
	}
	//Pack t in MSB and x in LSB. Time t and analog output x are encoded in 16 bits each.
	U32 packU32(U16 t, U16 x)
	{
		return (t << 16) | (0x0000FFFF & x);
	}

	//Convert microseconds to ticks
	U16 convertUs2tick(double t_us)
	{
		const double t_tick = t_us * tickPerUs;

		if ((U32)t_tick > 0x0000FFFF)
		{
			std::cerr << "WARNING in " << __FUNCTION__ << ": time step overflow. Time step set to the max: " << std::fixed << _UI16_MAX * dt_us << " us" << std::endl;
			return _UI16_MAX;
		}
		else if ((U32)t_tick < dt_tick_MIN)
		{
			std::cerr << "WARNING in " << __FUNCTION__ << ": time step underflow. Time step set to the min: " << std::fixed << dt_tick_MIN * dt_us << " us" << std::endl;;
			return dt_tick_MIN;
		}
		else
			return (U16)t_tick;
	}


	/*converts voltage (range: -10V to 10V) to a signed int 16 (range: -32768 to 32767)
	0x7FFFF = 0d32767
	0xFFFF = -1
	0x8000 = -32768
	*/
	I16 convertVolt2I16(double x)
	{
		if (x > 10)
		{
			std::cerr << "WARNING in " << __FUNCTION__ << ": voltage overflow. Voltage set to the max: 10 V" << std::endl;
			return (I16)_I16_MAX;
		}
		else if (x < -10)
		{
			std::cerr << "WARNING in " << __FUNCTION__ << ": voltage underflow. Voltage set to the min: -10 V" << std::endl;
			return (I16)_I16_MIN;
		}
		else
			return (I16)(x / 10 * _I16_MAX);
	}


	//Send out an analog instruction, where the analog level 'val' is held for the amount of time 't'
	U32 singleAnalogOut(double t, double val)
	{
		const U16 AOlatency_tick = 2;	//To calibrate it, run AnalogLatencyCalib(). I think the latency comes from the memory block, which takes 2 cycles for reading
		return packU32(convertUs2tick(t) - AOlatency_tick, convertVolt2I16(val));
	}


	//Send out a single digital instruction, where 'DO' is held LOW or HIGH for the amount of time 't'. The DOs in Connector1 are rated at 10MHz, Connector0 at 80MHz.
	U32 singleDigitalOut(double t, bool DO)
	{
		const U16 DOlatency_tick = 2;	//To calibrate it, run DigitalLatencyCalib(). I think the latency comes from the memory block, which takes 2 cycles to read
		if (DO)
			return packU32(convertUs2tick(t) - DOlatency_tick, 0x0001);
		else
			return packU32(convertUs2tick(t) - DOlatency_tick, 0x0000);
	}


	//Generate a single pixel-clock instruction, where 'DO' is held LOW or HIGH for the amount of time 't'
	U32 singlePixelClock(double t, bool DO)
	{
		const U16 PClatency_tick = 1;//The pixel-clock is implemented in a SCTL. I think the latency comes from reading the LUT buffer
		if (DO)
			return packU32(convertUs2tick(t) - PClatency_tick, 0x0001);
		else
			return packU32(convertUs2tick(t) - PClatency_tick, 0x0000);
	}

	QU32 generateLinearRamp(double TimeStep, double RampLength, double Vinitial, double Vfinal)
	{
		QU32 queue;
		const bool debug = 0;

		if (TimeStep < AOdt_us)
		{
			std::cerr << "WARNING in " << __FUNCTION__ << ": time step too small. Time step set to " << AOdt_us << " us" << std::endl;
			TimeStep = AOdt_us;						//Analog output time increment (in us)
			return {};
		}

		const int nPoints = (int)(RampLength / TimeStep);		//Number of points

		if (nPoints <= 1)
		{
			std::cerr << "ERROR in " << __FUNCTION__ << ": not enought points for the linear ramp" << std::endl;
			std::cerr << "nPoints: " << nPoints << std::endl;
			return {};
		}
		else
		{
			if (debug)
			{
				std::cout << "nPoints: " << nPoints << std::endl;
				std::cout << "time \tticks \tv" << std::endl;
			}

			for (int ii = 0; ii < nPoints; ii++)
			{
				const double V = Vinitial + (Vfinal - Vinitial)*ii / (nPoints - 1);
				queue.push(singleAnalogOut(TimeStep, V));

				if (debug)
					std::cout << (ii + 1) * TimeStep << "\t" << (ii + 1) * convertUs2tick(TimeStep) << "\t" << V << "\t" << std::endl;
			}

			if (debug)
			{
				getchar();
				return {};
			}


		}
		return queue;
	}
}


FPGAapi::FPGAapi(): mVectorOfQueues(Nchan)
{
	mStatus = NiFpga_Initialize();		//Must be called before any other FPGA calls
	if (mStatus < 0) throw FPGAexception((std::string)__FUNCTION__ + " with FPGA code " + std::to_string(mStatus));
	//std::cout << "FPGA initialize status: " << mStatus << std::endl;

	NiFpga_MergeStatus(&mStatus, NiFpga_Open(Bitfile, NiFpga_FPGAvi_Signature, "RIO0", 0, &mSession));		//Opens a session, downloads the bitstream. 1=no run, 0=run
	if (mStatus < 0) throw FPGAexception((std::string)__FUNCTION__ + " with FPGA code " + std::to_string(mStatus));
}

FPGAapi::~FPGAapi(){};


void FPGAapi::initialize()
{
	//Initialize the FPGA variables. See 'Const.cpp' for the definition of each variable
	NiFpga_MergeStatus(&mStatus, NiFpga_WriteU8(mSession, NiFpga_FPGAvi_ControlU8_PhotonCounterInputSelector, photonCounterInput));			//Debugger. Use the PMT-pulse simulator as the input of the photon-counter
	NiFpga_MergeStatus(&mStatus, NiFpga_WriteU8(mSession, NiFpga_FPGAvi_ControlU8_LineClockInputSelector, lineClockInput));					//Select the Line clock: resonant scanner or function generator
	NiFpga_MergeStatus(&mStatus, NiFpga_WriteBool(mSession, NiFpga_FPGAvi_ControlBool_FIFOINtrigger, 0));									//control-sequence trigger
	NiFpga_MergeStatus(&mStatus, NiFpga_WriteBool(mSession, NiFpga_FPGAvi_ControlBool_LineGateTrigger, 0));									//data-acquisition trigger

	NiFpga_MergeStatus(&mStatus, NiFpga_WriteU16(mSession, NiFpga_FPGAvi_ControlU16_FIFOtimeout, (U16)FIFOtimeout_tick));
	NiFpga_MergeStatus(&mStatus, NiFpga_WriteU16(mSession, NiFpga_FPGAvi_ControlU16_Nchannels, (U16)Nchan));
	NiFpga_MergeStatus(&mStatus, NiFpga_WriteU16(mSession, NiFpga_FPGAvi_ControlU16_SyncDOtoAO, (U16)syncDOtoAO_tick));
	NiFpga_MergeStatus(&mStatus, NiFpga_WriteU16(mSession, NiFpga_FPGAvi_ControlU16_SyncAODOtoLineGate, (U16)syncAODOtoLineGate_tick));
	NiFpga_MergeStatus(&mStatus, NiFpga_WriteU16(mSession, NiFpga_FPGAvi_ControlU16_NlinesAll, (U16)(nLinesAllFrames + nFrames * nLinesSkip)));			//Total number of lines in all the frames, including the skipped lines
	NiFpga_MergeStatus(&mStatus, NiFpga_WriteU16(mSession, NiFpga_FPGAvi_ControlU16_NlinesPerFrame, (U16)heightPerFrame_pix));							//Number of lines in a frame, without including the skipped lines
	NiFpga_MergeStatus(&mStatus, NiFpga_WriteU16(mSession, NiFpga_FPGAvi_ControlU16_NlinesPerFramePlusSkips, (U16)(heightPerFrame_pix + nLinesSkip)));	//Number of lines in a frame including the skipped lines

	//Shutters
	NiFpga_MergeStatus(&mStatus, NiFpga_WriteBool(mSession, NiFpga_FPGAvi_ControlBool_Shutter1, 0));
	NiFpga_MergeStatus(&mStatus, NiFpga_WriteBool(mSession, NiFpga_FPGAvi_ControlBool_Shutter2, 0));

	//Vibratome control
	NiFpga_MergeStatus(&mStatus, NiFpga_WriteBool(mSession, NiFpga_FPGAvi_ControlBool_VT_start, 0));
	NiFpga_MergeStatus(&mStatus, NiFpga_WriteBool(mSession, NiFpga_FPGAvi_ControlBool_VT_back, 0));
	NiFpga_MergeStatus(&mStatus, NiFpga_WriteBool(mSession, NiFpga_FPGAvi_ControlBool_VT_forward, 0));
	NiFpga_MergeStatus(&mStatus, NiFpga_WriteBool(mSession, NiFpga_FPGAvi_ControlBool_VT_NC, 0));

	//Resonant scanner
	NiFpga_MergeStatus(&mStatus, NiFpga_WriteI16(mSession, NiFpga_FPGAvi_ControlI16_RS_voltage, 0));										//Output voltage
	NiFpga_MergeStatus(&mStatus, NiFpga_WriteBool(mSession, NiFpga_FPGAvi_ControlBool_RS_ON_OFF, 0));										//Turn on/off

																																			//Debugging
	NiFpga_MergeStatus(&mStatus, NiFpga_WriteArrayBool(mSession, NiFpga_FPGAvi_ControlArrayBool_Pulsesequence, pulseArray, nPulses));
	NiFpga_MergeStatus(&mStatus, NiFpga_WriteBool(mSession, NiFpga_FPGAvi_ControlBool_FIFOOUTdebug, 0));									//FIFO OUT

	if (mStatus < 0) throw FPGAexception((std::string)__FUNCTION__ + " with FPGA code " + std::to_string(mStatus));
	//std::cout << "FPGA initialization status: " << mStatus << std::endl;
}


//Send every single queue in VectorOfQueue to the FPGA bufer
//For this, concatenate all the single queues in a single long queue. THE QUEUE POSITION DETERMINES THE TARGETED CHANNEL	
//Then transfer the elements in the long queue to an array to interface the FPGA
//Alternatively, the single queues could be transferred directly to the array, but why bothering...
void FPGAapi::writeFIFO()
{
	QU32 allQueues;								//Create a single long queue
	for (int i = 0; i < Nchan; i++)
	{
		allQueues.push(mVectorOfQueues[i].size());			//Push the number of elements in each individual queue VectorOfQueues[i]
		while (!mVectorOfQueues[i].empty())
		{
			allQueues.push(mVectorOfQueues[i].front());		//Push all the elemets from the individual queues VectorOfQueues[i] to allQueues
			mVectorOfQueues[i].pop();
		}
	}


	const int sizeFIFOqueue = allQueues.size();		//Total number of elements in all the queues 

	if (sizeFIFOqueue > FIFOINmax)
		throw std::overflow_error((std::string)__FUNCTION__ + ": FIFO IN overflow");

	U32* FIFO = new U32[sizeFIFOqueue];				//Create an array for interfacing the FPGA	
	for (int i = 0; i < sizeFIFOqueue; i++)
	{
		FIFO[i] = allQueues.front();				//Transfer the queue elements to the array
		allQueues.pop();
	}
	allQueues = {};									//Cleanup the queue (C++11 style)

													//Send the data to the FPGA through the FIFO
	const U32 timeout = -1;							// in ms. A value -1 prevents the FIFO from timing out
	U32 r;											//empty elements remaining

	NiFpga_MergeStatus(&mStatus, NiFpga_WriteFifoU32(mSession, NiFpga_FPGAvi_HostToTargetFifoU32_FIFOIN, FIFO, sizeFIFOqueue, timeout, &r));

	//std::cout << "FPGA FIFO status: " << mStatus << std::endl;
	delete[] FIFO;									//cleanup the array
}

void FPGAapi::sendRTtoFPGA()
{
	this->writeFIFO();

	//Distribute the commands among the different channels (see the implementation of the LV code)
	NiFpga_MergeStatus(&mStatus, NiFpga_WriteBool(mSession, NiFpga_FPGAvi_ControlBool_FIFOINtrigger, 1));
	if (mStatus < 0) throw FPGAexception((std::string)__FUNCTION__ + " with FPGA code " + std::to_string(mStatus));

	NiFpga_MergeStatus(&mStatus, NiFpga_WriteBool(mSession, NiFpga_FPGAvi_ControlBool_FIFOINtrigger, 0));
	if (mStatus < 0) throw FPGAexception((std::string)__FUNCTION__ + " with FPGA code " + std::to_string(mStatus));

	//std::cout << "Pulse trigger status: " << mStatus << std::endl;
}

//Execute the commands
void FPGAapi::triggerRTsequence()
{
	NiFpga_MergeStatus(&mStatus, NiFpga_WriteBool(mSession, NiFpga_FPGAvi_ControlBool_LineGateTrigger, 1));
	if (mStatus < 0) throw FPGAexception((std::string)__FUNCTION__ + " with FPGA code " + std::to_string(mStatus));

	NiFpga_MergeStatus(&mStatus, NiFpga_WriteBool(mSession, NiFpga_FPGAvi_ControlBool_LineGateTrigger, 0));
	if (mStatus < 0) throw FPGAexception((std::string)__FUNCTION__ + " with FPGA code " + std::to_string(mStatus));

	//std::cout << "Acquisition trigger status: " << status << std::endl;
}


//Trigger the FIFO flushing
void FPGAapi::flushFIFO()
{
	NiFpga_MergeStatus(&mStatus, NiFpga_WriteBool(mSession, NiFpga_FPGAvi_ControlBool_FlushTrigger, 1));
	if (mStatus < 0) throw FPGAexception((std::string)__FUNCTION__ + " with FPGA code " + std::to_string(mStatus));

	NiFpga_MergeStatus(&mStatus, NiFpga_WriteBool(mSession, NiFpga_FPGAvi_ControlBool_FlushTrigger, 0));
	if (mStatus < 0) throw FPGAexception((std::string)__FUNCTION__ + " with FPGA code " + std::to_string(mStatus));

	//std::cout << "Flush trigger status: " << mStatus << std::endl;
}

//The FPGAapi object has to be closed explicitly (in opposition to using the destructor) because it lives in main()
void FPGAapi::close()
{
	if (NiFpga_IsNotError(mStatus))
	{
		NiFpga_MergeStatus(&mStatus, NiFpga_Close(mSession, 1));						//Closes the session to the FPGA. The FPGA resets (Re-downloads the FPGA bitstream to the target, the outputs go to zero)
																						//unless either another session is still open or you use the NiFpga_CloseAttribute_NoResetIfLastSession attribute.
																						//0 resets, 1 does not reset
		//std::cout << "FPGA closing-session status: " << mStatus << std::endl;
	}

	//You must call this function after all other function calls if NiFpga_Initialize succeeds. This function unloads the NiFpga library.
	NiFpga_MergeStatus(&mStatus, NiFpga_Finalize());
	//std::cout << "FPGA finalize status: " << mStatus << std::endl;
}


void FPGAapi::printFPGAstatus(char functionName[])
{
	if (mStatus < 0)
		std::cerr << "ERROR: '" << functionName << "' exited with FPGA code: " << mStatus << std::endl;
	if (mStatus > 0)
		std::cerr << "WARNING: '" << functionName << "' exited with FPGA code: " << mStatus << std::endl;
}


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


#include "FPGAsequences.h"

#pragma region "FPGA combined sequences"

int FPGAcombinedSequence(NiFpga_Status* status, NiFpga_Session session)
{

	/*
	const double RSamplitude_um = 200 * um;
	const double RSamplitude_volt = RSamplitude_um * RS_voltPerUm;
	resonantScanner_SetOutputVoltager(status, session, RSamplitude_volt);
	Sleep(1000);
	resonantScanner_StartStop(status, session, 1);
	Sleep(3000);
	resonantScanner_StartStop(status, session, 0);
	*/


	//Send the commands to the FPGA
	sendQueueToFPGA(status, session, Scan2D());

	//Trigger the data acquisition
	triggerFIFOIN(status, session);		

	//Read the photon count
	readPhotonCount(status, session);


	//SECOND ROUND
	if (0)
	{
		//sendQueueToFPGA(status, session, TestAODO());
		triggerFIFOIN(status, session);
		triggerLineGate(status, session);
	}

	return 0;
}
//endregion "FPGA combined sequences"
#pragma endregion

#pragma region "FPGA individual sequences"

//Acquire a 2D image by linearly scan the galvo while the RS is on
U32QV Scan2D()
{
	U32QV VectorOfQueues(Nchan);																			//Create and initialize a vector of queues. Each queue correspond to a channel on the FPGA

	//Pixel clock
	VectorOfQueues[PCLOCK] = PixelClockEvenTime();

	//Linear ramp for the galvo
	const double GalvoAmplitude_um = 200 * um;
	const double GalvoAmplitude_volt = GalvoAmplitude_um * Galvo_voltPerUm;
	//const double GalvoAmplitude_volt = 2.5;
	const double GalvoStep = 8 * us;

	U32Q linearRampSegment0 = generateLinearRamp(GalvoStep, 25 * ms, -GalvoAmplitude_volt, GalvoAmplitude_volt);	//Ramp up the galvo from -GalvoAmplitude_volt to GalvoAmplitude_volt
	VectorOfQueues[ABUF0] = linearRampSegment0;
	VectorOfQueues[ABUF0].push(generateSingleAnalogOut(4 * us, -GalvoAmplitude_volt));									//Set the galvo back to -GalvoAmplitude_volt

	//DO0
	VectorOfQueues[DBUF0].push(generateSingleDigitalOut(4 * us, 1));
	VectorOfQueues[DBUF0].push(generateSingleDigitalOut(4 * us, 0));
	VectorOfQueues[DBUF0].push(generateSingleDigitalOut(4 * us, 0));
	VectorOfQueues[DBUF0].push(generateSingleDigitalOut(4 * us, 0));

	return VectorOfQueues;
}

//Pixel clock sequence. The pixel clock is triggered by the line clock (see the LV implementation), followed by a waiting time 'InitialWaitingTime'
//At 160MHz, the clock increment is 6.25ns = 0.00625us
//Pixel clock evently spaced in time
U32Q PixelClockEvenTime()
{
	U32Q Q;																//Create a queue
	
	const double InitialWaitingTime = 6.25*us;							//Initial waiting time to center the pixel clock in a line scan
																		//Currently, there are 400 pixels and the dwell time is 125ns. Then, 400*125ns = 50us. A line-scan lasts 62.5us. Therefore, the waiting time is (62.5-50)/2 = 6.25us
	U16 latency = 2;													//latency of detecting the line clock. Calibrate the latency with the oscilloscope
	Q.push(u32pack(us2tick(InitialWaitingTime) - latency, 0x0000));
																			
	const double PixelWaitingTime = 0.125 * us;
	for (U16 ii = 0; ii < WidthPerFrame_pix + 1; ii++)					//Npixels+1 because there is one more pixel-clock tick than number of pixels
		Q.push(generateSinglePixelClock(PixelWaitingTime, TRUE));						//Generate the pixel clock. Everytime TRUE is pushed, the pixel clock "ticks" (flips its state)

	return Q;															//Return a queue (and not a vector of queues)
}

//Pixel clock sequence. The pixel clock is triggered by the line clock (see the LV implementation), followed by a waiting time 'InitialWaitingTime'
//At 160MHz, the clock increment is 6.25ns = 0.00625us
U32Q PixelClockEvenSpace()
{
	U32Q Q;

	return Q;
}

//endregion "FPGA individual sequences"
#pragma endregion


/* Test the Seq class
void SeqClassTest()
{
Seq ss;
ss.shutter(1 * us, 1);
std::cout << "size of the vector" << ss.size() << std::endl;
std::cout << "" << (ss.vector())[0].size() << std::endl;
Sleep(1000);
}
*/
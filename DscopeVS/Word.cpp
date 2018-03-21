#include "Word.h"


void printHex(U32 input)
{
	std::cout << std::hex << std::uppercase << input << std::nouppercase << std::dec << std::endl;
}


//time t and analog output x are encoded in 16 bits each. Pack t in MSB and x in LSB.
U32 u32pack(U16 t, U16 x)
{
	return (t << 16) | (0x0000FFFF & x);
}


//convert microseconds to ticks
U16 us2tick(double t)
{
	double aux = t * tickPerUs;
	U16 dt_tick_MIN = 2;		//Currently, DO and AO have a latency of 2 ticks
	if ((U32)aux > 0x0000FFFF)
	{
		std::cout << "WARNING: time step overflow. Time step set to the max: " << std::fixed << _UI16_MAX * dt_us << " us\n";
		return _UI16_MAX;
	}
	else if ((U32)aux < dt_tick_MIN)
	{
		std::cout << "WARNING: time step underflow. Time step set to the min:" << std::fixed << dt_tick_MIN * dt_us << " us\n";;
		return dt_tick_MIN;
	}
	else
		return (U16)aux;
}


//converts voltage (range: -10V to 10V) to a signed int 16 (range: -32768 to 32767)
//0x7FFFF = 0d32767
//0xFFFF = -1
//0x8000 = -32768
I16 AOUT(double x)
{
	if (x > 10)
	{
		std::cout << "WARNING: voltage overflow. Voltage set to the max: 10 V\n";
		return (U16)_I16_MAX;
	}
	else if (x < -10)
	{
		std::cout << "WARNING: voltage underflow. Voltage set to the min: -10 V\n";
		return (U16)_I16_MIN;
	}
	else
		return (U16)(x / 10 * _I16_MAX);
}


//Send out an analog instruction, where the analog level 'val' is held for the amount of time 't'
U32 AnalogOut(double t, double val)
{
	U16 AOlatency_tick = 2;	//To  calibrate it, run AnalogLatencyCalib(). I think the latency comes from the memory block, which takes 2 cycles for reading
	return u32pack(us2tick(t) - AOlatency_tick, AOUT(val));
}


//Send out a digital instruction, where 'DO' is held LOW or HIGH for the amount of time 't'. The DOs in Connector1 are rated at 10MHz, Connector0 at 80MHz.
U32 DigitalOut(double t, bool DO)
{
	U16 DOlatency_tick = 2;	//To  calibrate it, run DigitalLatencyCalib(). I think the latency comes from the memory block, which takes 2 cycles to read
	if (DO)
		return u32pack(us2tick(t) - DOlatency_tick, 0x0001);
	else
		return u32pack(us2tick(t) - DOlatency_tick, 0x0000);
}


//Send out a pixel-clock instruction, where 'DO' is held LOW or HIGH for the amount of time 't'
U32 PixelClock(double t, bool DO)
{
	U16 PClatency_tick = 1;//The pixel-clock is implemented in a SCTL. I think the latency comes from reading the LUT buffer
	if (DO)
		return u32pack(us2tick(t) - PClatency_tick, 0x0001);
	else
		return u32pack(us2tick(t) - PClatency_tick, 0x0000);
}


// Push all elements of 'tailQ' into 'headQ'
U32Q PushQ(U32Q& headQ, U32Q& tailQ)
{
	while (!tailQ.empty())
	{
		headQ.push(tailQ.front());
		tailQ.pop();
	}
	return headQ;
}


// PARAMETERS: time step, ramp length, initial voltage, final voltage
U32Q linearRamp(double dt, double T, double Vi, double Vf)
{
	U32Q queue;
	bool debug = false;

	if (dt < AOdt_us)
	{
		std::cout << "WARNING: time step too small. Time step set to " << AOdt_us << " us\n";
		dt = AOdt_us; //Analog output time increment in us
		getchar();
	}

	U32 nPoints = (U32)(T / dt);		//number of points

	if (nPoints <= 1)
	{
		std::cout << "ERROR: not enought points for the linear ramp\n";
		std::cout << "nPoints: " << nPoints << "\n";
		getchar();
	}
	else
	{
		if (debug)
		{
			std::cout << "nPoints: " << nPoints << "\n";
			std::cout << "time \tticks \tv \n";
		}

		for (U32 ii = 0; ii < nPoints; ii++)
		{
			double V = Vi + (Vf - Vi)*ii / (nPoints - 1);
			queue.push(AnalogOut(dt, V));

			if (debug)
				std::cout << (ii + 1) * dt << "\t" << (ii + 1) * us2tick(dt) << "\t" << V << "\t" << "\n";
		}

		if (debug)
			getchar();

	}
	return queue;
}


void CountPhotons(NiFpga_Status* status, NiFpga_Session session)
{

	//test: write output to txt file
	std::ofstream myfile;
	myfile.open("_photon-counts.txt");



	//The LV code saves the count every time the pixel clock ticks (flips its state)
	size_t Npop = Npixels * Nmaxlines;
	uint32_t remainingFIFOa; //elements remaining
	uint32_t remainingFIFOb; //elements remaining
	size_t timeout = 100;
	uint32_t* dataFIFOa = new uint32_t[Npop];
	//uint32_t* dataFIFOb = new uint32_t[Npop];
	for (U32 ii = 0; ii < Npop; ii++)
	{
		dataFIFOa[ii] = 0;
		//dataFIFOb[ii] = 0;
	}

	std::queue<uint32_t> NumberOfElements;


	uint32_t** test = new uint32_t*[10];
	for (int i = 0; i < 10; i++)
		test[i] = new uint32_t[Npop];


	//Start the FIFO on the PC's side
	NiFpga_MergeStatus(status, NiFpga_StartFifo(session, NiFpga_FPGA_TargetToHostFifoU32_FIFOOUTa));
	NiFpga_MergeStatus(status, NiFpga_StartFifo(session, NiFpga_FPGA_TargetToHostFifoU32_FIFOOUTb));

	//Total number of elements read from the FIFO
	size_t NelementsReadFIFOa = 0;
	size_t NelementsReadFIFOb = 0;
	size_t timeoutCounter = 0;
	size_t bufferIndex = 0;



	//Declare the timer
	std::clock_t start;
	double duration;
	start = std::clock(); //Start the timer

	//read the FIFO until it is emptied
	while (NelementsReadFIFOa < Npop || NelementsReadFIFOb < Npop)
	{
		/*
		if (timeoutCounter == 50)
		{
			NiFpga_MergeStatus(status, NiFpga_WriteBool(session, NiFpga_FPGA_ControlBool_Start_acquisition, 1));
			NiFpga_MergeStatus(status, NiFpga_WriteBool(session, NiFpga_FPGA_ControlBool_Start_acquisition, 0));


			start = std::clock(); //Start the timer
		}
		*/


		//Ask if there are data available in the FIFO
		if (NelementsReadFIFOa < Npop) //Keep asking until all the data are downloaded, i.e. NelementsReadFIFOa = Npop
		{
			//By requesting 0 elements from the FIFO, the function returns the number of elements available in the FIFO. If no data are available yet, then remainingFIFOa = 0 is returned
			NiFpga_MergeStatus(status, NiFpga_ReadFifoU32(session, NiFpga_FPGA_TargetToHostFifoU32_FIFOOUTa, dataFIFOa, 0, timeout, &remainingFIFOa));
			std::cout << "Number of elements remaining in host FIFO a: " << remainingFIFOa << "\n";
		}
		else //If all the data have been downloaded, i.e. NelementsReadFIFOa = Npop, then stop reading the FIFO
			remainingFIFOa = 0;
			
		//Read the data
		if (remainingFIFOa > 0) //If there are data available in the FIFO
		{
			NelementsReadFIFOa += remainingFIFOa; //Keep track of how many data points have been read already

			//Read the elements in the FIFO
			NiFpga_MergeStatus(status, NiFpga_ReadFifoU32(session, NiFpga_FPGA_TargetToHostFifoU32_FIFOOUTa, dataFIFOa, remainingFIFOa, timeout, &remainingFIFOa));
		}



		//FIFO OUT b

		if (NelementsReadFIFOb < Npop)
		{
			NiFpga_MergeStatus(status, NiFpga_ReadFifoU32(session, NiFpga_FPGA_TargetToHostFifoU32_FIFOOUTb, test[bufferIndex], 0, timeout, &remainingFIFOb));
			std::cout << "Number of elements remaining in host FIFO b: " << remainingFIFOb << "\n";
		}
		else
			remainingFIFOb = 0;


		if (remainingFIFOb > 0)
		{
			NelementsReadFIFOb += remainingFIFOb;
		
			NumberOfElements.push(remainingFIFOb); //record how many elements are read													

			//Read the elements in the FIFO
			std::cout << "counters: " << bufferIndex << "\n";
			NiFpga_MergeStatus(status, NiFpga_ReadFifoU32(session, NiFpga_FPGA_TargetToHostFifoU32_FIFOOUTb, test[bufferIndex], remainingFIFOb, timeout, &remainingFIFOb));


			//for (U32 ii = 0; ii < qwerty; ii++)
				//myfile << test[timeoutCounter][ii] << "\n";


			bufferIndex++;

		}

		//std::cout << "counters: " << NelementsReadFIFOa << "\t" << NelementsReadFIFOb << "\n";
		
		//Timeout the FIFO reading in case the data transfer fails
		timeoutCounter++;
		if (timeoutCounter > 100)
		{
			std::cout << "WARNING: FIFO timeout: \n";
			break;
		}
			
	}

	//Stop the timer
	duration = (std::clock() - start) / (double)CLOCKS_PER_SEC;
	std::cout << "Elapsed time: " << duration << " s \n";
	std::cout << "FIFO bandwidth: " << 2*32*Npop/duration/1000000 << " Mbps \n"; //2 FIFOs of 32 bits each

	
	for (int i = 0; !NumberOfElements.empty(); i++)
	{
		for (int j = 0; j < NumberOfElements.front(); j++)
			myfile << test[i][j] << "\n";
		NumberOfElements.pop();
	}
		


	//Read the number of free spots remaining in the FIFO
	uint32_t Nfree;
	NiFpga_MergeStatus(status, NiFpga_ReadU32(session, NiFpga_FPGA_IndicatorU32_FIFOfreespots, &Nfree));
	std::cout << "Number of free spots in the FIFO: " << (U32)Nfree << "\n";
	

	//close txt file
	myfile.close();


	delete dataFIFOa;
	//delete dataFIFOb;

	for (int i = 0; i < bufferIndex; ++i) {
		delete[] test[i];
	}
	delete[] test;

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

/*
// Create a new queue with 'headQ' and 'tailQ' combined
U32Q NewConcatenatedQ(U32Q& headQ, U32Q& tailQ)
{
	U32Q newQ;
	while (!headQ.empty())
	{
		newQ.push(headQ.front());
		headQ.pop();
	}
	while (!tailQ.empty())
	{
		newQ.push(tailQ.front());
		tailQ.pop();
	}
	return newQ;
}
*/
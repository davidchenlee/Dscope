#include "TCsequences.h"

//Test the analog and digital output and the relative timing wrt the pixel clock
U32QV TestAODO()
{
	//Create and initialize a vector of queues. Each queue correspond to a channel on the FPGA
	U32QV QV(Nchan);

	//AO0
	//QV[ABUF2].push(generateSingleAnalogOut(4 * us, 10));
	//QV[ABUF2].push(generateSingleAnalogOut(4 * us, 0));
	//QV[ABUF2].push(generateSingleAnalogOut(4 * us, 10));
	//QV[ABUF2].push(generateSingleAnalogOut(4 * us, 0));//go back to zero

	//DO0
	QV[IDshutter1].push(generateSingleDigitalOut(4 * us, 1));
	QV[IDshutter1].push(generateSingleDigitalOut(4 * us, 0));
	QV[IDshutter1].push(generateSingleDigitalOut(4 * us, 0));
	QV[IDshutter1].push(generateSingleDigitalOut(4 * us, 0));

	//QV[AO0] = GalvoSeq();


	//CURRENTLY, AO1 AND DO1 ARE TRIGGERED BY THE LINE CLOCK
	//AO0
	QV[IDgalvo1].push(generateSingleAnalogOut(4 * us, 5));
	QV[IDgalvo1].push(generateSingleAnalogOut(4 * us, 0));
	//QV[IDgalvo1].push(generateSingleAnalogOut(4 * us, 5));
	//QV[IDgalvo1].push(generateSingleAnalogOut(4 * us, 0));

	//DO0
	//QV[DBUF1].push(generateSingleDigitalOut(4 * us, 1));
	//QV[DBUF1].push(generateSingleDigitalOut(4 * us, 0));
	//QV[DBUF1].push(generateSingleDigitalOut(4 * us, 0));
	//QV[DBUF1].push(generateSingleDigitalOut(4 * us, 0));

	//Pixel clock
	PixelClock pixelClock;
	QV[PCLOCK] = pixelClock.PixelClock::PixelClockEqualDuration();

	return QV;
}

U32QV TestAODOandRamp()
{
	//Create and initialize a vector of queues. Each queue correspond to a channel on the FPGA
	U32QV QV(Nchan);

	//Generate linear ramps
	double Vmax = 5;
	double step = 4 * us;
	U32Q Q; //Create a queue
	U32Q linearRamp1 = generateLinearRamp(step, 2 * ms, 0, -Vmax);
	U32Q linearRamp2 = generateLinearRamp(step, 20 * ms, -Vmax, Vmax);
	U32Q linearRamp3 = generateLinearRamp(step, 2 * ms, Vmax, 0);
	//concatenateQueues(Q, linearRamp1);
	concatenateQueues(Q, linearRamp2);
	//concatenateQueues(Q, linearRamp3);
	QV[IDgalvo1] = Q;
	Q = {}; //clean up

	double pulsewidth = 300 * us;

	/*
	QV[AO0].push(generateSingleAnalogOut(4 * us, 0.000));
	QV[AO0].push(generateSingleAnalogOut(pulsewidth, 5));
	QV[AO0].push(generateSingleAnalogOut(4 * us, 0.000));
	*/

	QV[IDshutter1].push(generateSingleDigitalOut(pulsewidth, 1));
	QV[IDshutter1].push(generateSingleDigitalOut(4 * us, 0));
	return QV;
}

//Generate a long digital pulse and check the duration with the oscilloscope
U32QV DigitalTimingCheck()
{
	//Create and initialize a vector of queues. Each queue correspond to a channel on the FPGA
	U32QV QV(Nchan);
	double step = 400 * us;

	//DO0
	QV[IDshutter1].push(generateSingleDigitalOut(step, 1));
	QV[IDshutter1].push(generateSingleDigitalOut(step, 0));

	return QV;
}


//Generate many short digital pulses and check the overall duration with the oscilloscope
U32QV DigitalLatencyCalib()
{
	//Create and initialize a vector of queues. Each queue correspond to a channel on the FPGA
	U32QV QV(Nchan);
	double step = 4 * us;

	//DO0
	QV[IDshutter1].push(generateSingleDigitalOut(step, 1));

	//many short digital pulses to accumulate the error
	for (U32 ii = 0; ii < 99; ii++)
		QV[IDshutter1].push(generateSingleDigitalOut(step, 0));

	QV[IDshutter1].push(generateSingleDigitalOut(step, 1));
	QV[IDshutter1].push(generateSingleDigitalOut(step, 0));

	return QV;
}

//First, calibrate the digital channels, then use it as a time reference
U32QV AnalogLatencyCalib()
{
	//Create and initialize a vector of queues. Each queue correspond to a channel on the FPGA
	U32QV QV(Nchan);
	double delay = 400 * us;
	double step = 4 * us;

	//AO0
	QV[IDgalvo1].push(generateSingleAnalogOut(step, 10));//initial pulse
	QV[IDgalvo1].push(generateSingleAnalogOut(step, 0));
	QV[IDgalvo1] = concatenateQueues(QV[0], generateLinearRamp(4 * us, delay, 0, 5));//linear ramp to accumulate the error
	QV[IDgalvo1].push(generateSingleAnalogOut(step, 5));//final pulse
	QV[IDgalvo1].push(generateSingleAnalogOut(step, 0));

	//DO0
	QV[IDshutter1].push(generateSingleDigitalOut(step, 1));
	QV[IDshutter1].push(generateSingleDigitalOut(step, 0));
	QV[IDshutter1].push(generateSingleDigitalOut(delay, 0));
	QV[IDshutter1].push(generateSingleDigitalOut(step, 1));
	QV[IDshutter1].push(generateSingleDigitalOut(step, 0));

	return QV;
}
#include "Sequences.h"

/*
There are basically 2 imaging modes :
1. Frame by frame: For each frame, a RT sequence is created, loaded onto the fpga, and a corresponding image is acquired. The z stage is moved after each image is acquired.
2. Continuous: A single long RT sequence contains all the frames. Such sequence is loaded onto the fpga and run once. A stream of images is acquired. The z stage moves continuously
*/

void seq_main(const FPGAapi::Session &fpga)
{	
	const int runmode = 2;
	/*
	0 - Single image
	1 - Image continuously the same plane
	2 - Image many times the same plane for subsequent averaging
	3 - Stack volume from the initial z position
	4 - Stack volume around the initial z position
	*/
		
	const RunMode runMode = static_cast<RunMode>(runmode);
	
	//STAGE
	double3 position_mm = { 37.950, 29.150, 16.950 };	//Initial position

	//STACK
	const double stepSize_um = 0.5 * um;
	double zDelta_um = 10 * um;							//Acquire a stack within this range
	int nFramesAvg = 10;

	//LASER
	const int wavelength_nm = 940;
	double laserPower_mW = 170 * mW;
	Laser vision;
	vision.setWavelength(wavelength_nm);

	//GALVO
	const double FFOVgalvo_um = 300 * um;					//Full FOV in the slow axis
	const double duration = halfPeriodLineclock_us * heightPerFrame_pix; //= 62.5us * 400 pixels = 25 ms
	const double galvoTimeStep = 8 * us;
	const double posMax_um = FFOVgalvo_um / 2;

	//SAMPLE
	const std::string filename = "Liver";
	const double collar = 1.488;

	//FILTERWHEEL
	Filterwheel fw(FW1);
	fw.setColor(wavelength_nm);

	//SHUTTER
	Shutter shutter1(fpga, Shutter1);

	//RUN MODE SETTINGS
	int nFramesStack;
	bool overrideFlag;
	switch (runMode)
	{
	case single:
		nFramesStack = 1;
		nFramesAvg = 1;
		zDelta_um = 0.0;
		overrideFlag = FALSE;
		break;
	case continuous:
		nFramesStack = 1;
		nFramesAvg = 1000;
		zDelta_um = 0.0;
		overrideFlag = TRUE;
		break;
	case average:
		nFramesStack = 1;
		zDelta_um = 0.0;
		overrideFlag = FALSE;
		break;
	case stack:
		nFramesStack = (int)(zDelta_um / stepSize_um);
		overrideFlag = FALSE;
		break;
	case stack_centered:
		nFramesStack = (int)(zDelta_um / stepSize_um);
		position_mm.at(zz) -= 0.5 * zDelta_um / 1000; //For acquiring a stack
		overrideFlag = FALSE;
		break;
	default:
		throw std::invalid_argument((std::string)__FUNCTION__ + ": Selected run mode not available");
	}

	//DATALOG
	Logger datalog(filename);
	datalog.record("Wavelength (nm) = ", wavelength_nm);
	datalog.record("Laser power (mW) = ", laserPower_mW);
	datalog.record("Galvo full FOV (um) = ", FFOVgalvo_um);
	datalog.record("Galvo time step (us) = ", galvoTimeStep);
	datalog.record("Correction collar = ", collar);

	//SEQUENCE
	shutter1.open();
	Sleep(50);

	Stage stage;
	stage.moveStage3(position_mm);
	stage.waitForMovementToStop3();

	for (int ii = 0; ii < nFramesStack; ii++)
	{
		for (int jj = 0; jj < nFramesAvg; jj++)
		{
			std::cout << "z plane " << (ii + 1) << "/" << nFramesStack <<
				"\tFrame " << (jj + 1) << "/" << nFramesAvg <<
				"\tTotal frame " << ii * nFramesAvg + (jj + 1) << "/" << nFramesStack * nFramesAvg << std::endl;

			//CREATE A REAL-TIME SEQUENCE
			FPGAapi::RTsequence sequence(fpga);

			//GALVO FOR RT
			Galvo galvo(sequence, GALVO1);
			galvo.positionLinearRamp(galvoTimeStep, duration, posMax_um, -posMax_um);		//Linear ramp for the galvo
			galvo.positionLinearRamp(galvoTimeStep, 1 * ms, -posMax_um, posMax_um);			//set the output back to the initial value

			//POCKELS CELL FOR RT
			PockelsCell pockels(sequence, POCKELS1, wavelength_nm);
			pockels.pushPowerSinglet(8 * us, laserPower_mW);
			//pockels.voltageLinearRamp(4*us, 40*us, 0, 1*V);
			//pockels.voltageLinearRamp(galvoTimeStep, duration, 0.5*V, 1*V);	//Ramp up the laser intensity in a frame and repeat for each frame
			//pockels.scalingLinearRamp(1.0, 2.0);								//Linearly scale the laser intensity across all the frames

			//Upload the realtime sequence to the FPGA but don't execute it yet
			sequence.uploadRT();

			//Execute the realtime sequence and acquire the image
			Image image(fpga);
			image.acquire(TRUE, filename + " " + toString(wavelength_nm, 0) + "nm " + toString(laserPower_mW, 0) + "mW " +
				" x=" + toString(position_mm.at(xx), 3) + " y=" + toString(position_mm.at(yy), 3) + " z=" + toString(position_mm.at(zz), 4), overrideFlag); //Execute the RT sequence and acquire the image
		
			std::cout << std::endl;
		}

		stage.printPosition3();
		std::cout << std::endl;

		if (runMode == stack || runMode == stack_centered)
		{
			position_mm.at(zz) += stepSize_um / 1000;
			stage.moveStage(zz, position_mm.at(zz));
			//stage.waitForMovementToStop3();
			Sleep(500);
			//laserPower_mW += 0.5; //Increase the laser power by this much
		}
	}

	shutter1.close();
}

void seq_cont(const FPGAapi::Session &fpga)
{
	int nFramesAvg = 1;

	//LASER
	const int wavelength_nm = 940;
	double laserPower_mW = 170 * mW;
	Laser vision;
	vision.setWavelength(wavelength_nm);

	//GALVO
	const double FFOVgalvo_um = 300 * um;					//Full FOV in the slow axis
	const double duration = halfPeriodLineclock_us * heightPerFrame_pix; //= 62.5us * 400 pixels = 25 ms
	const double galvoTimeStep = 8 * us;
	const double posMax_um = FFOVgalvo_um / 2;

	//SAMPLE
	const std::string filename = "Liver";
	const double collar = 1.488;

	//FILTERWHEEL
	Filterwheel fw(FW1);
	fw.setColor(wavelength_nm);

	//SHUTTER
	Shutter shutter1(fpga, Shutter1);

	//SEQUENCE
	shutter1.open();
	Sleep(50);

	for (int jj = 0; jj < nFramesAvg; jj++)
	{
		//CREATE A REAL-TIME SEQUENCE
		FPGAapi::RTsequence sequence(fpga);

		//GALVO FOR RT
		Galvo galvo(sequence, GALVO1);
		galvo.positionLinearRamp(galvoTimeStep, duration, posMax_um, -posMax_um);		//Linear ramp for the galvo
		galvo.positionLinearRamp(galvoTimeStep, 1 * ms, -posMax_um, posMax_um);			//set the output back to the initial value

		//POCKELS CELL FOR RT
		PockelsCell pockels(sequence, POCKELS1, wavelength_nm);
		pockels.pushPowerSinglet(8 * us, laserPower_mW);

		//Upload the realtime sequence to the FPGA but don't execute it yet
		sequence.uploadRT();

		//Execute the realtime sequence and acquire the image
		Image image(fpga);
		image.acquire(TRUE,"Untitled",TRUE); //Execute the RT sequence and acquire the image

		Sleep(500);
	}
	shutter1.close();
}




void seq_testPixelclock(const FPGAapi::Session &fpga)
{
	//Create a realtime sequence
	FPGAapi::RTsequence sequence(fpga);
	sequence.uploadRT(); //Upload the realtime sequence to the FPGA but don't execute it yet
	Image image(fpga);
	image.acquire(TRUE); //Execute the realtime sequence and acquire the image

}

//Test the analog and digital output and the relative timing wrt the pixel clock
void seq_testAODO(const FPGAapi::Session &fpga)
{
	FPGAapi::RTsequence sequence(fpga);

	//DO
	sequence.pushDigitalSinglet(DOdebug, 4 * us, 1);
	sequence.pushDigitalSinglet(DOdebug, 4 * us, 0);

	//AO
	sequence.pushAnalogSinglet(GALVO1, 8 * us, 4);
	sequence.pushAnalogSinglet(GALVO1, 4 * us, 2);
	sequence.pushAnalogSinglet(GALVO1, 4 * us, 1);

	sequence.uploadRT();	//Upload the realtime sequence to the FPGA but don't execute it yet
	sequence.triggerRT();	//Execute the realtime sequence
}

void seq_testAOramp(const FPGAapi::Session &fpga)
{
	const double Vmax = 5;
	const double step = 4 * us;

	FPGAapi::RTsequence sequence(fpga);
	sequence.pushLinearRamp(GALVO1, step, 2 * ms, 0, -Vmax);
	sequence.pushLinearRamp(GALVO1, step, 20 * ms, -Vmax, Vmax);
	sequence.pushLinearRamp(GALVO1, step, 2 * ms, Vmax, 0);

	const double pulsewidth = 300 * us;
	sequence.pushDigitalSinglet(DOdebug, pulsewidth, 1);
	sequence.pushDigitalSinglet(DOdebug, 4 * us, 0);
}

//Generate a long digital pulse and check the duration with the oscilloscope
void seq_checkDigitalTiming(const FPGAapi::Session &fpga)
{
	const double step = 400 * us;

	FPGAapi::RTsequence sequence(fpga);
	sequence.pushDigitalSinglet(DOdebug, step, 1);
	sequence.pushDigitalSinglet(DOdebug, step, 0);
}

//Generate many short digital pulses and check the overall duration with the oscilloscope
void seq_calibDigitalLatency(const FPGAapi::Session &fpga)
{
	const double step = 4 * us;

	FPGAapi::RTsequence sequence(fpga);

	sequence.pushDigitalSinglet(DOdebug, step, 1);

	//Many short digital pulses to accumulate the error
	for (U32 ii = 0; ii < 99; ii++)
		sequence.pushDigitalSinglet(DOdebug, step, 0);

	sequence.pushDigitalSinglet(DOdebug, step, 1);
	sequence.pushDigitalSinglet(DOdebug, step, 0);
}

//First calibrate the digital channels, then use it as a time reference
void seq_calibAnalogLatency(const FPGAapi::Session &fpga)
{
	const double delay = 400 * us;
	const double step = 4 * us;

	FPGAapi::RTsequence sequence(fpga);
	sequence.pushAnalogSinglet(GALVO1, step, 10);	//Initial pulse
	sequence.pushAnalogSinglet(GALVO1, step, 0);
	sequence.pushLinearRamp(GALVO1, 4 * us, delay, 0, 5 * V);			//Linear ramp to accumulate the error
	sequence.pushAnalogSinglet(GALVO1, step, 10);	//Initial pulse
	sequence.pushAnalogSinglet(GALVO1, step, 0);	//Final pulse

	//DO0
	sequence.pushDigitalSinglet(DOdebug, step, 1);
	sequence.pushDigitalSinglet(DOdebug, step, 0);
	sequence.pushDigitalSinglet(DOdebug, delay, 0);
	sequence.pushDigitalSinglet(DOdebug, step, 1);
	sequence.pushDigitalSinglet(DOdebug, step, 0);
}

void seq_testFilterwheel()
{
	Filterwheel FW(FW1);
	//FW.setColor(RED);
	
	if(1)
	FW.setColor(1040);
	else
	FW.setColor(940);
}

void seq_testStageSetPosition()
{
	double duration;
	const double newPosition_mm = 5;
	Stage stage;

	std::cout << "Stages initial position:" << std::endl;
	stage.printPosition3();

	auto t_start = std::chrono::high_resolution_clock::now();

	stage.moveStage(zz, newPosition_mm);
	//stage.waitForMovementToStop(zz);

	//Stop the stopwatch
	duration = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - t_start).count();
	std::cout << "Elapsed time: " << duration << " ms" << std::endl;

	std::cout << "Stages final position:" << std::endl;
	stage.printPosition3();
	
	/*
	int input = 1;
	while (input)
	{
		std::cout << "Stage X position = " << stage.downloadPosition_mm(xx) << std::endl;
		std::cout << "Stage Y position = " << stage.downloadPosition_mm(yy) << std::endl;
		std::cout << "Stage X position = " << stage.downloadPosition_mm(zz) << std::endl;

		std::cout << "Enter command: ";
		std::cin >> input;
		//input = 0;
	}
	*/
}

void seq_testStageTriggerConfig()
{
	Stage stages;
	stages.printPosition3();
}

void seq_testmPMT()
{
	mPMT pmt;
	pmt.readAllGain();
	//pmt.setSingleGain(2, 300);
	//pmt.setAllGain(255);
	//pmt.readTemp();
	//pmt.setAllGain({ 100,255,255,255,255,255,255,255,255,255,255,255,255,255,100,255});
}

//Keep the pockels cell on.
//1. Manually open the shutter
//2. Set pockels1_enableAutoOff = 0
//3. Set lineclockInput = FG
void seq_testPockels(const FPGAapi::Session &fpga)
{
	//Create a realtime sequence
	FPGAapi::RTsequence sequence(fpga);

	//Open the Uniblitz shutter
	//Shutter shutter1(fpga, Shutter1);
	//shutter1.open();

	//Turn on the pockels cell
	PockelsCell pockels(sequence, POCKELS1, 750);
	pockels.pushPowerSinglet(8 * us, 50 * mW);
	//pockels.pushVoltageSinglet_(8 * us, 2.508 * V);

	//Upload the pockels sequence to the FPGA but don't execute it yet
	sequence.uploadRT();

	//Execute the sequence
	Image image(fpga);
	image.acquire();
}

void seq_testLaserComm(const FPGAapi::Session &fpga)
{
	Laser vision;
	//vision.setShutter(0);
	vision.setWavelength(940);
}

void seq_testRS(const FPGAapi::Session &fpga)
{
	ResonantScanner RS(fpga);
	//RS.turnOn_um(150);
	RS.turnOff();
}
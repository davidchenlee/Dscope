#pragma once
#include "Devices.h"
#include "Sequencer.h"
#include "SampleConfig.h"

//MAIN SEQUENCES
namespace Routines
{
	void stepwiseScan(const FPGA &fpga);
	void contScanZ(const FPGA &fpga);
	void panoramicScan(const FPGA &fpga);
	void sequencer(const FPGA &fpga, const int firstCommandIndex, const bool forceScanAllStacks, const RUN runSeq);
	void liveScan(const FPGA &fpga);
	void correctTiffReadFromTileConfiguration(const int firstCutNumber, const int lastCutNumber, std::vector<int> vec_wavelengthIndex);
}

//TESTS
namespace TestRoutines
{
	//FPGA timing
	void digitalLatency(const FPGA &fpga);
	void analogLatency(const FPGA &fpga);
	void pixelclock(const FPGA &fpga);
	void digitalTiming(const FPGA &fpga);
	void analogAndDigitalOut(const FPGA &fpga);
	void analogRamp(const FPGA &fpga);

	//Scanners
	void fineTuneScanGalvo(const FPGA &fpga);
	void resonantScanner(const FPGA &fpga);
	void galvosSync(const FPGA &fpga);
	void galvosLaserSync(const FPGA &fpga);

	//Stages
	void stagePosition();
	void stageConfig();

	//Lasers
	void shutter(const FPGA &fpga);
	void pockels(const FPGA &fpga);
	void semiAutomatedPockelsCalibration(const FPGA &fpga);
	void pockelsRamp(const FPGA &fpga);
	void lasers(const FPGA &fpga);
	void virtualLasers(const FPGA &fpga);
	void photobleach(const FPGA &fpga);

	//Data management
	void convertI16toVolt();
	void ethernetSpeed();
	void multithread();
	void clipU8();
	void dataLogger();
	void createSubfolder();
	void filterTileConfigurationFile(const int firstCutNumber, const int lastCutNumber);

	//Postprocessing
	void correctImageSingle();
	void correctImageBatch();
	void quickStitcher();
	void boolmap();

	//Sequence
	void sequencerConcurrentTest();

	//PMT16X
	void PMT16Xconfig();
	void PMT16Xdemultiplex(const FPGA &fpga);

	//Others
	void vibratome(const FPGA &fpga);
	void filterwheel();
	void collectorLens();
	void openCV();
}

#pragma once
#include "Devices.h"
#include "Sequencer.h"

//MAIN SEQUENCES
namespace PMT16XRoutines
{
	void frameByFrameZscan(const FPGAns::FPGA &fpga);
	void liveScan(const FPGAns::FPGA &fpga);
	void contZscan(const FPGAns::FPGA &fpga);
	void sequencer(const FPGAns::FPGA &fpga);
	//void frameByFrameZscanTilingXY(const FPGAns::FPGA &fpga, const int nSlice);
	void snapshot(const FPGAns::RTcontrol &RTcontrol, const Sequence &sequence, VirtualLaser &virtualLaser, Galvo &rescanner, Stage &stage);
}

//TESTS
namespace TestRoutines
{
	//FPGA timing
	void digitalLatency(const FPGAns::FPGA &fpga);
	void analogLatency(const FPGAns::FPGA &fpga);
	void pixelclock(const FPGAns::FPGA &fpga);
	void digitalTiming(const FPGAns::FPGA &fpga);
	void analogAndDigitalOut(const FPGAns::FPGA &fpga);
	void analogRamp(const FPGAns::FPGA &fpga);

	//Scanners
	void fineTuneScanGalvo(const FPGAns::FPGA &fpga);
	void resonantScanner(const FPGAns::FPGA &fpga);
	void galvosSync(const FPGAns::FPGA &fpga);

	//Stages
	void stagePosition();
	void stageConfig();

	//Lasers
	void shutter(const FPGAns::FPGA &fpga);
	void pockels(const FPGAns::FPGA &fpga);
	void pockelsRamp(const FPGAns::FPGA &fpga);
	void lasers(const FPGAns::FPGA &fpga);
	void virtualLasers(const FPGAns::FPGA &fpga);

	//Data management
	void convertI16toVolt();
	void tiffU8();
	void ethernetSpeed();
	void multithread();
	void clipU8();

	//Sequence
	void sequencerConcurrentTest();
	void locationSequence();

	//PMT16X
	void PMT16Xconfig();
	void PMT16Xdemultiplex(const FPGAns::FPGA &fpga);
	void PMT16XgavosSyncAndLaser(const FPGAns::FPGA &fpga);

	//Others
	void vibratome(const FPGAns::FPGA &fpga);
	void filterwheel();
	void collectorLens();
	void photobleach(const FPGAns::FPGA &fpga);
	void generateLocationsForBigStitcher();
	int2 nTileToArrayIndices(const int nTile);
	void openCV();
}

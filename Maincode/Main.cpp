#include "Routines.h"

int main(int argc, char* argv[])
{
	try
	{
		FPGA fpga;		//Create a FPGA session
		try
		{
			//SEQUENCES
			//Routines::stepwiseScan(fpga);
			//Routines::contScanZ(fpga);
			//Routines::panoramicScan(fpga);
			//Routines::sequencer(fpga, 0, false, RUN::EN);
			//Routines::liveScan(fpga);
			Routines::correctTiffReadFromTileConfiguration(0, 52, { 2 });

			//TESTS
			//TestRoutines::digitalLatency(fpga);
			//TestRoutines::analogLatency(fpga);
			//TestRoutines::pixelclock(fpga);
			//TestRoutines::digitalTiming(fpga);
			//TestRoutines::analogAndDigitalOut(fpga);
			//TestRoutines::analogRamp(fpga);

			//TestRoutines::fineTuneScanGalvo(fpga);
			//TestRoutines::resonantScanner(fpga);
			//TestRoutines::galvosSync(fpga);
			//TestRoutines::galvosLaserSync(fpga);

			//TestRoutines::stagePosition();
			//TestRoutines::stageConfig();

			//TestRoutines::shutter(fpga);
			//TestRoutines::pockels(fpga);
			//TestRoutines::semiAutomatedPockelsCalibration(fpga);
			//TestRoutines::pockelsRamp(fpga);
			//TestRoutines::lasers(fpga);
			//TestRoutines::virtualLasers(fpga);
			//TestRoutines::photobleach(fpga);

			//TestRoutines::convertI16toVolt();
			//TestRoutines::ethernetSpeed();
			//TestRoutines::multithread();
			//TestRoutines::clipU8();
			//TestRoutines::dataLogger();
			//TestRoutines::createSubfolder();
			//TestRoutines::filterTileConfigurationFile(0, 52);

			//TestRoutines::correctImageSingle();
			//TestRoutines::correctImageBatch();
			//TestRoutines::quickStitcher();
			//TestRoutines::boolmap();

			//TestRoutines::sequencerConcurrentTest();

			//TestRoutines::PMT16Xconfig();
			//TestRoutines::PMT16Xdemultiplex(fpga);

			//TestRoutines::vibratome(fpga);
			//TestRoutines::filterwheel();
			//TestRoutines::collectorLens();;
		}
		catch (const std::invalid_argument &e)
		{
			std::cout << "An invalid argument has occurred in " << e.what() << "\n";
			Util::pressAnyKeyToCont();
		}
		catch (const std::overflow_error &e)
		{
			std::cout << "An overflow has occurred in " << e.what() << "\n";
			Util::pressAnyKeyToCont();
		}
		catch (const FPGAexception &e)
		{
			std::cout << "An FPGA exception has occurred in " << e.what() << "\n";
			Util::pressAnyKeyToCont();
		}
		catch (const std::runtime_error &e)
		{
			std::cout << "A runtime error has occurred in " << e.what() << "\n";
			Util::pressAnyKeyToCont();
		}
		catch (const std::out_of_range& e)
		{
			std::cout << "An out of error has occurred in " << e.what() << "\n";
			Util::pressAnyKeyToCont();
		}
		catch (...)
		{
			std::cout << "An unknown error has occurred" << "\n";
			Util::pressAnyKeyToCont();
		}

		fpga.close(FPGARESET::DIS);		//Close the FPGA connection
	}
	//Catch exceptions thrown by the constructor FPGA
	catch (const FPGAexception &e)
	{
		std::cout << "An FPGA exception has occurred in " << e.what() << "\n";
		Util::pressAnyKeyToCont();
	}
	return 0;
}


/*
//For debugging. main() without calling the fpga because LV blocks the access to it
int main(int argc, char* argv[])
{
	try
	{
		//Make sure that the power supply of the FPGA is up
		TestRoutines::PMT16Xconfig();
	}
	catch (...)
	{
		std::cout << "An error has occurred" << "\n";
	}
}
*/



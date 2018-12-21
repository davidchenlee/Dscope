#pragma once
#include <fstream>					//file management
#include <ctime>					//Clock()
#include <algorithm>				//std::max and std::min
#include "FPGAapi.h"
#include "PI_GCS2_DLL.h"
#include "serial/serial.h"

class Image
{
	FPGAns::RTcontrol &mRTcontrol;			//Const because the variables referenced by mRTcontrol are not changed by the methods in this class
	U32* mBufArrayA;						//Vector to read FIFOOUTpc A
	U32* mBufArrayB;						//Vector to read FIFOOUTpc B
	TiffU8 mTiff;							//Tiff that store the content of mBufArrayA and mBufArrayB after demultiplexing

	void startFIFOOUTpc_() const;
	void configureFIFOOUTpc_(const U32 depth) const;	//Currently I don't use this function
	void stopFIFOOUTpc_() const;
	void readFIFOOUTpc_();
	void readChunk_(int &nElemRead, const NiFpga_FPGAvi_TargetToHostFifoU32 FIFOOUTpc, U32* buffer, int &timeout);
	void correctInterleaved_();
	void demultiplex_();
	void FIFOOUTpcGarbageCollector_() const;
public:
	Image(FPGAns::RTcontrol &RTcontrol);
	~Image();
	Image(const Image&) = delete;				//Disable copy-constructor
	Image& operator=(const Image&) = delete;	//Disable assignment-constructor
	Image(Image&&) = delete;					//Disable move constructor
	Image& operator=(Image&&) = delete;			//Disable move-assignment constructor

	//const methods do not change the class members. The variables referenced by mRTcontrol can be modifiede, but not mRTcontrol itself
	void acquire();
	void initialize();
	void startFIFOOUTpc();
	void download();
	void mirrorOddFrames();
	void average();
	void saveTiffSinglePage(std::string filename, const OverrideFileSelector overrideFlag, const StackScanDir stackScanDir = TOPDOWN) const;
	void saveTiffMultiPage(std::string filename, const OverrideFileSelector overrideFlag = NOOVERRIDE, const StackScanDir stackScanDir = TOPDOWN) const;
	unsigned char* const pointerToTiff() const;
};

class ImageException : public std::runtime_error
{
public:
	ImageException(const std::string& message) : std::runtime_error(message.c_str()) {}
};

class Vibratome
{
	enum MotionDir { BACKWARD = -1, FORWARD = 1 };
	const FPGAns::FPGA &mFpga;
	double mCuttingSpeed_mmps = 0.5;	//in mm/s. Speed of the vibratome for cutting (manual setting)
	double mMovingSpeed_mmps = 2.495;	//in mm/s. Forward and backward moving speed of the head. 52.4 mm in 21 seconds = 2.495 mm/s
	//double mTravelRange = 52.4 * mm;	//(horizontal) travel range of the head. I measured 104.8 seconds at 0.5 mm/s = 52.4 mm

	void moveHead_(const double duration, const MotionDir motionDir) const;
	void run() const;
public:
	Vibratome(const FPGAns::FPGA &fpga);
	void cutAndRetractDistance(const double distance) const;
	void retractDistance(const double distance) const;
};

class ResonantScanner
{
	const FPGAns::RTcontrol &mRTcontrol;				//Needed to retrieve 'mRTcontrol.mWidthPerFrame_pix' to calculate the fill factor
	const double mVMAX = 5 * V;							//Max control voltage allowed
	const double mDelay = 10 * ms;
	const double mVoltagePerDistance = 0.00595 * V/um;	//Calibration factor. Last calibrated 
	double mFullScan;									//Full scan = distance between turning points
	double mControlVoltage;								//Control voltage 0-5V

	void setVoltage_(const double controlVoltage);
public:
	ResonantScanner(const FPGAns::RTcontrol &RTcontrol);
	double mFillFactor;									//Fill factor: how much of an RS swing is covered by the pixels
	double mFFOV;										//Current FFOV
	double mSampRes_umPerPix;							//Spatial sampling resolution (um per pixel)

	void setFFOV(const double FFOV);
	void turnOn(const double FFOV);
	void turnOnUsingVoltage(const double controlVoltage);
	void turnOff();
	double downloadControlVoltage();
	double getSamplingResolution_umPerPix();
	void isRunning();
};

class Galvo
{
	FPGAns::RTcontrol &mRTcontrol;								//Non-const because some of methods in this class change the variables referenced by mRTcontrol	
	RTchannel mGalvoRTchannel;
	const double mVoltagePerDistance = 0.02417210 * V/um;		//volts per um. Calibration factor of the galvo. Last calib 31/7/2018
public:
	Galvo(FPGAns::RTcontrol &RTcontrol, const RTchannel galvoChannel);
	//const methods do not change the class members. The variables referenced by mRTcontrol could change, but not mRTcontrol
	void voltageLinearRamp(const double timeStep, const double rampLength, const double Vi, const double Vf) const;
	void positionLinearRamp(const double timeStep, const double rampLength, const double xi, const double xf) const;
	void voltageToZero() const;
	void pushVoltageSinglet(const double timeStep, const double AO) const;
};

class PMT16X
{
	serial::Serial *mSerial;
	std::string mPort = assignCOM.at(COMPMT16X);
	const int mBaud = 9600;
	const int mTimeout = 300 * ms;
	const int mRxBufferSize = 256;				//Serial buffer size

	uint8_t sumCheck_(const std::vector<uint8_t> input, const int index) const;		//The PMT requires a sumcheck. Refer to the manual
	std::vector<uint8_t> sendCommand_(std::vector<uint8_t> command) const;
public:
	PMT16X();
	~PMT16X();
	PMT16X(const PMT16X&) = delete;				//Disable copy-constructor
	PMT16X& operator=(const PMT16X&) = delete;	//Disable assignment-constructor
	PMT16X(PMT16X&&) = delete;					//Disable move constructor
	PMT16X& operator=(PMT16X&&) = delete;		//Disable move-assignment constructor

	void readAllGain() const;
	void setSingleGain(const int channel, const int gain) const;
	void setAllGainToZero() const;
	void setAllGain(const int gain) const;
	void setAllGain(std::vector<uint8_t> gains) const;
	void readTemp() const;
};

class Filterwheel
{
	FilterwheelID mDeviceID;				//Device ID = 1, 2, ...
	std::string mDeviceName;				//Device given name
	serial::Serial *mSerial;
	std::string mPort;
	const int mBaud = 115200;
	const int mTimeout = 150 * ms;
	Filtercolor mColor;
	const int mNpos = 6;					//Nunmber of filter positions
	const double mTuningSpeed_Hz = 0.8;		//The measured filterwheel tuning speed is ~ 1 position/s. Choose a slightly smaller value
	const int mRxBufSize = 256;				//Serial buffer size

	std::string convertToString_(const Filtercolor color) const;
	void downloadColor_();
public:
	Filterwheel(const FilterwheelID ID);
	~Filterwheel();
	Filterwheel(const Filterwheel&) = delete;				//Disable copy-constructor
	Filterwheel& operator=(const Filterwheel&) = delete;	//Disable assignment-constructor
	Filterwheel(Filterwheel&&) = delete;					//Disable move constructor
	Filterwheel& operator=(Filterwheel&&) = delete;			//Disable move-assignment constructor

	void setColor(const Filtercolor color);
	void setColor(const int wavelength_nm);
};

class Laser
{
	std::string mLaserNameString;
	RTchannel mLaserID;
	int mWavelength_nm;
	serial::Serial *mSerial;
	std::string mPort;
	int mBaud;
	const int mTimeout = 100 * ms;
	const double mTuningSpeed_nmPerS = 35;		//in nm per second. The measured laser tuning speed is ~ 40 nm/s. Choose a slightly smaller value
	const int mRxBufSize = 256;					//Serial buffer size

	int downloadWavelength_nm_();
public:
	Laser(RTchannel laserID);
	~Laser();
	Laser(const Laser&) = delete;				//Disable copy-constructor
	Laser& operator=(const Laser&) = delete;	//Disable assignment-constructor
	Laser(Laser&&) = delete;					//Disable move constructor
	Laser& operator=(Laser&&) = delete;			//Disable move-assignment constructor

	void printWavelength_nm() const;
	void setWavelength(const int wavelength_nm);
	void setShutter(const bool state) const;
};

class Shutter
{
	const FPGAns::FPGA &mFpga;
	NiFpga_FPGAvi_ControlBool mDeviceID;						//Device ID
public:
	Shutter(const FPGAns::FPGA &fpga, RTchannel laserID);		//I use RTchannel to re-use the laserID. The shutters are not RT
	~Shutter();
	void setShutter(const bool state) const;
	void pulse(const double pulsewidth) const;
};

class PockelsCell
{
	FPGAns::RTcontrol &mRTcontrol;				//Non-const because some methods in this class change the variables referenced by mRTcontrol						
	RTchannel mPockelsRTchannel;
	RTchannel mScalingRTchannel;
	int mWavelength_nm;							//Laser wavelength
	const double maxPower = 250 * mW;			//Soft limit for the laser power
	Shutter mShutter;

	double convertLaserpowerToVolt_(const double power) const;
public:
	//Do not set the output to 0 through the destructor to allow latching the last value
	PockelsCell(FPGAns::RTcontrol &RTcontrol, const RTchannel laserID, const int wavelength_nm);

	//const methods do not change the class members. The variables referenced by mRTcontrol could change, but not mRTcontrol
	void pushVoltageSinglet(const double timeStep, const double AO) const;
	void pushPowerSinglet(const double timeStep, const double P, const OverrideFileSelector overrideFlag = NOOVERRIDE) const;
	void voltageLinearRamp(const double timeStep, const double rampDuration, const double Vi, const double Vf) const;
	void powerLinearRamp(const double timeStep, const double rampDuration, const double Pi, const double Pf) const;
	void voltageToZero() const;
	void scalingLinearRamp(const double Si, const double Sf) const;
	void setShutter(const bool state) const;
};

class VirtualLaser
{
	RTchannel mLaserID;		//Keep track of the current laser being used
	int mWavelength_nm;		//Keep track of the current wavelength being used
	Laser mVision;
	Laser mFidelity;
	PockelsCell mPockelsVision;
	PockelsCell mPockelsFidelity;
	Filterwheel mFWexcitation;
	Filterwheel mFWdetection;
public:
	VirtualLaser(FPGAns::RTcontrol &RTcontrol, const int wavelength_nm, const double power);
	void setWavelength(const int wavelength_nm);
	void pushPowerSinglet(const double timeStep, const double P, const OverrideFileSelector overrideFlag = NOOVERRIDE) const;
	void setShutter(const bool state) const;
};

class Stage
{
	enum StageDOparam { TriggerStep = 1, AxisNumber = 2, TriggerMode = 3, Polarity = 7, StartThreshold = 8, StopThreshold = 9, TriggerPosition = 10 };
	enum StageDOtriggerMode { PositionDist = 0, OnTarget = 2, InMotion = 6, PositionOffset = 7 };

	const int mPort_z = 4;								//COM port
	const int mBaud_z = 38400;
	int3 mID;											//Controller IDs
	const char mNstagesPerController[2] = "1";			//Number of stages per controller (currently 1)
	double3 mPositionXYZ_mm;							//Absolute position of the stages (x, y, z)
	const double3 mSoftPosMinXYZ_mm{ -60, 0, 1 };		//Stage soft limits, which do not necessarily coincide with the values set in hardware (stored in the internal memory of the stages)
	const double3 mSoftPosMaxXYZ_mm{ 50, 30, 25 };
	const std::vector<double2> mStagePosLimitXYZ_mm{ {-65,65},{-30,30},{0,26} };	//Position range of the stages
	int3 mNtile;									//Tile number in x, y, z
	int3 mNtileOverlap_pix;							//Tile overlap in x, y, z	

	void configVelAndDOtriggers_(const double3 velXYZ_mmps) const;
public:
	Stage(const double3 vel_mmps);
	~Stage();
	Stage(const Stage&) = delete;				//Disable copy-constructor
	Stage& operator=(const Stage&) = delete;	//Disable assignment-constructor
	Stage(Stage&&) = delete;					//Disable move constructor
	Stage& operator=(Stage&&) = delete;			//Disable move-assignment constructor

	double3 readPositionXYZ_mm() const;
	void printPositionXYZ() const;
	void moveSingleStage(const Axis stage, const double position_mm);
	void moveAllStages(const double3 positionXYZ_mm);
	double downloadPosition_mm(const Axis axis);
	bool isMoving(const Axis axis) const;
	void waitForMotionToStopSingleStage(const Axis axis) const;
	void waitForMotionToStopAllStages() const;
	void stopAllstages() const;
	double downloadSingleVelocity_mmps(const Axis axis) const;
	void setSingleVelocity(const Axis axis, const double vel_mmps) const;
	void setAllVelocities(const double3 vel_mmps) const;
	void setDOtriggerSingleParam(const Axis axis, const int DOchan, const StageDOparam paramId, const double value) const;
	void setDOtriggerAllParams(const Axis axis, const int DOchan, const double triggerStep_mm, const StageDOtriggerMode triggerMode, const double startThreshold_mm, const double stopThreshold_mm) const;
	double downloadDOtriggerSingleParam(const Axis axis, const int DOchan, const StageDOparam paramId) const;
	bool isDOtriggerEnabled(const Axis axis, const int DOchan) const;
	void setDOtriggerEnabled(const Axis axis, const int DOchan, const BOOL triggerState) const;
	void printStageConfig(const Axis axis, const int DOchan) const;
};


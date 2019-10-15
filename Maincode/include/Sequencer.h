#pragma once
#include "Utilities.h"
#include "Devices.h"
#include "SampleConfig.h"
using namespace Constants;
																																														
void reverseSCANDIR(SCANDIR &scanDir);
POSITION2 determineRelativeTileIndicesIJ(const TILEOVERLAP3 overlapIJK_frac, const TILEDIM2 tileArraySizeIJ, const TILEIJ tileIndicesIJ);
double determineInitialScanPos(const double posMin, const double travel, const double travelOverhead, const SCANDIR scanDir);
double determineFinalScanPos(const double posMin, const double travel, const double travelOverhead, const SCANDIR scanDir);
double determineInitialLaserPower(const double powerMin, const double totalPowerInc, const SCANDIR scanDir);
double determineFinalLaserPower(const double powerMin, const double totalPowerInc, const SCANDIR scanDir);

class TileArray
{
public:
	enum Axis { II, JJ, KK };		//Currently, the tile axis II coincides with AXIS::XX, JJ with AXIS::YY, and KK with AXIS::ZZ
	TileArray(const int tileHeight_pix, const int tileWidth_pix, const TILEDIM2 tileArraySizeIJ, const TILEOVERLAP3 overlapIJK_frac);
	TileArray(const PIXDIM2 tileSize_pix, const TILEDIM2 tileArraySizeIJ, const TILEOVERLAP3 overlapIJK_frac);
	int readTileHeight_pix() const;
	int readTileWidth_pix() const;
	TILEDIM2 readTileArraySizeIJ() const;
	int readTileArraySizeIJ(const TileArray::Axis axis) const;
	TILEOVERLAP3 readTileOverlapIJK_frac() const;
	PIXELij determineTileRelativePixelPos_pix(const TILEIJ tileIndicesIJ) const;
protected:
	const int mTileHeight_pix;		//Pixel height of a single tile
	const int mTileWidth_pix;		//Pixel width of a single tile
	TILEDIM2 mArraySizeIJ;			//Dimension of the array of tiles
	TILEOVERLAP3 mOverlapIJK_frac;
};

class QuickStitcher : public TiffU8, public TileArray
{
public:
	QuickStitcher(const int tileHeight_pix, const int tileWidth_pix, const TILEDIM2 tileArraySizeIJ, const TILEOVERLAP3 overlapIJK_frac);
	void push(const U8 *tile, const TILEIJ tileIndicesIJ);
	void saveToFile(std::string filename, const OVERRIDE override) const;
	int readFullHeight_pix() const;
	int readFullWidth_pix() const;
private:
	int mFullHeight;
	int mFullWidth;
};

class QuickScanXY final: public QuickStitcher
{
public:
	QuickScanXY(const POSITION2 ROIcenterXY, const FFOV2 ffov, const LENGTH2 pixelSizeXY, const LENGTH2 LOIxy);
	double determineInitialScanPosX(const double travelOverhead, const SCANDIR scanDir) const;
	double determineFinalScanPosX(const double travelOverhead, const SCANDIR scanDir) const;
	int readNstageYpos() const;
	double readStageYposFront() const;
	double readStageYposBack() const;
	double readStageYposAt(const int index) const;
private:
	std::vector<double> mStageYpos;
	const POSITION2 mROIcenterXY;
	const FFOV2 mFFOV;
	const LENGTH2 mPixelSizeXY;
	const LENGTH2 mLOIxy;
	const int mFullWidth_pix;

	int castToOddnumber_(const double input) const;
	LENGTH2 castLOIxy_(const FFOV2 FFOV, const LENGTH2 LOIxy) const;
};

class Boolmap final
{
public:
	Boolmap(const TiffU8 &tiff, const PIXDIM2 tileSize_pix, const TILEOVERLAP3 overlapIJK_frac, const double threshold);
	Boolmap(const QuickScanXY &quickScanXY, const PIXDIM2 tileSize_pix, const TILEOVERLAP3 overlapIJK_frac, const double threshold);
	bool isTileBright(const TILEIJ tileIndicesIJ) const;
	void saveTileMapToText(std::string filename);
	void saveTileGridOverlap(std::string filename, const OVERRIDE override = OVERRIDE::DIS) const;
	void saveTileMap(std::string filename, const OVERRIDE override = OVERRIDE::DIS) const;
private:
	const TiffU8 mTiff;
	const TileArray mTileArray;
	const double mThreshold;			//Threshold for generating the boolmap
	const int mFullHeight_pix;			//Pixel height of the tiled image
	const int mFullWidth_pix;			//Pixel width of the tiled image
	const int mNpixFull;				//Total number of pixels in mTiff
	PIXELij mAnchorPixel_pix;			//Reference position for the tile array wrt the Tiff
	std::vector<bool> mIsBrightMap;

	PIXELij determineTileAbsolutePixelPos_pix_(const TILEIJ tileIndicesIJ) const;
	bool isQuadrantBright_(const double threshold, const TILEIJ tileIndicesIJ) const;
	void generateBoolmap_();
};

class Stack final
{
public:
	Stack(const FFOV2 FFOV, const int tileHeight_pix, int const tileWidth_pix, const double pixelSizeZ, const int nFrames, const TILEOVERLAP3 overlapIJK_frac);
	void printParams(std::ofstream *fileHandle) const;
	double readFFOV(const AXIS axis) const;
	int readTileHeight_pix() const;
	int readTileWidth_pix() const;
	double readPixelSizeZ() const;
	double readDepthZ() const;
	TILEOVERLAP3 readOverlapIJK_frac() const;
	double readOverlap_frac(const TileArray::Axis axis) const;
private:
	FFOV2 mFFOV;					//Full field of view in the X-stage and Y-stage axes
	int mTileHeight_pix;
	int mTileWidth_pix;
	double mPixelSizeZ;				//Image resolution in the Z-stage axis
	double mDepthZ;					//Stack depth or thickness
	TILEOVERLAP3 mOverlapIJK_frac;	//Stack overlap in the X-stage, Y-stage, and Z-stage axes
};

namespace Action
{
	enum class ID { CUT, ACQ, SAV, MOV };
	class MoveStage final
	{
	public:
		void setParam(const int sliceNumber, const TILEIJ tileIndicesIJ, const POSITION2 tileCenterXY);
		int readSliceNumber() const;
		int readTileIndex(const TileArray::Axis axis) const;
		POSITION2 readTileCenterXY() const;
		double readTileCenter(AXIS axis) const;
	private:
		int mSliceNumber;			//Slice number
		TILEIJ mTileIndicesIJ;		//Indices of the tile array
		POSITION2 mTileCenterXY;	//X-stage and Y-stage positions corresponding to the center of the tile
	};

	class AcqStack final
	{
	public:
		void setParam(const int stackNumber, const int stackIndex, const int wavelength_nm, const SCANDIR scanDirZ, const double scanZmin, const double depthZ, const double scanPmin, const double scanPexp, const int nFrameBinning);
		int readStackNumber() const;
		int readStackIndex() const;
		int readWavelength_nm() const;
		SCANDIR readScanDirZ() const;
		double readScanZmin() const;
		double readDepthZ() const;
		double readScanPmin() const;
		double readScanPexp() const;
		int readNframeBinning() const;
	private:
		int mStackNumber;		//Number of the stack following the scan pattern (e.g. snake)
		int mStackIndex;		//Number of the stack column by column from top to bottom and left to right (used for saving the tiff)
		int mWavelength_nm;
		SCANDIR mScanDirZ;		//THIS IS NOT READ BY THE SEQUENCER ANYMORE!!
		double mScanZmin;		//Min z position of a stack scan
		double mDepthZ;			//Stack depth (thickness)
		double mScanPmin;		//Min laser power of the stack scan (at the top of the stack)
		double mScanPexp;		//Laser power increase in the Z-stage axis per unit of distance
		int mNframeBinning;
	};

	class CutSlice final
	{
	public:
		void setParam(const double planeZtoCut, const double stageZheightForFacingTheBlade);
		double readPlaneZtoCut() const;
		double readStageZheightForFacingTheBlade() const;
	private:
		double mPlaneZtoCut;
		double mStageZheightForFacingTheBlade;
	};
}

//A list of commands that form a full sequence
class Sequencer final 
{
public:
	class Commandline final	//Single commands
	{
	public:
		Action::ID mActionID;
		union {
			Action::MoveStage moveStage;
			Action::AcqStack acqStack;
			Action::CutSlice cutSlice;
		} mAction;

		Commandline(const Action::ID actionID);
		void printToFile(std::ofstream *fileHandle) const;
		void printParameters() const;
	private:
		std::string convertActionIDtoString_(const Action::ID actionID) const;
	};
	Sequencer(const Sample sample, const Stack stack);
	Sequencer(const Sequencer&) = delete;					//Disable copy-constructor
	Sequencer& operator=(const Sequencer&) = delete;		//Disable assignment-constructor
	Sequencer(Sequencer&&) = delete;						//Disable move constructor
	Sequencer& operator=(Sequencer&&) = delete;				//Disable move-assignment constructor

	void generateCommandList();
	POSITION2 convertTileIndicesIJToStagePosXY(const TILEIJ tileIndicesIJ) const;
	std::string printHeader() const;
	std::string printHeaderUnits() const;
	void printSequenceParams(std::ofstream *fileHandle) const;
	void printToFile(std::string fileName, const OVERRIDE override) const;
	int readTotalNumberOfSlices() const;
	int readNumberOfStacksPerSlice() const;
	int readTotalNumberOfStacks() const;
	int readNtotalCommands() const;
	Commandline readCommandline(const int iterCommandline) const;
	int readTileArraySizeIJ(const TileArray::Axis axis) const;
private:
	const Sample mSample;									//Sample
	const Stack mStack;										//Stack
	std::vector<Commandline> mCommandList;
	int mII{ 0 };											//Tile iterator for the X-stage
	int mJJ{ 0 };											//Tile iterator for the Y-stage
	int mCommandCounter{ 0 };
	ROI4 mROI;												//Effective ROI covered by the tile array. It could be slightly larger than the size specified by mSample.mLOIxyz_req
	int mStackCounter{ 0 };									//Count the number of stacks
	int mSliceCounter{ 0 };									//Count the number of slices
	TileArray mTileArray;
	SCANDIR3 mIterScanDirXYZ{ g_initialStageScanDirXYZ };	//Scan directions wrt the X-stage, Y-stage, and Z-stage axes	
	double mIterScanZi;										//Initial Z-stage position for a stack scan
	double mIterSamplePlaneZtoCut;							//Sample plane to cut (height of the stage Z)
	double mIterStageZheightForFacingTheBlade;				//Actual height of the stage Z for cutting the sample at mIterSamplePlaneZtoCut
															//(It defers from mIterSamplePlaneZtoCut by the height offset of the blade wrt the imaging plane)
	int mNtotalSlices;										//Number of vibratome slices in the entire sample

	void initializeVibratomeSlice_();
	TILEDIM2 determineTileArraySizeIJ_();
	void initializeEffectiveROI_();
	void reserveMemoryBlock_();
	void initializeIteratorIJ_();
	void resetStageScanDirections_();
	LENGTH3 determineEffectiveLOIxyz() const;

	void moveStage_(const TILEIJ tileIndicesIJ);
	void acqStack_(const int indexFluorMarker);
	void saveStack_();
	void cutSlice_();
};
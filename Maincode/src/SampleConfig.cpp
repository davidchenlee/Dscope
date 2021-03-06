#include "SampleConfig.h"
const extern std::vector<LIMIT2> PetridishPosLimit{ { 27. * mm, 57. * mm}, { -1. * mm, 30. * mm}, { 15. * mm, 24. * mm} };		//Soft limit of the stage for the petridish
const extern std::vector<LIMIT2> ContainerPosLimit{ { -65. * mm, 65. * mm}, { 1.99 * mm, 30. * mm}, { 10. * mm, 24. * mm} };	//Soft limit of the stage for the oil container

//const extern POSITION3 g_stackCenterXYZ{ (33.350) * mm, (16.200) * mm, (17.670) * mm };
const extern POSITION3 g_stackCenterXYZ{ (37.900) * mm, (26.0)* mm, (17.565) * mm };//For 4um beads
//const extern POSITION3 g_stackCenterXYZ{ (46.889 - 0.068 - 0.250 - 0.180 - 0.082 - 0.197 - 0.117) * mm, (16.519 + 0.034 )* mm, (19.000 - 0.018) * mm };//For 0.1um beads

#pragma region "FluorMarkerList"
FluorMarkerList::FluorMarkerList(const std::vector<FluorMarker> fluorMarkerList) :
	mFluorMarkerList{ fluorMarkerList }
{}

std::size_t FluorMarkerList::readFluorMarkerListSize() const
{
	return mFluorMarkerList.size();
}

void FluorMarkerList::printFluorParams(std::ofstream *fileHandle) const
{
	*fileHandle << "LASERS ************************************************************\n";

	for (std::vector<int>::size_type iterWL = 0; iterWL != mFluorMarkerList.size(); iterWL++)
	{
		*fileHandle << "Wavelength = " << mFluorMarkerList.at(iterWL).mWavelength_nm <<
			" nm\nPower = " << mFluorMarkerList.at(iterWL).mScanPmin / mW <<
			" mW\nPower exponential length = " << mFluorMarkerList.at(iterWL).mScanPLexp / um << " um\n";
	}
	*fileHandle << "\n";
}

//Return the first instance of "fluorMarker" in mFluorMarkerList
FluorMarkerList::FluorMarker FluorMarkerList::findFluorMarker(const std::string fluorMarker) const
{
	for (std::vector<int>::size_type iterMarker = 0; iterMarker < mFluorMarkerList.size(); iterMarker++)
	{
		if (!fluorMarker.compare(mFluorMarkerList.at(iterMarker).mName)) //compare() returns 0 if the strings are identical
			return mFluorMarkerList.at(iterMarker);
	}
	//If the requested fluorMarker is not found
	throw std::runtime_error((std::string)__FUNCTION__ + ": Fluorescent marker " + fluorMarker + " not found");
}

//indexFluorMarker is the position of the fluorMarker in mFluorMarkerList
FluorMarkerList::FluorMarker FluorMarkerList::readFluorMarker(const int indexFluorMarker) const
{
	return mFluorMarkerList.at(indexFluorMarker);
}

FluorMarkerList FluorMarkerList::readFluorMarkerList() const
{
	return mFluorMarkerList;
}
#pragma endregion "FluorMarkerList"

#pragma region "Sample"
Sample::Sample(const std::string sampleName, const std::string immersionMedium, const std::string objectiveCollar, const std::vector<LIMIT2> stageSoftPosLimXYZ, const FluorMarkerList fluorMarkerList) :
	mName{ sampleName },
	mImmersionMedium{ immersionMedium },
	mObjectiveCollar{ objectiveCollar },
	mStageSoftPosLimXYZ{ stageSoftPosLimXYZ },
	FluorMarkerList{ fluorMarkerList }
{}

Sample::Sample(const Sample& sample, const POSITION2 centerXY, const LENGTH3 LOIxyz, const double sampleSurfaceZ, const double cutOffset) :
	mName{ sample.mName },
	mImmersionMedium{ sample.mImmersionMedium },
	mObjectiveCollar{ sample.mObjectiveCollar },
	mStageSoftPosLimXYZ{ sample.mStageSoftPosLimXYZ },
	FluorMarkerList{ sample.readFluorMarkerList() },
	mCenterXY{ centerXY },
	mLOIxyz_req{ LOIxyz },
	mSurfaceZ{ sampleSurfaceZ },
	mCutAboveBottomOfStack{ cutOffset }
{
	if (mLOIxyz_req.XX <= 0)
		throw std::invalid_argument((std::string)__FUNCTION__ + ": The sample length X must be > 0");
	if (mLOIxyz_req.YY <= 0)
		throw std::invalid_argument((std::string)__FUNCTION__ + ": The sample length Y must be > 0");
	if (mCutAboveBottomOfStack < 0)
		throw std::invalid_argument((std::string)__FUNCTION__ + ": The cut offset must be >= 0");
}

void Sample::printSampleParams(std::ofstream *fileHandle) const
{
	*fileHandle << "SAMPLE ************************************************************\n";
	*fileHandle << "Name = " << mName << "\n";
	*fileHandle << "Immersion medium = " << mImmersionMedium << "\n";
	*fileHandle << "Correction collar = " << mObjectiveCollar << "\n";
	*fileHandle << std::setprecision(4);
	*fileHandle << "Sample center (stageX, stageY) = (" << mCenterXY.XX / mm << " mm, " << mCenterXY.YY / mm << " mm)\n";
	*fileHandle << "Requested ROI size (stageX, stageY, stageZ) = (" << mLOIxyz_req.XX / mm << " mm, " << mLOIxyz_req.YY / mm << " mm, " << mLOIxyz_req.ZZ / mm << " mm)\n\n";

	*fileHandle << "CUT ************************************************************\n";
	*fileHandle << std::setprecision(1);
	*fileHandle << "Blade-focal plane vertical offset = " << mBladeFocalplaneOffsetZ / um << " um\n";
	*fileHandle << "Cut above the bottom of the stack = " << mCutAboveBottomOfStack / um << " um\n";
	*fileHandle << "\n";
}

std::string Sample::readName() const
{
	return mName;
}

std::string Sample::readImmersionMedium() const
{
	return mImmersionMedium;
}

std::string Sample::readObjectiveCollar() const
{
	return mObjectiveCollar;
}

std::vector<LIMIT2> Sample::readStageSoftPosLimXYZ() const
{
	return mStageSoftPosLimXYZ;
}

double Sample::readStageSoftPosLimXMIN() const
{
	return mStageSoftPosLimXYZ.at(AXIS::XX).MIN;
}

double Sample::readStageSoftPosLimXMAX() const
{
	return mStageSoftPosLimXYZ.at(AXIS::XX).MAX;
}

double Sample::readStageSoftPosLimYMIN() const
{
	return mStageSoftPosLimXYZ.at(AXIS::YY).MIN;
}

double Sample::readStageSoftPosLimYMAX() const
{
	return mStageSoftPosLimXYZ.at(AXIS::YY).MAX;
}
#pragma endregion "Sample"


//SAMPLE PARAMETERS
//This should really go in Routines.cpp but I'll leave here for now out of convenience to avoid scrolling up and down through Routines.cpp
#if g_multibeam


//This is used with beads
#if 0 //slide
const extern POSITION3 g_stackCenterXYZ{ (34.000) * mm, (0.000) * mm, (16.600) * mm };
const extern Sample g_currentSample{ "FSlide16X", "SiliconeOil", "1.51", PetridishPosLimit, {{{"DAPI", 750, Util::multiply16X(6. * mW), Util::multiply16X(2000. * um) },
																				   { "GFP", 920, Util::multiply16X(8. * mW), Util::multiply16X(2000. * um) },
																				   { "TDT", 1040, Util::multiply16X(3. * mW), Util::multiply16X(2000. * um) } }} };

#else //beads
//const extern Sample g_currentSample{ "Beads4um16X", "SiliconeOil", "1.51", PetridishPosLimit, {{{"DAPI", 750, Util::multiply16X(45. * mW), Util::multiply16X(2000. * um) },
//																				   { "GFP", 920, Util::multiply16X(60. * mW), Util::multiply16X(2000. * um) },
//																				   { "TDT", 1040, Util::multiply16X(15. * mW), Util::multiply16X(2000. * um) } }} };

//const extern Sample g_currentSample{ "Beads01um16X", "SiliconeOil", "1.51", PetridishPosLimit, {{{"DAPI", 750, Util::multiply16X(30. * mW), Util::multiply16X(2000. * um) },
//																				                { "TDT", 1040, Util::multiply16X(15. * mW), Util::multiply16X(2000. * um) } }} };
#endif//fluorescent slide or beads

//const extern Sample g_currentSample{ "Brain", "SiliconeMineralOil5050", "1.49", ContainerPosLimit, {{ { "TDT", 1040, Util::multiply16X(60. * mW), 800. * um, 2 },
//																							          { "DAPI", 750, Util::multiply16X(13. * mW), 500. * um, 2 } }} };

const extern Sample g_currentSample{ "Liver", "SiliconeMineralOil5050", "1.49", ContainerPosLimit, {{ { "TDT", 1040, Util::multiply16X(60. * mW), 300. * um, 2 },
																							          { "DAPI", 750, Util::multiply16X(13. * mW), 150. * um, 2 } }} };

//const extern Sample g_currentSample{ "Planarian16X", "MineralOil", "1.465", ContainerPosLimit, {{{ "DAPI", 750, Util::multiply16X(20. * mW), 2000. * um, 2 } }} };

#else//singlebeam

const extern Sample g_currentSample{ "Beads4um1X", "SiliconeOil", "1.51", PetridishPosLimit, {{{"DAPI", 750, 30. * mW, 2000. * um, 1},
																				  { "GFP", 920, 50. * mW, 2000. * um, 1},
																				  { "TDT", 1040, 30. * mW, 2000. * um, 1}}} };

//const extern Sample g_currentSample{ "Planarian1X", "MineralOil", "1.465", ContainerPosLimit, {{{"DAPI", 750, 20. * mW, 140. * um, 12}}} };


//const extern Sample g_currentSample{ "Brain", "SiliconeMineralOil5050", "1.49", ContainerPosLimit,  {{{ "TDT", 1040, 30. * mW, 500. * um, 1 },
//																									  { "DAPI", 750, 8. * mW, 300. * um, 1 }}} };//120

//const extern Sample g_currentSample{ "Liver", "SiliconeMineralOil5050", "1.49", ContainerPosLimit,  {{{ "TDT", 1040, 30. * mW, 160. * um, 1 },
//																									  { "DAPI", 750, 8. * mW, 120. * um, 1 }}} };//120

//const extern Sample g_currentSample{ "Beads1um1X", "SiliconeOil", "1.51", PetridishPosLimit, {{{"DAPI", 750, 40. * mW, 0. },
//																					{ "GFP", 920, 40. * mW, 0. },
//																					{ "TDT", 1040, 15. * mW, 0. }}} };

//Sample g_currentSample{ "fluorBlue1X", "SiliconeOil", "1.51", PetridishPosLimit, {{{ "DAPI", 750, 10. * mW, 0. }}} };
																					

#endif//singlebeam or multibeam
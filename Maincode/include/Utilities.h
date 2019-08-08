#pragma once
#include <sstream>					//For std::ostringstream
#include <iomanip>					//For std::setprecision
#include <fstream>					//For std::ofstream
#include <iostream>
#include <Const.h>
#include <experimental/filesystem>	//standard method in C++14 but not C++11
#include <bitset>					//For std::bitset
#include <tiffio.h>					//Tiff files				
#include <windows.h>				//For using the ESC key
#include <CL/cl.hpp>				//OpenCL
using namespace Constants;

std::string doesFileExist(const std::string filename);
std::string toString(const double number, const int nDecimalPlaces);
void printHex(int input);
void printHex(const std::vector<uint8_t>  input);
void printHex(const std::string input);
void printBinary16(const int input);
U16 doubleToFx2p14(double n);
template<class T> inline T clip(T x, T lower, T upper);
template<class T> inline U8 clipU8top(const T x);
template<class T> inline U8 clipU8dual(const T x);
void pressAnyKeyToCont();
void pressESCforEarlyTermination();
int pnpoly(int nvert, float *vertx, float *verty, float testx, float testy);
int2 discriminator(const double2 inputArray, const double threshold);

//For saving the parameters to a text file
class Logger
{
	std::ofstream mFileHandle;
public:
	Logger(const std::string filename);
	~Logger();
	void record(const std::string description);
	void record(const std::string description, const double input);
	void record(const std::string description, const std::string input);
};

//For manipulating and saving U8 Tiff images
class TiffU8
{
	U8* mArray;
	int mWidthPerFrame;
	int mHeightPerFrame;
	int mNframes;
	int mBytesPerLine; 
	//int mStripSize;	//I think this was implemented to allow different channels (e.g., RGB) on each pixel
public:
	TiffU8(const std::string filename);
	TiffU8(const U8* inputImage, const int widthPerFrame, const int heightPerFrame, const int nFrames);
	TiffU8(const std::vector<U8> &inputImage, const int widthPerFrame, const int heightPerFrame, const int nFrames);
	TiffU8(const int width, const int height, const int nFrames);
	~TiffU8();
	U8* const data() const;
	int widthPerFrame() const;
	int heightPerFrame() const;
	int nFrames() const;
	void splitIntoFrames(const int nFrames);
	void saveToFile(std::string filename, const MULTIPAGE multipage, const OVERRIDE override = OVERRIDE::DIS, const ZSCAN scanDir = ZSCAN::TOPDOWN) const;
	void mirrorOddFrames();
	void averageEvenOddFrames();
	void averageFrames();
	void analyze() const;
	void saveTxt(const std::string fileName) const;
	void pushImage(const int frameIndex, const U8* inputArray) const;
	void pushImage(const int firstFrameIndex, const int lastFrameIndex, const U8* inputArray) const;
	void mergePMT16Xchannels(const int heightPerChannelPerFrame, const U8* inputArrayA, const U8* inputArrayB) const;
	void correctRSdistortionGPU(const double FFOVfast);
	void correctRSdistortionCPU(const double FFOVfast);
	void suppressCrosstalk(const double crosstalkRatio = 1.0);
	void flattenField(const double maxScaleFactor = 1.0);
	double2 testBrightnessUnbalance() const;
};

class TiffStack
{
	TiffU8 mArraySameZ;		//For imaging the same z plane many times and then compute the average image
	TiffU8 mArrayDiffZ;		//For imaging different z planes
public:
	TiffStack(const int widthPerFrame_pix, const int heightPerFrame_pix, const int nDiffZ, const int nSameZ);
	void pushSameZ(const int indexSameZ, const U8* data);
	void pushDiffZ(const int indexDiffZ);
	void saveToFile(const std::string filename, OVERRIDE override) const;
};


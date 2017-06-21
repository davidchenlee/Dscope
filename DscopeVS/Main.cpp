//#include <stdio.h>
//#include <stdlib.h> //For malloc
//#include <ansi_c.h>
//#include <limits.h>
//#include <math.h>
//#include <time.h>
#include "windows.h"		//the stages use this lib. also Sleep
#include "NiFpga_FPGA.h"
#include <iostream>
#include "main.h"
//#include <concrt.h> 	//Concurrency::wait(2000);



/*Define the full path of the bitfile*/
static const char* const Bitfile = "D:\\OwnCloud\\Codes\\Dscope\\DscopeVS\\LabView\\FPGA Bitfiles\\" NiFpga_FPGA_Bitfile;

void printHex(int16_t input)
{
	std::cout << std::hex << std::uppercase << input << std::nouppercase << std::dec << std::endl;
}


void linearRamp(uint32_t *aa, tt_t ti, double Vi, tt_t tf, double Vf)
{

	
	tt_t dt = AO_dt;  //Analog output time increament in us
	int nPoints = (int)((1.*tf - ti) / dt); //number of points

	tt_t *tPoints = new tt_t[nPoints]; //time points in us
	double *vPoints = new double[nPoints]; //voltage points in V

	if (nPoints <= 1)
	{
		aa[0] = 0;
		std::cout << "ERROR: not enought points for the analog linear ramp\n";
		std::cout << "nPoints: " << nPoints << "\n";
	}
	else
	{
		for (int ii = 0; ii < nPoints; ii++)
		{
			tPoints[ii] = ti + ii * dt; //just for debugging 
			vPoints[ii] = Vi + (Vf - Vi)*ii / (nPoints - 1);
			aa[ii] = dt*tickPerUs << Abits | (LSBmask & v2hex(vPoints[ii]));
		}

		delete[] tPoints;
		delete[] vPoints;
	}

	//for debugging
	if (0)
	{
		std::cout << "nPoints: " << nPoints << "\n";

		std::cout << "time \tticks \tv \n";
		for (int ii = 0; ii < nPoints; ii++)
		{
			std::cout << tPoints[ii] << "\t" << ii * dt*tickPerUs << "\t" << vPoints[ii] << "\t" << "\n";
			//std::cout << ii << "\t" << aa[ii] << "\n";
		}
		getchar();
	}

	
}





int main()
{
	/* must be called before any other FPGA calls */
	NiFpga_Status status = NiFpga_Initialize();

	std::cout << "FPGA status: " << status << "\n";

	/* check for any FPGA error */
	if (NiFpga_IsNotError(status))
	{
		NiFpga_Session session;

		/* opens a session, downloads the bitstream, but does not run the FPGA */
		NiFpga_MergeStatus(&status, NiFpga_Open(Bitfile, NiFpga_FPGA_Signature, "RIO0", NiFpga_OpenAttribute_NoRun, &session));

		if (NiFpga_IsNotError(status))
		{
			NiFpga_MergeStatus(&status, NiFpga_WriteU16(session, NiFpga_FPGA_ControlU16_InitialWaittick, initialWait));

			/*DELAY. Sync AO and DO by delaying DO*/
			NiFpga_MergeStatus(&status, NiFpga_WriteU16(session, NiFpga_FPGA_ControlU16_DOFIFODelaytick, DOfifoDelayTick));
			NiFpga_MergeStatus(&status, NiFpga_WriteU16(session, NiFpga_FPGA_ControlU16_AOCalibratetick, calibrateAOtiming));
			NiFpga_MergeStatus(&status, NiFpga_WriteU16(session, NiFpga_FPGA_ControlU16_DOCalibratetick, calibrateDOtiming));

			tt_t ti = 0*us;
			tt_t tf = 6*us;
			double vi = 0;
			double vf = 10;
			int nPoints = (int)((1.*tf - ti) / AO_dt);
			

			/*AO1 FIFO*/
			size_t rAO1; //empty elements remaining


			//size_t sizeFifo = nPoints;
			size_t sizeFifo = 4;
			uint32_t *AOfifo = new uint32_t[sizeFifo];
			for (int i = 0; i < sizeFifo; i++) { //initialize the array
				AOfifo[i] = 0;
			}

			if (1)
			{
				tt_t At0 = us2tick(4*us);//40 tick = 1 us
				tt_t At1 = us2tick(1000*us);
				tt_t At2 = us2tick(4*us);
				tt_t At3 = us2tick(4*us);
				int16_t Vout0 = v2hex(10);
				int16_t Vout1 = v2hex(0);
				int16_t Vout2 = v2hex(10);
				int16_t Vout3 = v2hex(0);

				AOfifo[0] = (At0 << Abits) | (LSBmask & Vout0);
				AOfifo[1] = (At1 << Abits) | (LSBmask & Vout1);
				AOfifo[2] = (At2 << Abits) | (LSBmask & Vout2);
				AOfifo[3] = (At3 << Abits) | (LSBmask & Vout3);
			}
			else {
				linearRamp(AOfifo, ti, vi, tf, vf);
			}
			
			NiFpga_MergeStatus(&status, NiFpga_WriteFifoU32(session, NiFpga_FPGA_HostToTargetFifoU32_A0FIFO, AOfifo, sizeFifo, timeout, &rAO1)); //send the AO data


			/*DO FIFO*/
			size_t rDO1; //empty elements remaining
			uint32_t *DOfifo = new uint32_t[sizeFifo];
			for (int i = 0; i < sizeFifo; i++) { //initialize the array
				DOfifo[i] = 0;
			}
			
			tt_t Dt0 = us2tick(4*us); //40 tick = 1 us
			tt_t Dt1 = us2tick(1000*us);
			tt_t Dt2 = us2tick(4*us);
			tt_t Dt3 = us2tick(4*us);
			DOfifo[0] = (Dt0 << Abits) | (LSBmask & 0x0001);
			DOfifo[1] = (Dt1 << Abits) | (LSBmask & 0x0000);
			//DOfifo[2] = (Dt2 << Abits) | (LSBmask & 0x0001);
			//DOfifo[3] = (Dt3 << Abits) | (LSBmask & 0x0000);

			NiFpga_MergeStatus(&status, NiFpga_WriteFifoU32(session, NiFpga_FPGA_HostToTargetFifoU32_DOFIFO, DOfifo, sizeFifo, timeout, &rDO1)); //send the DO data

			/* run the FPGA application.*/
			NiFpga_MergeStatus(&status, NiFpga_Run(session, 0));

			/*trigger the FPGA*/
			NiFpga_Bool start = 1;
			NiFpga_MergeStatus(&status, NiFpga_WriteBool(session, NiFpga_FPGA_ControlBool_start, start));


			/* close the session. THIS TURNS OFF THE OUTPUT OF THE FPGA */
			Sleep(1500);//temp hack to let the FPGA finish before shutting it down
		
			NiFpga_MergeStatus(&status, NiFpga_Close(session, 0));

			/*cleanup*/
			delete[] AOfifo;
			delete[] DOfifo;
		}


		/* must be called after all other calls */
		NiFpga_MergeStatus(&status, NiFpga_Finalize());
	}



	return 0;
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
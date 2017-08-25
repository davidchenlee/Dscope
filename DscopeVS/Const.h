#pragma once
#include <queue>
#include <vector>
//#include <limits.h>		//for _I16_MAX??

typedef   signed    char	I8;
typedef unsigned    char	U8;
typedef            short	I16;
typedef unsigned   short	U16;
typedef              int	I32;
typedef unsigned     int	U32;
typedef          __int64	I64;
typedef unsigned __int64	U63;
typedef std::queue<uint32_t> U32Q;			//Queue of unsigned integers
typedef std::vector<U32Q> U32QV;			//Vector of queues of unsigned integers

namespace Const
{
	extern const U8 Nchan;
	extern const U8 AO0;
	extern const U8 AO1;
	extern const U8 DO0;
	extern const U8 PCLOCK;

	extern const U32 us;
	extern const U32 ms;
	extern const U32 s;
	extern const U32 tickPerUs;
	extern const double tstep;
	extern const U32 AO_dt;
	extern const U16 DODelayTick;
	extern const U16 FIFOtimeout;

	extern const U8 Npulses;
	extern const U8 pulseArray[];

	extern const U16 Nmaxlines;
	extern const U16 Npixels;
};

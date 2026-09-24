#ifndef __FARM_COMMON__
#define __FARM_COMMON__

inline U64 GetSystemTimeAsU64()
{
	return ost::getTicks();

//	U64 time;
//	GetSystemTimeAsFileTime((FILETIME*) &time );
//	return time;
}


#endif

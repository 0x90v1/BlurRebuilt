#ifndef FAST_SELECT_H
#define FAST_SELECT_H

namespace SharedPhysics
{

template<class T_Source>
NOALIAS inline T_Source* __restrict Fast_Select(const bool b, const T_Source* const __restrict pIfTrue, const T_Source* const __restrict pIfFalse)
{
	const U32 selector = ~(((U32)b)-1);
	const U32 pTrue((U32)pIfTrue);
	const U32 pFalse((U32)pIfFalse);
	return (T_Source*)((pTrue & selector) | (pFalse & (~selector)));
}


};

#endif

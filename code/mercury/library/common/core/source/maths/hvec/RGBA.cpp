#include "CorePch.h"
//#include <core/Core.h>
//#include <core/Maths.h>

#ifdef PLATFORM_XENON
#include <xtl.h>
#endif

namespace Maths
{

#if defined( PLATFORM_XENON )
#if _XDK_VER < 3424
const __vector4 UnpackRGBA_ScaleOffset = XM_UNPACK_UNSIGNEDN_SCALE_OFFSET( 8, 8, 8, 8 );
#else
const __vector4 UnpackRGBA_ScaleOffset = XM_UNPACK_UNSIGNEDN_OFFSET( 8, 8, 8, 8 );
#endif
const __vector4 PackRGBA_Scale = XM_PACK_UNSIGNEDN_SCALE( 8, 8, 8, 8 );
#endif


//#ifdef SHOOTER_VECMAT_DEBUG_PRINT
//void RGBA::DbgPrint( char* name ) const
//{
//    char buffer[256];
//    sprintf_s( buffer, 256, "%s RGBA( %02X, %02X, %02X, %02X )\n", name, R(), G(), B(), A() );
//    OutputDebugString( buffer );
//}
//#endif

}; // namespace Maths

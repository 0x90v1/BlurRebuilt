
#pragma once

#if !defined(CFG_SPEED) && defined(PLATFORM_XENON)

void EnableFloatExceptions(void);
void DisableFloatExceptions(void);

#else

inline EnableFloatExceptions(void) {}
inline DisableFloatExceptions(void) {}

#endif
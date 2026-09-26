#ifndef SAMPLES_XB2APPLICATION_H
#define SAMPLES_XB2APPLICATION_H

#ifdef _MSC_VER
#pragma once
#endif

#include "Samples/System/Application.h"

namespace Samples
{

class C_XB2Application : public C_Application
{
public:
	
	C_XB2Application();
	virtual ~C_XB2Application();

	bool Create();

	void Run();

protected:

	void Update();

private:

	bool m_Quit;
};

}; // namespace Samples

void LinkToDebugView(const char* name);

#endif // SAMPLES_XB2APPLICATION_H

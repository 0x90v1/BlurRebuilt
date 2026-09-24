#ifndef __XFARM_H__
#define __XFARM_H__

class XFarmApp : public Tools::C_ToolsApplication
{
public:
	XFarmApp();

	int Main(int argc, char **argv, Progress::I_ProgressBar *pProgress);
};

#endif

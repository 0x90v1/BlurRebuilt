#include "stdafx.h"

#include "farm/resource.h"

#include "farm/Farm.h"
#include "farm_library/Farmer.h"
#include "FarmUserInterface.h"

#include <CommCtrl.h>

void ExecuteUi(bool IsManager, U16 broadcastPort, U16 networkPort)
{
	ost::IPV4Host ServerAddress;

	if (!FindServer(broadcastPort, networkPort, &ServerAddress))
	{
		FarmPrint("No server found.\r\n");
		return;
	}

	FarmPrint("Found server at %s.\r\n", ServerAddress.getHostname());

	HACCEL Accel(LoadAccelerators(GetModuleHandle(0), MAKEINTRESOURCE(IDR_ACCEL)));

	FreeConsole();

	INITCOMMONCONTROLSEX Ccex;

	ZeroMemory(&Ccex, sizeof(Ccex));

	Ccex.dwSize	= sizeof(Ccex);
	Ccex.dwICC	= ICC_LISTVIEW_CLASSES;

	InitCommonControlsEx(&Ccex);

	FarmWindow Wnd(IsManager, broadcastPort, networkPort);

	Wnd.Create();

	ShowWindow(Wnd, SW_SHOW);

	for(MSG Msg; GetMessage(&Msg, 0, 0, 0); )
	{
		if (TranslateAccelerator(Wnd, Accel, &Msg) == 0)
			TranslateMessage(&Msg);

		DispatchMessage(&Msg);
	}
}

/*
*/
FarmUserInterface::FarmUserInterface(bool isManager, U16 broadcastPort, U16 networkPort) : m_isManager(isManager), Farmer(broadcastPort, networkPort)
{
}

FarmUserInterface::~FarmUserInterface()
{
}

bool FarmUserInterface::Start(void)
{
	//ExecuteServer(m_broadcastPort, m_networkPort);
	ExecuteUi(m_isManager, m_broadcastPort, m_networkPort);

	return true;
}



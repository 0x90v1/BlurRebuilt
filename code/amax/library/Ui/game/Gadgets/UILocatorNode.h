
#ifndef UI_LOCATORNODE_H
#define UI_LOCATORNODE_H

#ifdef _MSC_VER
#pragma once
#endif

#include "ui\UIGameDefines.h"
#include "GameLogic/ActorStudioActors/UiLocatorActor.h"

namespace UI
{

class UILocatorNode : public C_GadgetBase
{
public:
	DECLARE_GADGET( E_UIGameButtonType_LocatorNode, "UILocatorNode" )

	void			Init();
	void			PostInit();
	void			FrameUpdate();
	
	void			ChangeLocation( const char* pLocationName );

	bool			MessageHandler(const Messaging::C_MessageBase *pMessage); // from C_MessageHandler

public:
	XtSz8						m_LocationName;

private:
	Actors::UiLocatorActor *	m_pActor;
};

}; // namespace UI

#endif // UI_LOCATORNODE_H

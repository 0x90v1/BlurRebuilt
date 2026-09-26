#include "stdafx.h"

#include <UI/UIHelpers.h>

#include "UICarNode.h"
#include "Game/GameActors/VehicleLogic/VehicleLogicActor.h"

// XT Definition
XT_TYPE( UI::UICarNode, "UICarNode" )
	XT_BASE( UI::C_GadgetBase )
XT_END_TYPE

namespace UI
{

void UICarNode::Init()
{
	m_sLocalPlayerIndex = -1;
	m_pLogicActor = NULL;

	AddSubscriber( E_UiMsg_FrameUpdate );
	AddSubscriber( E_UiGameMsg_SetLocalPlayerIndex );

	C_GadgetBase::Init();
}

bool UICarNode::MessageHandler( const Messaging::C_MessageBase* pMessage )
{
	switch( pMessage->Type() )
	{
	case E_UiGameMsg_SetLocalPlayerIndex:
		if ( m_sLocalPlayerIndex == -1 )
		{
			m_sLocalPlayerIndex = (S32)Messaging::MessageCast<C_UiMessageData>( pMessage )->m_Args[ 0 ];
			m_Panel = E_GameSpritePanel_VP0 + m_sLocalPlayerIndex;

			m_pLogicActor = GetVehicleLogicActor( m_sLocalPlayerIndex );
			if( !m_pLogicActor )
				return true;

			return true;
		}
	}

	return C_GadgetBase::MessageHandler( pMessage );
}

void UICarNode::FrameUpdate()
{
	if( m_pLogicActor )
	{
		Maths::HMat43 mat;
		CalcTransformHierarchy( mat );

		SetMatrix(mat * m_pLogicActor->GetWorldMatrix());
	}

	return C_GadgetBase::FrameUpdate();
}

}; // namespace UI
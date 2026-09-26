#include "stdafx.h"

#include <input\GameInput.h>

namespace Input
{

C_GameInputManager* g_pGameInputManager = NULL;

// C_GameInputButton

C_GameInputButton::C_GameInputButton() 
	: m_Value(0.0f)
	, m_DebouncePressedState(false)
	, m_DebounceReleasedState(false)
	, m_TimeHeld(0)
	, m_pButtonInput(NULL)
{
}

C_GameInputButton::~C_GameInputButton()
{
	if(m_pButtonInput && m_pButtonInput->DecRefCount())
		SAFE_DELETE_PTR(m_pButtonInput);
}

void C_GameInputButton::Update( F32 buttonPressedThreshold )
{
    const F32 prevValue = m_Value;
    if(m_pButtonInput)
    {
        m_Value = m_pButtonInput->GetValue();
    }
    else
    {
        m_Value = 0;
    }

	const bool pressed = m_Value > buttonPressedThreshold;
	const bool prevPressed = prevValue > buttonPressedThreshold;

	if(pressed)
	{
		m_TimeHeld++;
	}
	else
	{
		m_TimeHeld = 0;
	}

    if(pressed && !prevPressed)
    {
        m_DebouncePressedState = true;
    }
    else
    {
        m_DebouncePressedState = false;
    }

	if(prevPressed && !pressed)
	{
		m_DebounceReleasedState = true;
	}
	else
	{
		m_DebounceReleasedState = false;
	}
}

void C_GameInputButton::SetButtonInput( C_ButtonInput* pInput )
{
	// inc ref count on new input
	if(pInput)
	{
		pInput->IncRefCount();
	}

	// remove old input
	if(m_pButtonInput)
	{
		if(m_pButtonInput->DecRefCount())
		{
			SAFE_DELETE_PTR(m_pButtonInput);
		}
	}

	// assign new input
	m_pButtonInput = pInput;
}

C_GameInputAxis::C_GameInputAxis()
	: m_Value(0.0f)
	, m_pAxisInput(NULL)
{
}

C_GameInputAxis::~C_GameInputAxis()
{
	if(m_pAxisInput && m_pAxisInput->DecRefCount())
		SAFE_DELETE_PTR(m_pAxisInput);
}

void C_GameInputAxis::Update()
{
	//const F32 prevValue = m_Value;
    if(m_pAxisInput)
    {
        m_Value = m_pAxisInput->GetValue();
    }
	else
	{
		m_Value = 0.0f;
	}
}

void C_GameInputAxis::SetAxisInput( C_AxisInput* pInput )
{
	// inc ref count on new input
	if(pInput)
	{
		pInput->IncRefCount();
	}

	// remove old input
	if(m_pAxisInput)
	{
		if(m_pAxisInput->DecRefCount())
		{
			SAFE_DELETE_PTR(m_pAxisInput);
		}
	}

	// assign new input
	m_pAxisInput = pInput;
}

// C_GameInput

C_GameInput::C_GameInput()
	: m_ButtonPressedThreshold(0.1f)
	, m_UserIndex(-1)
{
}

C_GameInput::~C_GameInput()
{
}

C_GameInput* C_GameInput::GetInstance( const S32 userIndex )
{
	return C_GameInputManager::GetInstance()->GetGameInput(userIndex);
}

const S32 C_GameInput::GetUserIndex() const
{
	return m_UserIndex;
}

const S32 C_GameInput::GetButtonCount() const
{
	return m_GameButtonList.Size();
}

C_ButtonInput* C_GameInput::GetButtonInput( const U32 buttonId ) const
{
	return m_GameButtonList[buttonId].m_pButtonInput;
}

void C_GameInput::SetButtonInput( const U32 buttonId, C_ButtonInput* pInput )
{
	m_GameButtonList[buttonId].SetButtonInput(pInput);
}

const bool C_GameInput::GetButtonPressed( const U32 buttonId ) const
{
	return m_GameButtonList[buttonId].m_Value >= m_ButtonPressedThreshold; 
}

const F32 C_GameInput::GetButtonValue( const U32 buttonId ) const
{
	return m_GameButtonList[buttonId].m_Value;
}

const bool C_GameInput::GetButtonPressedDebounced( const U32 buttonId ) const
{
	return m_GameButtonList[ buttonId ].m_DebouncePressedState;
}

const bool C_GameInput::GetButtonReleasedDebounced( const U32 buttonId ) const
{
	return m_GameButtonList[ buttonId ].m_DebounceReleasedState;
}

const U16 C_GameInput::GetButtonPressedTime( const U32 buttonId ) const
{
	return m_GameButtonList[ buttonId ].m_TimeHeld;
}

const bool C_GameInput::GetButtonAutoRepeat( const U32 buttonId, const U16 initialDelay, const U16 repeatTime ) const
{
	const U16 timeHeld = GetButtonPressedTime( buttonId );

	if(timeHeld == 1)
		return true;

	if(timeHeld < initialDelay)
		return false;

	if(repeatTime == 0)
		return true;

	if(((timeHeld - initialDelay) % repeatTime) == 0)
		return true;

	return false;
}

const S32 C_GameInput::GetAxisCount() const
{
	return m_GameAxisList.Size();
}

C_AxisInput* C_GameInput::GetAxisInput( const U32 axisId ) const
{
	return m_GameAxisList[axisId].m_pAxisInput;
}

void C_GameInput::SetAxisInput( const U32 axisId, C_AxisInput* pInput)
{
	m_GameAxisList[axisId].SetAxisInput( pInput );
}

const F32 C_GameInput::GetAxisValue( const U32 axisId ) const
{
	return (m_GameAxisList[axisId].m_Value + 1.0f) * 0.5f;
}

const F32 C_GameInput::GetAxisSignedValue( const U32 axisId ) const
{
	return m_GameAxisList[axisId].m_Value;
}

void C_GameInput::SetButtonPressedThreshold( const F32 threshold )
{
	m_ButtonPressedThreshold = threshold; 
}

void C_GameInput::Update()
{
	const S32 buttonCount = m_GameButtonList.Size();
	for(S32 i = 0; i < buttonCount; i++)
		m_GameButtonList[ i ].Update( m_ButtonPressedThreshold );

	const S32 axisCount = m_GameAxisList.Size();
	for(S32 i = 0; i < axisCount; i++)
		m_GameAxisList[ i ].Update();
}

// C_GameInputManager

C_GameInputManager::C_GameInputManager( const S32 userCount, const S32 buttonCount, const S32 axisCount )
{
	assert(g_pGameInputManager == NULL);
	g_pGameInputManager = this;

	Resize( userCount, buttonCount, axisCount );
}

C_GameInputManager::~C_GameInputManager()
{
	assert(g_pGameInputManager == this);
	g_pGameInputManager = NULL;
}

C_GameInputManager* C_GameInputManager::GetInstance()
{
	return g_pGameInputManager;
}

void C_GameInputManager::Resize( const S32 userCount, const S32 buttonCount, const S32 axisCount )
{
	// clear all mappings
	for(S32 userNo=0; userNo<m_GameInputList.Size(); userNo++)
	{
		for(S32 i=0; i<GetGameInput(userNo)->GetButtonCount(); i++)
		{
			GetGameInput(userNo)->SetButtonInput(i, NULL);
		}

		for(S32 i=0; i<GetGameInput(userNo)->GetAxisCount(); i++)
		{
			GetGameInput(userNo)->SetAxisInput(i, NULL);
		}
	}

	// resize mapping list
	m_GameInputList.Resize(userCount);

	for(S32 userNo=0; userNo<userCount; userNo++)
	{
		m_GameInputList[userNo].m_UserIndex = userNo;
		m_GameInputList[userNo].m_GameButtonList.Resize(buttonCount);
		m_GameInputList[userNo].m_GameAxisList.Resize(axisCount);
	}
}

const S32 C_GameInputManager::GetUserIndexCount()
{
	return m_GameInputList.Size();
}

C_GameInput* C_GameInputManager::GetGameInput( const S32 userIndex )
{
	return &m_GameInputList[userIndex];
}

void C_GameInputManager::Update()
{
	const S32 userCount = m_GameInputList.Size();
	for(S32 userNo=0; userNo<userCount; userNo++)
	{
		GetGameInput(userNo)->Update();
	}
}

// Helper functions

C_GameInput* GetGameInput( const S32 userIndex )
{
	return C_GameInputManager::GetInstance()->GetGameInput( userIndex );
}

C_GameInputManager* GetGameInputManager()
{
	return C_GameInputManager::GetInstance();
}

} // namespace Input

#include "stdafx.h"

#include <input\Device.h>

namespace Input
{

C_Device::C_Device(	C_DeviceManager* pDeviceManager, 
							const S32 userIndex, 
							const S32 buttonCount, 
							const S32 axisCount, 
							const S32 rumbleMotorCount )
:	m_pDeviceManager(pDeviceManager),
	m_UserIndex(userIndex),
	m_Connected(false),
	m_Enabled(true),
	m_RumbleEnabled(false)
{
	m_ButtonValues.Resize(buttonCount);
	m_AxisValues.Resize(axisCount);

	m_RumbleMotorSpeeds.Resize(rumbleMotorCount);

	ResetValues();
	ResetRumbleMotorSpeeds();
}

C_Device::~C_Device()
{
	m_pDeviceManager->RemoveDevice(this);
}

const S32 C_Device::GetUserIndex() const
{
	return m_UserIndex;
}

const bool C_Device::IsConnected() const
{
	return m_Connected;
}

const S32 C_Device::GetButtonCount() const
{
	return m_ButtonValues.Size();
}

const F32 C_Device::GetButtonValue( const S32 index ) const
{
	return m_ButtonValues[index];
}

const S32 C_Device::GetAxisCount() const
{
	return m_AxisValues.Size();
}

const F32 C_Device::GetAxisSignedValue( const S32 index ) const
{
	return m_AxisValues[index];
}

void C_Device::Enable()
{
	m_Enabled = false;
}

void C_Device::Disable()
{
	m_Enabled = true;
}

const bool C_Device::IsEnabled() const
{
	return m_Enabled;
}

void C_Device::ResetValues()
{
	const S32 buttonCount = m_ButtonValues.Size();
	for(S32 i=0; i<buttonCount; i++)
	{
		m_ButtonValues[i] = 0;
	}

	const S32 axisCount = m_AxisValues.Size();
	for(S32 i=0; i<axisCount; i++)
	{
		m_AxisValues[i] = 0;
	}
}

void C_Device::ResetRumbleMotorSpeeds()
{
	const S32 rumbleMotorCount = m_RumbleMotorSpeeds.Size();
	for(S32 i=0; i<rumbleMotorCount; i++)
	{
		m_RumbleMotorSpeeds[i] = 0;
	}
}

const U32 C_Device::GetRumbleMotorCount() const
{
	return m_RumbleMotorSpeeds.Size();
}

void C_Device::EnableRumble()
{
	m_RumbleEnabled = true;
}

void C_Device::DisableRumble()
{
	m_RumbleEnabled = false;
}

const bool C_Device::IsRumbleEnabled() const
{
	return m_RumbleEnabled;
}

void C_Device::SetRumbleMotorSpeed(const S32 motorIndex, const F32 motorSpeed)
{
	m_RumbleMotorSpeeds[motorIndex] = motorSpeed;
}

} // namespace Input

#ifndef GAME_GAME_CONTROLS_H
#define GAME_GAME_CONTROLS_H

#ifdef _MSC_VER
#pragma once
#endif

#include <Input/Input.h>
#include <Input/GameInput.h>
#include <framework/application/UserControls.h>

namespace Game
{

//! E_Button

enum E_Button
{
	E_Button_Throttle = Framework::E_Button_Count,	
	E_Button_Brake,
	E_Button_Handbrake,
	E_Button_GearUp,
	E_Button_GearDown,
	E_Button_ChangeView,
	E_Button_ResetVehicle,

	E_Button_UsePickup,
	E_Button_DiscardPickup,
	E_Button_SwitchPickup,
	E_Button_SwitchPickupLeft,
	E_Button_SwitchPickupAlt,
	E_Button_UseShield,
	E_Button_UseSecondTierPickup,

	E_Button_Auto_Shuffle_Perks,
	E_Button_Convert_To_Health,
	E_Button_Clone_Selected_Perk,
	E_Button_Fullhouse,

	E_Button_UsePickupAlt,
	E_Button_PhotoRoll,
	E_Button_PhotoZoom,
	E_Button_PhotoRollReset,
	E_Button_PhotoZoomReset,

	E_Button_FireSlot1,
	E_Button_FireSlot2,
	E_Button_FireSlot3,

	E_Button_Count,
};

enum E_UiButton
{
	E_UiButton_Pad_Up = E_Button_Count,
	E_UiButton_Pad_Down,
	E_UiButton_Pad_Left,
	E_UiButton_Pad_Right,
	E_UiButton_Confirm,
	E_UiButton_Cancel,
	E_UiButton_Option1,
	E_UiButton_Option2,
	E_UiButton_LeftTrigger,
	E_UiButton_RightTrigger,
	E_UiButton_LeftShoulder,
	E_UiButton_RightShoulder,
	E_UiButton_LeftThumb,
	E_UiButton_RightThumb,
	E_UiButton_Start,
	E_UiButton_Back,

	E_UiButton_PadOrAxis_Up,  
	E_UiButton_PadOrAxis_Down,	
	E_UiButton_PadOrAxis_Left,
	E_UiButton_PadOrAxis_Right,

	E_UiButton_Count,
};

enum E_Axis
{
	E_Axis_Steering = Framework::E_Axis_Count,	
	E_Axis_Pitch,	
	E_Axis_PhotoMoveUp,
	E_Axis_DigitalSteering,

	E_Axis_Count,
};

enum E_UiAxis
{
	E_UiAxis_LeftStick_UD = E_Axis_Count,
	E_UiAxis_LeftStick_LR,

	E_UiAxis_RightStick_UD,
	E_UiAxis_RightStick_LR,

	E_UiAxis_Count
};	

void SetupAmaxUserControls( Input::C_GameInput* pGameInput, Input::C_Device* pKeyboard, Input::C_Device* pMouse, Input::C_Device* pPad );

} // namespace Game

#endif // GAME_GAME_CONTROLS_H

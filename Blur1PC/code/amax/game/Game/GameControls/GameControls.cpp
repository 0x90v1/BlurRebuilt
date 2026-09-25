#include "stdafx.h"

#include <Input/Device.h>
#include <Input/DeviceManager.h>
#include <Input/GameInputMapping.h>

#if defined(PLATFORM_PC) || defined(PLATFORM_XENON)
#include <Input/Pad360.h>
#endif

#if defined(PLATFORM_PS3)
#include <Input/PadPS3.h>
#endif

#if defined(PLATFORM_PC)
#include <Input/KeyboardPC.h>
#include <Input/Mouse.h>
#endif

#include <Game/GameControls/GameControls.h>


namespace Game
{
using namespace Input;

#if defined ( PLATFORM_PS3 )

struct C_PadOrAxisSetup
{
      E_UiButton					m_PadOrAxis;
      C_PadPS3::E_Button     m_Pad;
      C_PadPS3::E_Axis       m_Axis;
      bool                          m_InvertAxis;
};

static const C_PadOrAxisSetup kPadOrAxisSetup [] = 
{
      {     E_UiButton_PadOrAxis_Up,      C_PadPS3::E_Button_DPadUp,         C_PadPS3::E_Axis_LeftStick_Y,      false,    },
      {     E_UiButton_PadOrAxis_Down,    C_PadPS3::E_Button_DPadDown,       C_PadPS3::E_Axis_LeftStick_Y,      true,		},
      {     E_UiButton_PadOrAxis_Left,    C_PadPS3::E_Button_DPadLeft,       C_PadPS3::E_Axis_LeftStick_X,      true,		},
      {     E_UiButton_PadOrAxis_Right,   C_PadPS3::E_Button_DPadRight,      C_PadPS3::E_Axis_LeftStick_X,      false,    },
};

#elif defined(PLATFORM_PC) || defined(PLATFORM_XENON)

struct C_PadOrAxisSetup
{
      E_UiButton					m_PadOrAxis;
      C_Pad360::E_Button     m_Pad;
      C_Pad360::E_Axis       m_Axis;
#if defined (PLATFORM_PC)
	  C_Keyboard::E_Button	m_Key;
#endif
      bool                          m_InvertAxis;
};


#if defined (PLATFORM_XENON)

static const C_PadOrAxisSetup kPadOrAxisSetup [] = 
{
      {     E_UiButton_PadOrAxis_Up,      C_Pad360::E_Button_DPadUp,         C_Pad360::E_Axis_LeftStick_Y,  	  false,    },
      {     E_UiButton_PadOrAxis_Down,    C_Pad360::E_Button_DPadDown,       C_Pad360::E_Axis_LeftStick_Y,      true,		},
      {     E_UiButton_PadOrAxis_Left,    C_Pad360::E_Button_DPadLeft,       C_Pad360::E_Axis_LeftStick_X,      true,		},
      {     E_UiButton_PadOrAxis_Right,   C_Pad360::E_Button_DPadRight,      C_Pad360::E_Axis_LeftStick_X,      false,    },
};

#elif defined(PLATFORM_PC)

static const C_PadOrAxisSetup kPadOrAxisSetup [] = 
{
      {     E_UiButton_PadOrAxis_Up,      C_Pad360::E_Button_DPadUp,     C_Pad360::E_Axis_LeftStick_Y,  C_Keyboard::E_Button_Up,		false,  },
      {     E_UiButton_PadOrAxis_Down,    C_Pad360::E_Button_DPadDown,   C_Pad360::E_Axis_LeftStick_Y,  C_Keyboard::E_Button_Down,		true,	},
      {     E_UiButton_PadOrAxis_Left,    C_Pad360::E_Button_DPadLeft,   C_Pad360::E_Axis_LeftStick_X,  C_Keyboard::E_Button_Left,		true,	},
      {     E_UiButton_PadOrAxis_Right,   C_Pad360::E_Button_DPadRight,  C_Pad360::E_Axis_LeftStick_X,  C_Keyboard::E_Button_Right,	false,  },
};

#endif

#endif




void SetupAmaxUserControls( C_GameInput* pGameInput, C_Device* pKeyboard, C_Device* pMouse, C_Device* pPad )
{
	assert(pGameInput);

	#if defined(PLATFORM_PS3)

	pGameInput->SetButtonInput(E_Button_Throttle,		new C_ButtonInputFromDevice(pPad, 	C_PadPS3::E_Button_R2));
	pGameInput->SetButtonInput(E_Button_Brake,			new C_ButtonInputFromDevice(pPad, 	C_PadPS3::E_Button_L2));
	pGameInput->SetButtonInput(E_Button_Handbrake,		new C_ButtonInputFromDevice(pPad, 	C_PadPS3::E_Button_Circle));
	//CA - Amax doesn't currently require gear up/down buttons
	//pGameInput->SetButtonInput(E_Button_GearUp,		new C_ButtonInputFromDevice(pPad, 	C_PadPS3::E_Button_Circle));
	//pGameInput->SetButtonInput(E_Button_GearDown,		new C_ButtonInputFromDevice(pPad, 	C_PadPS3::E_Button_Cross));
	pGameInput->SetButtonInput(E_Button_ChangeView,		new C_ButtonInputFromDevice(pPad, 	C_PadPS3::E_Button_R1));
	pGameInput->SetButtonInput(E_Button_ResetVehicle,	new C_ButtonInputFromDevice(pPad,    C_PadPS3::E_Button_R1));
	pGameInput->SetButtonInput(E_Button_UsePickup,		new C_ButtonInputFromDevice(pPad, 	C_PadPS3::E_Button_Cross));
	pGameInput->SetButtonInput(E_Button_DiscardPickup,	new C_ButtonInputFromDevice(pPad, 	C_PadPS3::E_Button_Triangle));
	pGameInput->SetButtonInput(E_Button_SwitchPickup,	new C_ButtonInputFromDevice(pPad, 	C_PadPS3::E_Button_L1));
	pGameInput->SetButtonInput(E_Button_SwitchPickupAlt, new C_ButtonInputFromDevice(pPad, 	C_PadPS3::E_Button_Square));
	pGameInput->SetButtonInput(E_Button_UseShield,		new C_ButtonInputFromDevice(pPad, 	C_PadPS3::E_Button_Square));
	
	pGameInput->SetButtonInput(E_Button_UseSecondTierPickup,	new C_ButtonInputFromDevice(pPad, 	C_PadPS3::E_Button_Triangle));
	pGameInput->SetButtonInput(E_Button_Auto_Shuffle_Perks,		new C_ButtonInputFromDevice(pPad,    C_PadPS3::E_Button_DPadUp));
	pGameInput->SetButtonInput(E_Button_Convert_To_Health,		new C_ButtonInputFromDevice(pPad,    C_PadPS3::E_Button_DPadLeft));
	pGameInput->SetButtonInput(E_Button_Clone_Selected_Perk,	new C_ButtonInputFromDevice(pPad,    C_PadPS3::E_Button_DPadRight));
	pGameInput->SetButtonInput(E_Button_Fullhouse,				new C_ButtonInputFromDevice(pPad,    C_PadPS3::E_Button_DPadDown));

	//pGameInput->SetButtonInput(E_Button_UsePickupAlt,	new C_ButtonInputFromDevice(pPad,    C_PadPS3::E_Button_R3));

	pGameInput->SetButtonInput(E_Button_PhotoRoll,		new C_ButtonInputFromDevice(pPad,    C_PadPS3::E_Button_L2));
	pGameInput->SetButtonInput(E_Button_PhotoZoom,		new C_ButtonInputFromDevice(pPad,    C_PadPS3::E_Button_R2));
	pGameInput->SetButtonInput(E_Button_PhotoRollReset,	new C_ButtonInputFromDevice(pPad,    C_PadPS3::E_Button_L3));
	pGameInput->SetButtonInput(E_Button_PhotoZoomReset,	new C_ButtonInputFromDevice(pPad,    C_PadPS3::E_Button_R3));

	C_ButtonInput* flyUp   = new C_ButtonInputFromDevice(pPad, C_PadPS3::E_Button_DPadUp);
	C_ButtonInput* flyDown = new C_ButtonInputFromDevice(pPad, C_PadPS3::E_Button_DPadDown);
	pGameInput->SetAxisInput(E_Axis_PhotoMoveUp, new C_AxisInputFromButtons(flyDown, flyUp));

	pGameInput->SetAxisInput(E_Axis_Steering,			new C_AxisInputFromDevice(pPad, 		C_PadPS3::E_Axis_LeftStick_X));
	pGameInput->SetAxisInput(E_Axis_Pitch,				new C_AxisInputFromDevice(pPad, 		C_PadPS3::E_Axis_LeftStick_Y));

	C_ButtonInput * pSteerLeftButton =									new C_ButtonInputFromDevice(pPad, 		C_PadPS3::E_Button_DPadLeft);
	C_ButtonInput * pSteerRightButton =									new C_ButtonInputFromDevice(pPad, 		C_PadPS3::E_Button_DPadRight);
	pGameInput->SetAxisInput(E_Axis_DigitalSteering,					new C_AxisInputFromDigitalButtons(pSteerLeftButton,	pSteerRightButton));

	// Ui buttons
	pGameInput->SetButtonInput(E_UiButton_Pad_Up,		new C_ButtonInputFromDevice(pPad, 	C_PadPS3::E_Button_DPadUp));
	pGameInput->SetButtonInput(E_UiButton_Pad_Down,		new C_ButtonInputFromDevice(pPad, 	C_PadPS3::E_Button_DPadDown));
	pGameInput->SetButtonInput(E_UiButton_Pad_Left,		new C_ButtonInputFromDevice(pPad, 	C_PadPS3::E_Button_DPadLeft));
	pGameInput->SetButtonInput(E_UiButton_Pad_Right,	new C_ButtonInputFromDevice(pPad, 	C_PadPS3::E_Button_DPadRight));
	pGameInput->SetButtonInput(E_UiButton_Confirm,		new C_ButtonInputFromDevice(pPad, 	C_PadPS3::E_Button_Cross));
	pGameInput->SetButtonInput(E_UiButton_Cancel,		new C_ButtonInputFromDevice(pPad, 	C_PadPS3::E_Button_Circle));
	pGameInput->SetButtonInput(E_UiButton_Option1,		new C_ButtonInputFromDevice(pPad, 	C_PadPS3::E_Button_Square));
	pGameInput->SetButtonInput(E_UiButton_Option2,		new C_ButtonInputFromDevice(pPad, 	C_PadPS3::E_Button_Triangle));
	pGameInput->SetButtonInput(E_UiButton_LeftTrigger,	new C_ButtonInputFromDevice(pPad, 	C_PadPS3::E_Button_L2));
	pGameInput->SetButtonInput(E_UiButton_RightTrigger,	new C_ButtonInputFromDevice(pPad, 	C_PadPS3::E_Button_R2));
	pGameInput->SetButtonInput(E_UiButton_LeftShoulder,	new C_ButtonInputFromDevice(pPad, 	C_PadPS3::E_Button_L1));
	pGameInput->SetButtonInput(E_UiButton_RightShoulder,new C_ButtonInputFromDevice(pPad, 	C_PadPS3::E_Button_R1));
	pGameInput->SetButtonInput(E_UiButton_LeftThumb,	new C_ButtonInputFromDevice(pPad, 	C_PadPS3::E_Button_L3));
	pGameInput->SetButtonInput(E_UiButton_RightThumb,	new C_ButtonInputFromDevice(pPad, 	C_PadPS3::E_Button_R3));
	pGameInput->SetButtonInput(E_UiButton_Start,		new C_ButtonInputFromDevice(pPad, 	C_PadPS3::E_Button_Start));
	pGameInput->SetButtonInput(E_UiButton_Back,			new C_ButtonInputFromDevice(pPad, 	C_PadPS3::E_Button_Select));
	// Ui Axis
	pGameInput->SetAxisInput(E_UiAxis_LeftStick_UD,		new C_AxisInputFromDevice(pPad, 		C_PadPS3::E_Axis_LeftStick_Y));
	pGameInput->SetAxisInput(E_UiAxis_LeftStick_LR,		new C_AxisInputFromDevice(pPad, 		C_PadPS3::E_Axis_LeftStick_X));
	pGameInput->SetAxisInput(E_UiAxis_RightStick_UD,	new C_AxisInputFromDevice(pPad, 		C_PadPS3::E_Axis_RightStick_Y));
	pGameInput->SetAxisInput(E_UiAxis_RightStick_LR,	new C_AxisInputFromDevice(pPad, 		C_PadPS3::E_Axis_RightStick_X));

	#elif defined(PLATFORM_XENON)

	// Game buttons
	pGameInput->SetButtonInput(E_Button_Throttle,						new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_RightTrigger));
	pGameInput->SetButtonInput(E_Button_Brake,							new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_LeftTrigger));
	pGameInput->SetButtonInput(E_Button_Handbrake,						new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_B));
	
	//CA - Amax doesn't currently require gear up/down buttons
	//pGameInput->SetButtonInput(E_Button_GearUp,						new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_B));
	//pGameInput->SetButtonInput(E_Button_GearDown,						new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_X));

	pGameInput->SetButtonInput(E_Button_ChangeView,						new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_RightShoulder));
	pGameInput->SetButtonInput(E_Button_UsePickup,						new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_A));
	pGameInput->SetButtonInput(E_Button_DiscardPickup,					new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_Y));
	pGameInput->SetButtonInput(E_Button_SwitchPickupAlt,				new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_LeftShoulder));
	pGameInput->SetButtonInput(E_Button_SwitchPickup,					new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_X));
	pGameInput->SetButtonInput(E_Button_UseShield,						new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_X));
	pGameInput->SetButtonInput(E_Button_ResetVehicle,					new C_ButtonInputFromDevice(pPad,    C_Pad360::E_Button_RightShoulder));
	pGameInput->SetButtonInput(E_Button_UseSecondTierPickup,			new C_ButtonInputFromDevice(pPad,    C_Pad360::E_Button_Y));
	pGameInput->SetButtonInput(E_Button_Auto_Shuffle_Perks,				new C_ButtonInputFromDevice(pPad,    C_Pad360::E_Button_DPadUp));
	pGameInput->SetButtonInput(E_Button_Convert_To_Health,				new C_ButtonInputFromDevice(pPad,    C_Pad360::E_Button_DPadLeft));
	pGameInput->SetButtonInput(E_Button_Clone_Selected_Perk,			new C_ButtonInputFromDevice(pPad,    C_Pad360::E_Button_DPadRight));
	pGameInput->SetButtonInput(E_Button_Fullhouse,						new C_ButtonInputFromDevice(pPad,    C_Pad360::E_Button_DPadDown));

	//pGameInput->SetButtonInput(E_Button_UsePickupAlt,					new C_ButtonInputFromDevice(pPad,    C_Pad360::E_Button_RightThumb));

	pGameInput->SetButtonInput(E_Button_PhotoRoll,						new C_ButtonInputFromDevice(pPad,    C_Pad360::E_Button_LeftTrigger));
	pGameInput->SetButtonInput(E_Button_PhotoZoom,						new C_ButtonInputFromDevice(pPad,    C_Pad360::E_Button_RightTrigger));
	pGameInput->SetButtonInput(E_Button_PhotoRollReset,					new C_ButtonInputFromDevice(pPad,    C_Pad360::E_Button_LeftThumb));
	pGameInput->SetButtonInput(E_Button_PhotoZoomReset,					new C_ButtonInputFromDevice(pPad,    C_Pad360::E_Button_RightThumb));

	C_ButtonInput* flyUp   = new C_ButtonInputFromDevice(pPad, C_Pad360::E_Button_DPadUp);
	C_ButtonInput* flyDown = new C_ButtonInputFromDevice(pPad, C_Pad360::E_Button_DPadDown);
	pGameInput->SetAxisInput(E_Axis_PhotoMoveUp, new C_AxisInputFromButtons(flyDown, flyUp));

	C_ButtonInput * pSteerLeftButton =									new C_ButtonInputFromDevice(pPad, 		C_Pad360::E_Button_DPadLeft);
	C_ButtonInput * pSteerRightButton =									new C_ButtonInputFromDevice(pPad, 		C_Pad360::E_Button_DPadRight);
	pGameInput->SetAxisInput(E_Axis_DigitalSteering,					new C_AxisInputFromButtons(pSteerLeftButton,	pSteerRightButton));

	// Game axis
	pGameInput->SetAxisInput(E_Axis_Steering,							new C_AxisInputFromDevice(pPad, 	C_Pad360::E_Axis_LeftStick_X));
	pGameInput->SetAxisInput(E_Axis_Pitch,								new C_AxisInputFromDevice(pPad, 	C_Pad360::E_Axis_LeftStick_Y));

	// Ui buttons
	pGameInput->SetButtonInput(E_UiButton_Pad_Up,						new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_DPadUp));
	pGameInput->SetButtonInput(E_UiButton_Pad_Down,						new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_DPadDown));
	pGameInput->SetButtonInput(E_UiButton_Pad_Left,						new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_DPadLeft));
	pGameInput->SetButtonInput(E_UiButton_Pad_Right,					new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_DPadRight));
	pGameInput->SetButtonInput(E_UiButton_Confirm,						new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_A));
	pGameInput->SetButtonInput(E_UiButton_Cancel,						new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_B));
	pGameInput->SetButtonInput(E_UiButton_Option1,						new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_X));
	pGameInput->SetButtonInput(E_UiButton_Option2,						new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_Y));
	pGameInput->SetButtonInput(E_UiButton_LeftTrigger,					new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_LeftTrigger));
	pGameInput->SetButtonInput(E_UiButton_RightTrigger,					new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_RightTrigger));
	pGameInput->SetButtonInput(E_UiButton_LeftShoulder,					new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_LeftShoulder));
	pGameInput->SetButtonInput(E_UiButton_RightShoulder,				new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_RightShoulder));
	pGameInput->SetButtonInput(E_UiButton_LeftThumb,					new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_LeftThumb));
	pGameInput->SetButtonInput(E_UiButton_RightThumb,					new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_RightThumb));
	pGameInput->SetButtonInput(E_UiButton_Start,						new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_Start));
	pGameInput->SetButtonInput(E_UiButton_Back,							new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_Back));
	// Ui Axis
	pGameInput->SetAxisInput(E_UiAxis_LeftStick_UD,						new C_AxisInputFromDevice(pPad, 		C_Pad360::E_Axis_LeftStick_Y));
	pGameInput->SetAxisInput(E_UiAxis_LeftStick_LR,						new C_AxisInputFromDevice(pPad, 		C_Pad360::E_Axis_LeftStick_X));
	pGameInput->SetAxisInput(E_UiAxis_RightStick_UD,					new C_AxisInputFromDevice(pPad, 		C_Pad360::E_Axis_RightStick_Y));
	pGameInput->SetAxisInput(E_UiAxis_RightStick_LR,					new C_AxisInputFromDevice(pPad, 		C_Pad360::E_Axis_RightStick_X));
#elif defined(PLATFORM_PC)

	//////////////////////////////////////////////
	// Setup keyboard as main controller

	// Game buttons
	pGameInput->SetButtonInput(E_Button_Throttle,						new C_ButtonInputFromDevice(pKeyboard,		C_Keyboard::E_Button_Up));
	pGameInput->SetButtonInput(E_Button_Brake,							new C_ButtonInputFromDevice(pKeyboard,		C_Keyboard::E_Button_Down));
	pGameInput->SetButtonInput(E_Button_Handbrake,						new C_ButtonInputFromDevice(pKeyboard,		C_Keyboard::E_Button_B));
	pGameInput->SetButtonInput(E_Button_ChangeView,						new C_ButtonInputFromDevice(pKeyboard,		C_Keyboard::E_Button_Next));
	pGameInput->SetButtonInput(E_Button_UsePickup,						new C_ButtonInputFromDevice(pKeyboard,		C_Keyboard::E_Button_V));
	pGameInput->SetButtonInput(E_Button_DiscardPickup,					new C_ButtonInputFromDevice(pKeyboard,		C_Keyboard::E_Button_C));
	pGameInput->SetButtonInput(E_Button_SwitchPickupAlt,				new C_ButtonInputFromDevice(pKeyboard,		C_Keyboard::E_Button_Delete));
	pGameInput->SetButtonInput(E_Button_SwitchPickup,					new C_ButtonInputFromDevice(pKeyboard,		C_Keyboard::E_Button_C));
	pGameInput->SetButtonInput(E_Button_UseShield,						new C_ButtonInputFromDevice(pKeyboard,		C_Keyboard::E_Button_X));
	pGameInput->SetButtonInput(E_Button_ResetVehicle,					new C_ButtonInputFromDevice(pKeyboard,		C_Keyboard::E_Button_Next));
	pGameInput->SetButtonInput(E_Button_UseSecondTierPickup,			new C_ButtonInputFromDevice(pKeyboard,		C_Keyboard::E_Button_Z));
	pGameInput->SetButtonInput(E_Button_Auto_Shuffle_Perks,				new C_ButtonInputFromDevice(pKeyboard,		C_Keyboard::E_Button_T));
	pGameInput->SetButtonInput(E_Button_Convert_To_Health,				new C_ButtonInputFromDevice(pKeyboard,		C_Keyboard::E_Button_F));
	pGameInput->SetButtonInput(E_Button_Clone_Selected_Perk,			new C_ButtonInputFromDevice(pKeyboard,		C_Keyboard::E_Button_G));
	pGameInput->SetButtonInput(E_Button_Fullhouse,						new C_ButtonInputFromDevice(pKeyboard,		C_Keyboard::E_Button_H));

	pGameInput->SetButtonInput(E_Button_PhotoRoll,						new C_ButtonInputFromDevice(pKeyboard,		C_Keyboard::E_Button_Up));
	pGameInput->SetButtonInput(E_Button_PhotoZoom,						new C_ButtonInputFromDevice(pKeyboard,		C_Keyboard::E_Button_Down));

	C_ButtonInput * pSteerLeftButton =							new C_ButtonInputFromDevice(pKeyboard, 		C_Keyboard::E_Button_Left);
	C_ButtonInput * pSteerRightButton =							new C_ButtonInputFromDevice(pKeyboard, 		C_Keyboard::E_Button_Right);
	C_ButtonInput * pPitchUpButton =								new C_ButtonInputFromDevice(pKeyboard, 		C_Keyboard::E_Button_Comma);
	C_ButtonInput * pPitchDownButton =							new C_ButtonInputFromDevice(pKeyboard, 		C_Keyboard::E_Button_Period);

	pGameInput->SetAxisInput(E_Axis_Steering,							new C_AxisInputFromButtons(pSteerLeftButton,	pSteerRightButton));
	pGameInput->SetAxisInput(E_Axis_Pitch,								new C_AxisInputFromButtons(pPitchUpButton,	pPitchDownButton));

	/////////////////////////////////////////////
	// Ui Buttons
	// [Blur2PC] Keyboard menu bindings as in retail Blur PC (Blur.exe 1.2, SetupAmaxUserControls at
	// 0x00415a30, single-keyboard layout: table 0x00e653b8 rows {button, key, split-screen key, pad},
	// fixed axes after it). Retail binds no keys to E_UiButton_Pad_*; this snapshot had arrows there,
	// V/B for Confirm/Cancel, Enter for Start and Escape for Back. Details: docs/analysis.md.
	pGameInput->SetButtonInput(E_UiButton_Confirm,						new C_ButtonInputFromDevice(pKeyboard, 	C_Keyboard::E_Button_Return));
	Framework::AddButtonInput(pGameInput, E_UiButton_Confirm,			new C_ButtonInputFromDevice(pKeyboard, 	C_Keyboard::E_Button_Space));
	pGameInput->SetButtonInput(E_UiButton_Cancel,						new C_ButtonInputFromDevice(pKeyboard, 	C_Keyboard::E_Button_Escape));
	pGameInput->SetButtonInput(E_UiButton_Option1,						new C_ButtonInputFromDevice(pKeyboard, 	C_Keyboard::E_Button_RShift));
	pGameInput->SetButtonInput(E_UiButton_Option2,						new C_ButtonInputFromDevice(pKeyboard, 	C_Keyboard::E_Button_RControl));
	pGameInput->SetButtonInput(E_UiButton_LeftTrigger,					new C_ButtonInputFromDevice(pKeyboard, 	C_Keyboard::E_Button_Prior));
	pGameInput->SetButtonInput(E_UiButton_RightTrigger,					new C_ButtonInputFromDevice(pKeyboard, 	C_Keyboard::E_Button_Next));
	pGameInput->SetButtonInput(E_UiButton_LeftShoulder,					new C_ButtonInputFromDevice(pKeyboard, 	C_Keyboard::E_Button_Insert));
	pGameInput->SetButtonInput(E_UiButton_RightShoulder,				new C_ButtonInputFromDevice(pKeyboard, 	C_Keyboard::E_Button_Delete));
	pGameInput->SetButtonInput(E_UiButton_RightThumb,					new C_ButtonInputFromDevice(pKeyboard, 	C_Keyboard::E_Button_Delete));
	pGameInput->SetButtonInput(E_UiButton_Start,						new C_ButtonInputFromDevice(pKeyboard, 	C_Keyboard::E_Button_F1));
	Framework::AddButtonInput(pGameInput, E_UiButton_Start,				new C_ButtonInputFromDevice(pKeyboard, 	C_Keyboard::E_Button_Escape));
	pGameInput->SetButtonInput(E_UiButton_Back,							new C_ButtonInputFromDevice(pKeyboard, 	C_Keyboard::E_Button_F2));

	/////////////////////////////////////////////
	// Ui Axis
	// [Blur2PC] retail: left stick A/D and S/W, right stick arrows (argument order negative, positive,
	// as retail's steering axis SteerLeft, SteerRight); this snapshot had W/S and the numeric keypad
	C_ButtonInput* pUILeftStickUpButton =						new C_ButtonInputFromDevice(pKeyboard, 	C_Keyboard::E_Button_W);
	C_ButtonInput* pUILeftStickDownButton =						new C_ButtonInputFromDevice(pKeyboard, 	C_Keyboard::E_Button_S);
	C_ButtonInput* pUILeftStickLeftButton =						new C_ButtonInputFromDevice(pKeyboard, 	C_Keyboard::E_Button_A);
	C_ButtonInput* pUILeftStickRightButton =						new C_ButtonInputFromDevice(pKeyboard, 	C_Keyboard::E_Button_D);
	C_ButtonInput* pUIRightStickUpButton =						new C_ButtonInputFromDevice(pKeyboard, 	C_Keyboard::E_Button_Up);
	C_ButtonInput* pUIRightStickDownButton =						new C_ButtonInputFromDevice(pKeyboard, 	C_Keyboard::E_Button_Down);
	C_ButtonInput* pUIRightStickLeftButton =						new C_ButtonInputFromDevice(pKeyboard, 	C_Keyboard::E_Button_Left);
	C_ButtonInput* pUIRightStickRightButton =					new C_ButtonInputFromDevice(pKeyboard, 	C_Keyboard::E_Button_Right);

	pGameInput->SetAxisInput(E_UiAxis_LeftStick_UD,						new C_AxisInputFromButtons(pUILeftStickDownButton,	pUILeftStickUpButton));
	pGameInput->SetAxisInput(E_UiAxis_LeftStick_LR,						new C_AxisInputFromButtons(pUILeftStickLeftButton,	pUILeftStickRightButton));
	pGameInput->SetAxisInput(E_UiAxis_RightStick_UD,					new C_AxisInputFromButtons(pUIRightStickDownButton,	pUIRightStickUpButton));
	pGameInput->SetAxisInput(E_UiAxis_RightStick_LR,					new C_AxisInputFromButtons(pUIRightStickLeftButton,	pUIRightStickRightButton));

	//////////////////////////////////////
	// Add XBox360 Contoller in

	// Game buttons
	Framework::AddButtonInput( pGameInput, E_Button_Throttle,						new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_RightTrigger));
	Framework::AddButtonInput( pGameInput, E_Button_Brake,							new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_LeftTrigger));
	Framework::AddButtonInput( pGameInput, E_Button_Handbrake,						new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_B));
	Framework::AddButtonInput( pGameInput, E_Button_ChangeView,						new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_RightShoulder));
	Framework::AddButtonInput( pGameInput, E_Button_UsePickup,						new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_A));
	Framework::AddButtonInput( pGameInput, E_Button_DiscardPickup,					new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_Y));
	Framework::AddButtonInput( pGameInput, E_Button_SwitchPickupAlt,				new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_LeftShoulder));
	Framework::AddButtonInput( pGameInput, E_Button_SwitchPickup,					new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_Y));
	Framework::AddButtonInput( pGameInput, E_Button_UseShield,						new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_X));
	Framework::AddButtonInput( pGameInput, E_Button_ResetVehicle,					new C_ButtonInputFromDevice(pPad,    C_Pad360::E_Button_RightShoulder));
	Framework::AddButtonInput( pGameInput, E_Button_UseSecondTierPickup,			new C_ButtonInputFromDevice(pPad,    C_Pad360::E_Button_Y));
	Framework::AddButtonInput( pGameInput, E_Button_Auto_Shuffle_Perks,				new C_ButtonInputFromDevice(pPad,    C_Pad360::E_Button_DPadUp));
	Framework::AddButtonInput( pGameInput, E_Button_Convert_To_Health,				new C_ButtonInputFromDevice(pPad,    C_Pad360::E_Button_DPadLeft));
	Framework::AddButtonInput( pGameInput, E_Button_Clone_Selected_Perk,			new C_ButtonInputFromDevice(pPad,    C_Pad360::E_Button_DPadRight));
	Framework::AddButtonInput( pGameInput, E_Button_Fullhouse,						new C_ButtonInputFromDevice(pPad,    C_Pad360::E_Button_DPadDown));

	Framework::AddButtonInput( pGameInput, E_Button_PhotoRoll,						new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_LeftTrigger));
	Framework::AddButtonInput( pGameInput, E_Button_PhotoZoom,						new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_RightTrigger));
	pGameInput->SetButtonInput( E_Button_PhotoRollReset,							new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_LeftThumb));
	pGameInput->SetButtonInput( E_Button_PhotoZoomReset,							new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_RightThumb));

	// Game axis
	Framework::AddAxisInput( pGameInput, E_Axis_Steering,							new C_AxisInputFromDevice(pPad, 	C_Pad360::E_Axis_LeftStick_X));
	Framework::AddAxisInput( pGameInput, E_Axis_Pitch,								new C_AxisInputFromDevice(pPad, 	C_Pad360::E_Axis_LeftStick_Y));

	// Ui buttons
	Framework::AddButtonInput( pGameInput, E_UiButton_Pad_Up,						new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_DPadUp));
	Framework::AddButtonInput( pGameInput, E_UiButton_Pad_Down,						new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_DPadDown));
	Framework::AddButtonInput( pGameInput, E_UiButton_Pad_Left,						new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_DPadLeft));
	Framework::AddButtonInput( pGameInput, E_UiButton_Pad_Right,					new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_DPadRight));
	Framework::AddButtonInput( pGameInput, E_UiButton_Confirm,						new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_A));
	Framework::AddButtonInput( pGameInput, E_UiButton_Cancel,						new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_B));
	Framework::AddButtonInput( pGameInput, E_UiButton_Option1,						new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_X));
	Framework::AddButtonInput( pGameInput, E_UiButton_Option2,						new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_Y));
	Framework::AddButtonInput( pGameInput, E_UiButton_LeftTrigger,					new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_LeftTrigger));
	Framework::AddButtonInput( pGameInput, E_UiButton_RightTrigger,					new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_RightTrigger));
	Framework::AddButtonInput( pGameInput, E_UiButton_LeftShoulder,					new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_LeftShoulder));
	Framework::AddButtonInput( pGameInput, E_UiButton_RightShoulder,				new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_RightShoulder));
	Framework::AddButtonInput( pGameInput, E_UiButton_LeftThumb,					new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_LeftThumb));
	Framework::AddButtonInput( pGameInput, E_UiButton_RightThumb,					new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_RightThumb));
	Framework::AddButtonInput( pGameInput, E_UiButton_Start,						new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_Start));
	Framework::AddButtonInput( pGameInput, E_UiButton_Back,							new C_ButtonInputFromDevice(pPad, 	C_Pad360::E_Button_Back));
	
	// Ui Axis
	Framework::AddAxisInput( pGameInput, E_UiAxis_LeftStick_UD,						new C_AxisInputFromDevice(pPad, 		C_Pad360::E_Axis_LeftStick_Y));
	Framework::AddAxisInput( pGameInput, E_UiAxis_LeftStick_LR,						new C_AxisInputFromDevice(pPad, 		C_Pad360::E_Axis_LeftStick_X));
	Framework::AddAxisInput( pGameInput, E_UiAxis_RightStick_UD,					new C_AxisInputFromDevice(pPad, 		C_Pad360::E_Axis_RightStick_Y));
	Framework::AddAxisInput( pGameInput, E_UiAxis_RightStick_LR,					new C_AxisInputFromDevice(pPad, 		C_Pad360::E_Axis_RightStick_X));
	
	#endif

	// Combine input from DPad and axis together according to kPadOrAxisSetup
	for ( U32 i = 0; i < lengthof( kPadOrAxisSetup ); i++ )
	{
		const C_PadOrAxisSetup& setup = kPadOrAxisSetup[i];
		
		// create combiner button
		C_ButtonInputCombiner* pButton = new C_ButtonInputCombiner();

		// add pad button
		pButton->AddButtonInput(new C_ButtonInputFromDevice( pPad, setup.m_Pad ));

		// add axis button
		pButton->AddButtonInput(new C_ButtonInputFromAxis(new C_AxisInputFromDevice( pPad, setup.m_Axis), setup.m_InvertAxis, 0.5f));

#if defined (PLATFORM_PC)
		// add key if keyboard present
		if(pKeyboard)
		{
			pButton->AddButtonInput(new C_ButtonInputFromDevice( pKeyboard, setup.m_Key ));
		}
#endif

		// set game input
		pGameInput->SetButtonInput( setup.m_PadOrAxis, pButton );
	}
}

} // namespace Game

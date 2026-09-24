GUI =
{
	finished = false,	
	carousel_branch = "Options",
	
	OutputSelection = -1,

	GetSelections = function()
		UIGlobals.OptionsTable.fx_vol		= UIButtons.GetSelection( GUI.SoundEffectsId )
		UIGlobals.OptionsTable.music_vol	= UIButtons.GetSelection( GUI.MusicId )
		UIGlobals.OptionsTable.voice_vol	= UIButtons.GetSelection( GUI.VoiceId )
		UIGlobals.OptionsTable.menu_vol		= UIButtons.GetSelection( GUI.MenuMusicId )
		UIGlobals.OptionsTable.music_type	= UIButtons.GetSelection( GUI.MusicModeId )
		UIGlobals.OptionsTable.dynamic		= UIButtons.GetSelection( GUI.DynamicRangeId )
		UIGlobals.OptionsTable.output		= UIButtons.GetSelection( GUI.OutputId )
	end,
	
	SetSelections = function()
		UIButtons.SetSelection( GUI.SoundEffectsId, UIGlobals.OptionsTable.fx_vol )
		UIButtons.SetSelection( GUI.MusicId, UIGlobals.OptionsTable.music_vol )
		--UIButtons.SetSelection( GUI.VoiceId, UIGlobals.OptionsTable.voice_vol )
		UIButtons.SetSelection( GUI.MenuMusicId, UIGlobals.OptionsTable.menu_vol )
		UIButtons.SetSelection( GUI.MusicModeId, UIGlobals.OptionsTable.music_type )
		UIButtons.SetSelection( GUI.OutputId, UIGlobals.OptionsTable.output )
		UIButtons.SetSelection( GUI.DynamicRangeId, UIGlobals.OptionsTable.dynamic )
	end,
	
	HaveChangedSelections = function()
		return ( GUI.StartingOptionsTable.fx_vol	~= UIButtons.GetSelection( GUI.SoundEffectsId ) or 
				GUI.StartingOptionsTable.music_vol	~= UIButtons.GetSelection( GUI.MusicId ) or
				--GUI.StartingOptionsTable.voice_vol	~= UIButtons.GetSelection( GUI.VoiceId ) or
				GUI.StartingOptionsTable.menu_vol	~= UIButtons.GetSelection( GUI.MenuMusicId ) or
				GUI.StartingOptionsTable.music_type	~= UIButtons.GetSelection( GUI.MusicModeId ) or 
				GUI.StartingOptionsTable.dynamic	~= UIButtons.GetSelection( GUI.DynamicRangeId ) or
				GUI.StartingOptionsTable.output		~= UIButtons.GetSelection( GUI.OutputId ) )
	end,
	
	UpdateInfoLine = function()
		local selection = UIButtons.GetSelectionIndex( GUI.MessageListId )
		if GUI.selection ~= selection then
			GUI.selection = selection 
			if selection == 6 then
				SetupInfoLine( UIText.INFO_A_SELECT, UIText.INFO_B_CANCEL, UIText.INFO_RESET_TO_DEFAULT )
			else
				SetupInfoLine(UIText.INFO_A_CONFIRM, UIText.INFO_B_CANCEL, UIText.INFO_RESET_TO_DEFAULT)
			end
		end
	end,
	
	bottom_help_text = {
		UIText.CMN_HLP_AUDIO_SFX,
		UIText.CMN_HLP_AUDIO_MUSIC,
		UIText.CMN_HLP_AUDIO_MENU_MUSIC,
		UIText.CMN_HLP_AUDIO_MUSIC_MODE,
		UIText.CMN_HLP_AUDIO_DYNAMIC_RANGE,
		UIText.CMN_HLP_AUDIO_OUTPUT,
		UIText.CMN_HLP_AUDIO_ADVANCED,
						},
		

	CanExit = function( _GUI )
		return false
	end,
	
	CheckAdvancedOption = function()
		-- Enable advanced audio options
		if UIGlobals.OptionsTable.output == 1 then
			UIButtons.SetNodeItemLocked( GUI.MessageListId, 6, false )
			UIButtons.SetActive( GUI.AdvancedNodeId, true )
		else
			UIButtons.SetNodeItemLocked( GUI.MessageListId, 6, true )
			UIButtons.SetActive( GUI.AdvancedNodeId, false)
		end
	end,
	
	UpdateAdvancedOption = function()
		if UIButtons.GetSelectionIndex( GUI.MessageListId ) ~= 5 then
			return
		end
		
		local selection = UIButtons.GetSelectionIndex( GUI.OutputId )
		if GUI.OutputSelection == selection then
			return
		end
		
		GUI.CheckAdvancedOption()
		
		GUI.OutputSelection = selection
	end,
}

function Init()
	AddSCUI_Elements()
	
	GUI.MessageListId = SCUI.name_to_id[ "OptionsList" ]
	
	GUI.SoundEffectsId	= SCUI.name_to_id[ "SDR_SoundEffects" ]
	GUI.MusicId			= SCUI.name_to_id[ "SDR_Music" ]
	GUI.VoiceId			= SCUI.name_to_id[ "SDR_Voice" ]
	GUI.MenuMusicId		= SCUI.name_to_id[ "SDR_MenuMusic" ]
	GUI.MusicModeId		= SCUI.name_to_id[ "SDR_MusicMode" ]
	GUI.DynamicRangeId	= SCUI.name_to_id[ "SDR_DynamicRange" ]
	GUI.OutputId		= SCUI.name_to_id[ "SDR_Ouput" ]
	
	GUI.AdvancedNodeId	= SCUI.name_to_id[ "Nde_Advanced" ]
	
	local items = 
	{
		GUI.SoundEffectsId,
		GUI.MusicId,	
		--GUI.VoiceId,
		GUI.MenuMusicId,
	}
	
	-- Add 10 items to each
	for j, v in ipairs( items ) do
		for i = 0, 9, 1 do
			UIButtons.AddItem( v, i, i, false )
		end
	end	
	
	UIButtons.AddItem( GUI.MusicModeId, 0, UIText.CMN_OFF, false )
	UIButtons.AddItem( GUI.MusicModeId, 1, UIText.CMN_ON, false )

	UIButtons.AddItem( GUI.OutputId, 0, UIText.CMN_OPT_AUDIO_STEREO, false )
	UIButtons.AddItem( GUI.OutputId, 1, UIText.CMN_OPT_AUDIO_SURROUND, false )

	UIButtons.AddItem( GUI.DynamicRangeId, 0, UIText.CMN_OPT_AUDIO_TV, false )
	UIButtons.AddItem( GUI.DynamicRangeId, 1, UIText.CMN_OPT_AUDIO_HOME_THEATRE, false )
	
	GUI.SetSelections()
	
	-- Enable advanced audio options
	GUI.CheckAdvancedOption()
		
	StoreInfoLine()
	SetupInfoLine( UIText.INFO_A_SELECT, UIText.INFO_B_CANCEL )
	
	GUI.UpdateAdvancedOption()
	GUI.StartingOptionsTable = CloneTable( UIGlobals.OptionsTable )
	
	SetupScreenTitle( UIText.CMN_AUDIO_BASIC, SCUI.name_to_id["Dummy"], "large_speaker" )
end

function PostInit()
	-- set up our "button" for advanced options
	local box_id, text_id = SetupSelectionBox( GUI.AdvancedNodeId, UIText.CMN_OPT_ADVANCED_AUDIO_OPTIONS, nil, nil, nil, nil, UIEnums.Justify.MiddleCentre )
	UIButtons.ChangeScale( box_id, 1.4, 1.4 )
	UIButtons.ChangePosition( box_id, 14, 1.5 ) -- cough
	
	UIButtons.NodeListScan( GUI.MessageListId )
	local s = GetScreenOtions().selection
	if IsNumber( s ) == true then
		UIButtons.SetSelectionByIndex( GUI.MessageListId, s )
		GUI.bottom_text_id = SetupBottomHelpBar( GUI.bottom_help_text[s+1] )
	else
		UIButtons.SetSelectionByIndex( GUI.MessageListId, 0 )
		GUI.bottom_text_id = SetupBottomHelpBar( GUI.bottom_help_text[1] )
	end
	
	GUI.UpdateInfoLine()
	GUI.do_update_options = true
end


function StartLoop( time )
	--
end

function MessageUpdate( message, arg1, arg2, arg3, arg4 )
	if SCUI_MessageUpdate( message, arg1, arg2, arg3, arg4 ) == true then
		return
	end
	
	if message == UIEnums.Message.PopupNext then
		if arg2 == UIEnums.CustomPopups.DiscardOptionsChanges and arg3 == UIEnums.PopupOptions.Yes then
			ResetToStarting()
			GoScreen("Shared\\Options.lua")
		elseif arg2 == UIEnums.CustomPopups.ResetToDefaults and arg3 == UIEnums.PopupOptions.Yes then
			ResetToDefaults()
		end
	end

	if message == UIEnums.Message.MenuBack and arg2 == true then
		if GUI.HaveChangedSelections() == true then
			SetupCustomPopup( UIEnums.CustomPopups.DiscardOptionsChanges )
		else
			PlaySfxBack()
			GoScreen("Shared\\Options.lua")
		end
	elseif message == UIEnums.Message.MenuNext and arg2 == true then
		local selected = UIButtons.GetSelectionIndex( GUI.MessageListId )
		if selected == 6 then
			PlaySfxNext()
			GoScreen("Shared\\Options_AdvancedAudio.lua")
		else
			PlaySfxNext()
			
			-- Store selections
			GUI.GetSelections()
			GoScreen("Shared\\Options.lua")
		end
	elseif message == UIEnums.Message.ButtonLeft or message == UIEnums.Message.ButtonRight then
		GUI.do_update_options = true
	elseif message == UIEnums.Message.ButtonDown or message == UIEnums.Message.ButtonUp then
		GUI.do_update_options = true
	elseif message == UIEnums.Message.ButtonX then
		SetupCustomPopup( UIEnums.CustomPopups.ResetToDefaults )
	end
end

function FrameUpdate( time )
	GUI.UpdateInfoLine()
	
	--
	if GUI.do_update_options == true then
		GUI.GetSelections()
		
		Amax.UpdateAudioOptions( UIGlobals.OptionsTable )
		
		local selected = UIButtons.GetSelectionIndex( GUI.MessageListId )
		UIButtons.ChangeText( GUI.bottom_text_id, GUI.bottom_help_text[ selected + 1 ] )
		UIButtons.TimeLineActive( "HelpFade", true, 0.5 )
		GUI.do_update_options = false
	end
	
	GUI.UpdateAdvancedOption()
end

function Render()
	--
end

function EnterEnd()
	RestoreInfoLine()
end

function EndLoop( time )
	--
end

function End()
	GetScreenOtions().selection = UIButtons.GetSelectionIndex( GUI.MessageListId )
end

function ResetToStarting()
	UIGlobals.OptionsTable.fx_vol		= GUI.StartingOptionsTable.fx_vol		
	UIGlobals.OptionsTable.music_vol	= GUI.StartingOptionsTable.music_vol
	UIGlobals.OptionsTable.voice_vol	= GUI.StartingOptionsTable.voice_vol
	UIGlobals.OptionsTable.menu_vol		= GUI.StartingOptionsTable.menu_vol	
	UIGlobals.OptionsTable.music_type	= GUI.StartingOptionsTable.music_type
	UIGlobals.OptionsTable.dynamic		= GUI.StartingOptionsTable.dynamic	
	UIGlobals.OptionsTable.output		= GUI.StartingOptionsTable.output	
	Amax.UpdateAudioOptions( UIGlobals.OptionsTable )
end

function ResetToDefaults()
	UIGlobals.OptionsTable.fx_vol		= UIGlobals.DefaultOptionsTable.fx_vol
	UIGlobals.OptionsTable.music_vol	= UIGlobals.DefaultOptionsTable.music_vol
	UIGlobals.OptionsTable.voice_vol	= UIGlobals.DefaultOptionsTable.voice_vol
	UIGlobals.OptionsTable.menu_vol		= UIGlobals.DefaultOptionsTable.menu_vol	
	UIGlobals.OptionsTable.music_type	= UIGlobals.DefaultOptionsTable.music_type
	UIGlobals.OptionsTable.dynamic		= UIGlobals.DefaultOptionsTable.dynamic	
	UIGlobals.OptionsTable.output		= UIGlobals.DefaultOptionsTable.output	
	
	GUI.SetSelections()
	GUI.CheckAdvancedOption()
	GUI.StartingOptionsTable = CloneTable( UIGlobals.OptionsTable )
	--[mtaylor] 37081 (Audio updates when options are reset to default)
	Amax.UpdateAudioOptions( UIGlobals.OptionsTable )
end

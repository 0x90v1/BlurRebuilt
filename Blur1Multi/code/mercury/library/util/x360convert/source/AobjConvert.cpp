// [Blur1Multi] Audio object definitions (audio/dat/*.dat, tag 'AOBJ'): Xbox 360 and PS3 -> PC.
//
// Paths below are relative to Blur1PC/code/mercury/library/audio/source/ unless given in full.
// The PS3 and 360 retail discs ship the same .dat bytes (370 of 371 files identical;
// docs/analysis.md "Console audio: banks and audio objects"), so one converter serves both.
//
// Reader: Definition.cpp:73-128 (Definition::Init) and Object.cpp:449-1120
// (Object::CalculateComponentFootprint, the walk every loader repeats). Writer: the exporter
// Blur1PC/code/mercury/tools/Audio/XMLtoDat/audioDatafile.cpp:720-800 (AudioDataFile::Save).
//
//   header, 6 x U32 (Definition.cpp:84-125): tag 'AOBJ' (read as the U32 0x414f424a, so the
//     PC file holds "JBOA"), version (2 << 16) | 11 (include/Audio.h:61-62), string-table
//     offset, object-name offset, component count, frame skip
//   component count x { U32 type (components/Component.h:139 enum Types), Data } back to
//     back; the walk advances by <Class>::DataSize(Data) (Object.cpp:449-1120), which is
//     sizeof(Data) plus, for some classes, arrays whose counts are U16 ArraySize members
//   the string table: NUL-terminated names, bytes, up to the end of the file
//
// Field widths come from the Data structs (components/*.h and the Compressor/Flanger bases
// in components/Component.h), with the reader's types (Component.h:62-68): StrOffset,
// AudioFloat, AudioInt S32/F32; ArraySize, Com, Out, Msg 16-bit; Control and Event
// {Com, Out/Msg}; Option {Control, AudioFloat}. The exporter writes the header, each type
// word, effect lists and script bodies as swapped U32s (CommandLineApp.h WriteBytes) and the
// Data members through per-width convertors (XMLtoDat/DataTypeConversion.cpp), so every
// field is swapped at its own width and struct padding is copied (it holds whatever the
// exporter's memory held; it differs from the PC files and the engine never reads it).
//
// MixerComponent::Flags (Component.h:728, a U32 bitfield) is the exception: the console
// files hold the same four bytes as the PC files (10 00 00 00 in all 225 console and
// 337 PC mixers), and the reader only uses nSurroundMode, which both layouts read as 0
// from those bytes. The mapping of any other value is unknown, so another value fails.
//
// An unknown component type, or one whose reader adds no data size (eSurroundDelay,
// Object.cpp:1015), fails the file: its width is not known.

#include <x360convert/X360Convert.h>

#include "ChunkFile.h"

#include <stdio.h>
#include <string.h>

namespace X360Convert
{
	namespace
	{
		// A struct as a string of its members in order, natural alignment:
		//   I = 32-bit (swapped), H = 16-bit (swapped), O = Option (H H I),
		//   F = MixerComponent::Flags (copied, see above)
		struct Tail
		{
			unsigned countOffset;		// offset of the U16 ArraySize count in Data
			const char* element;		// element shape; 0 = none
		};
		struct ComponentLayout
		{
			unsigned type;
			const char* name;
			const char* data;			// Data shape
			Tail tail[2];				// arrays after Data, in file order
		};

		// Generated from the headers named on each line and checked against every retail
		// PC, PS3 and 360 .dat (each walks to its string table). Array order is the one
		// the Init functions use (effects first: MonoToMonoMixer.cpp:25,
		// MonoOneShotSoundTable.cpp:33).
		const ComponentLayout kLayouts[] =
		{
		{   0, "ParameterIn", "II", { { 0, 0 }, { 0, 0 } } },	// ParameterIn.h:12, DataSize ParameterIn.h:62
		{   1, "ParameterOut", "IO", { { 0, 0 }, { 0, 0 } } },	// ParameterOut.h:16, DataSize ParameterOut.h:48
		{   2, "ActionIn", "IHH", { { 0, 0 }, { 0, 0 } } },	// ActionIn.h:16, DataSize ActionIn.h:52
		{   3, "ActionOut", "I", { { 0, 0 }, { 0, 0 } } },	// ActionOut.h:16, DataSize ActionOut.h:61
		{   4, "VectorIn", "IIII", { { 0, 0 }, { 0, 0 } } },	// VectorIn.h:12, DataSize VectorIn.h:63
		{   6, "ControlInput", "I", { { 0, 0 }, { 0, 0 } } },	// ControlInput.h:13, DataSize ControlInput.h:50
		{   7, "ControlOutput", "IO", { { 0, 0 }, { 0, 0 } } },	// ControlOutput.h:14, DataSize ControlOutput.h:51
		{   8, "EventInput", "IHH", { { 0, 0 }, { 0, 0 } } },	// EventInput.h:13, DataSize EventInput.h:45
		{   9, "EventOutput", "I", { { 0, 0 }, { 0, 0 } } },	// EventOutput.h:12, DataSize EventOutput.h:56
		{  10, "CoordinateInput", "I", { { 0, 0 }, { 0, 0 } } },	// CoordinateInput.h:13, DataSize CoordinateInput.h:51
		{  11, "CoordinateOutput", "IH", { { 0, 0 }, { 0, 0 } } },	// CoordinateOutput.h:12, DataSize CoordinateOutput.h:52
		{  12, "MonoAudioInput", "I", { { 0, 0 }, { 0, 0 } } },	// MonoAudioInput.h:15, DataSize MonoAudioInput.h:48
		{  13, "MonoAudioOutput", "IH", { { 0, 0 }, { 0, 0 } } },	// MonoAudioOutput.h:16, DataSize MonoAudioOutput.h:50
		{  14, "StereoAudioInput", "I", { { 0, 0 }, { 0, 0 } } },	// StereoAudioInput.h:15, DataSize StereoAudioInput.h:47
		{  15, "StereoAudioOutput", "IH", { { 0, 0 }, { 0, 0 } } },	// StereoAudioOutput.h:15, DataSize StereoAudioOutput.h:45
		{  16, "SurroundAudioInput", "I", { { 0, 0 }, { 0, 0 } } },	// SurroundAudioInput.h:13, DataSize SurroundAudioInput.h:46
		{  17, "SurroundAudioOutput", "IH", { { 0, 0 }, { 0, 0 } } },	// SurroundAudioOutput.h:14, DataSize SurroundAudioOutput.h:44
		{  18, "SurroundLFEAudioInput", "I", { { 0, 0 }, { 0, 0 } } },	// SurroundLFEAudioInput.h:13, DataSize SurroundLFEAudioInput.h:44
		{  19, "SurroundLFEAudioOutput", "IH", { { 0, 0 }, { 0, 0 } } },	// SurroundLFEAudioOutput.h:14, DataSize SurroundLFEAudioOutput.h:46
		{  20, "Variable", "II", { { 0, 0 }, { 0, 0 } } },	// Variable.h:12, DataSize Variable.h:47
		{  21, "PiecewiseLinearFunction", "IHHOH", { { 16, "IO" }, { 0, 0 } } },	// PiecewiseLinearFunction.h:12, DataSize PiecewiseLinearFunction.h:61
		{  23, "Source", "IOIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIHHHHHHHHHHH", { { 0, 0 }, { 0, 0 } } },	// Source.h:15, DataSize Source.h:124
		{  24, "Rolloff", "IHHOOOOO", { { 0, 0 }, { 0, 0 } } },	// Rolloff.h:12, DataSize Rolloff.h:60
		{  25, "Cone", "IHHOOOOOOO", { { 0, 0 }, { 0, 0 } } },	// Cone.h:18, DataSize Cone.h:78
		{  26, "LowPass", "IHHO", { { 0, 0 }, { 0, 0 } } },	// LowPass.h:12, DataSize LowPass.h:54
		{  28, "Timer", "IOOOHH", { { 0, 0 }, { 0, 0 } } },	// Timer.h:12, DataSize Timer.h:67
		{  29, "Metronome", "IOOOHHHHHH", { { 0, 0 }, { 0, 0 } } },	// Metronome.h:13, DataSize Metronome.h:68
		{  32, "Random", "IOO", { { 0, 0 }, { 0, 0 } } },	// Random.h:12, DataSize Random.h:55
		{  33, "Selector", "IOH", { { 12, "OOOO" }, { 0, 0 } } },	// Selector.h:14, DataSize Selector.h:70
		{  34, "StateMachine", "IIHHH", { { 12, "HHHH" }, { 0, 0 } } },	// StateMachine.h:12, DataSize StateMachine.h:65
		{  35, "Quantize", "IHHOO", { { 0, 0 }, { 0, 0 } } },	// Quantize.h:12, DataSize Quantize.h:47
		{  36, "Sum", "IH", { { 4, "O" }, { 0, 0 } } },	// Sum.h:12, DataSize Sum.h:53
		{  37, "Product", "IH", { { 4, "O" }, { 0, 0 } } },	// Product.h:12, DataSize Product.h:53
		{  40, "Exponent", "IHHO", { { 0, 0 }, { 0, 0 } } },	// Exponent.h:12, DataSize Exponent.h:51
		{  41, "Max", "IH", { { 4, "O" }, { 0, 0 } } },	// Max.h:12, DataSize Max.h:59
		{  42, "Min", "IH", { { 4, "O" }, { 0, 0 } } },	// Min.h:12, DataSize Min.h:58
		{  43, "Invert", "IHH", { { 0, 0 }, { 0, 0 } } },	// Invert.h:12, DataSize Invert.h:51
		{  44, "Gradient", "IHH", { { 0, 0 }, { 0, 0 } } },	// Gradient.h:12, DataSize Gradient.h:50
		{  45, "Spring", "IHHOO", { { 0, 0 }, { 0, 0 } } },	// Spring.h:16, DataSize Spring.h:57
		{  47, "Sine", "IHH", { { 0, 0 }, { 0, 0 } } },	// Sin.h:12, DataSize Sin.h:47
		{  51, "SampleAndHold", "IHH", { { 0, 0 }, { 0, 0 } } },	// SampleAndHold.h:12, DataSize SampleAndHold.h:54
		{  52, "Envelope", "IHHOOOOHHHHHHHH", { { 0, 0 }, { 0, 0 } } },	// Envelope.h:12, DataSize Envelope.h:59
		{  54, "GateSwitch", "IHHOOOOHHHH", { { 0, 0 }, { 0, 0 } } },	// GateSwitch.h:12, DataSize GateSwitch.h:57
		{  55, "Array", "IHHOH", { { 16, "O" }, { 0, 0 } } },	// Array.h:12, DataSize Array.h:62
		{  56, "Fraction", "IHH", { { 0, 0 }, { 0, 0 } } },	// Fraction.h:12, DataSize Fraction.h:51
		{  59, "MonoOneShotSound", "IIIOOOOHHHH", { { 48, "I" }, { 0, 0 } } },	// MonoOneShotSound.h:12, DataSize MonoOneShotSound.h:94
		{  60, "StereoOneShotSound", "IIIOOOOHHHH", { { 48, "I" }, { 0, 0 } } },	// StereoOneShotSound.h:12, DataSize StereoOneShotSound.h:94
		{  61, "MonoOneShotSoundTable", "IIOOOOOHHHHH", { { 52, "I" }, { 56, "I" } } },	// MonoOneShotSoundTable.h:12, DataSize MonoOneShotSoundTable.h:118
		{  62, "StereoOneShotSoundTable", "IIOOOOOHHHHH", { { 52, "I" }, { 56, "I" } } },	// StereoOneShotSoundTable.h:12, DataSize StereoOneShotSoundTable.h:118
		{  63, "MonoLoopingSound", "IIIOOOOOHHH", { { 56, "I" }, { 0, 0 } } },	// MonoLoopingSound.h:12, DataSize MonoLoopingSound.h:90
		{  64, "StereoLoopingSound", "IIIOOOOOHHH", { { 56, "I" }, { 0, 0 } } },	// StereoLoopingSound.h:12, DataSize StereoLoopingSound.h:90
		{  68, "MonoToMonoMixer", "IOOHH", { { 20, "I" }, { 22, "OOH" } } },	// MonoToMonoMixer.h:12, DataSize MonoToMonoMixer.h:62
		{  69, "MonoToStereoMixer", "IOOHH", { { 20, "I" }, { 22, "OOOOH" } } },	// MonoToStereoMixer.h:12, DataSize MonoToStereoMixer.h:75
		{  70, "MonoToSurroundMixer", "IFOOOOOOHH", { { 56, "I" }, { 58, "OOH" } } },	// MonoToSurroundMixer.h:12, DataSize MonoToSurroundMixer.h:78
		{  71, "MonoToSurroundLFEMixer", "IFOOOOOOOOHH", { { 72, "I" }, { 74, "OOH" } } },	// MonoToSurroundLFEMixer.h:12, DataSize MonoToSurroundLFEMixer.h:78
		{  72, "StereoToMonoMixer", "IOOHH", { { 20, "I" }, { 22, "OOOOH" } } },	// StereoToMonoMixer.h:12, DataSize StereoToMonoMixer.h:69
		{  73, "StereoToStereoMixer", "IOOHH", { { 20, "I" }, { 22, "OOOOOOOOH" } } },	// StereoToStereoMixer.h:12, DataSize StereoToStereoMixer.h:87
		{  74, "StereoToQuadMixer", "IOOHH", { { 20, "I" }, { 22, "OOOOOOOOOOOOOOOOH" } } },	// StereoToQuadMixer.h:12, DataSize StereoToQuadMixer.h:119
		{  75, "StereoToSurroundMixer", "IFOOOOOOOOOOHH", { { 88, "I" }, { 90, "OOOOH" } } },	// StereoToSurroundMixer.h:12, DataSize StereoToSurroundMixer.h:86
		{  76, "QuadToSurroundMixer", "IFOOOOOOOOOOOOOOOOOOHH", { { 152, "I" }, { 154, "OOOOOOOOH" } } },	// QuadToSurroundMixer.h:12, DataSize QuadToSurroundMixer.h:110
		{  79, "SurroundToSurroundMixer", "IOOHH", { { 20, "I" }, { 22, "OOH" } } },	// SurroundToSurroundMixer.h:12, DataSize SurroundToSurroundMixer.h:64
		{  80, "SurroundToSurroundLFEMixer", "IOOOOHH", { { 36, "I" }, { 38, "OOH" } } },	// SurroundToSurroundLFEMixer.h:12, DataSize SurroundToSurroundLFEMixer.h:66
		{  81, "SurroundLFEToSurroundLFEMixer", "IOOHH", { { 20, "I" }, { 22, "OOH" } } },	// SurroundLFEToSurroundLFEMixer.h:12, DataSize SurroundLFEToSurroundLFEMixer.h:68
		{ 112, "MonoDelay", "IIIOOO", { { 0, 0 }, { 0, 0 } } },	// MonoDelay.h:12, DataSize MonoDelay.h:64
		// 114 eSurroundDelay: Object.cpp:1015 adds no data size (no reader); unsupported
		{ 118, "MonoRMSLevel", "IO", { { 0, 0 }, { 0, 0 } } },	// MonoRMSLevel.h:12, DataSize MonoRMSLevel.h:59
		{ 119, "StereoRMSLevel", "IO", { { 0, 0 }, { 0, 0 } } },	// StereoRMSLevel.h:12, DataSize StereoRMSLevel.h:62
		{ 120, "SurroundRMSLevel", "IO", { { 0, 0 }, { 0, 0 } } },	// SurroundRMSLevel.h:12, DataSize SurroundRMSLevel.h:59
		{ 121, "MonoSaturator", "IOOOO", { { 0, 0 }, { 0, 0 } } },	// MonoSaturator.h:12, DataSize MonoSaturator.h:56
		{ 122, "StereoSaturator", "IOOOO", { { 0, 0 }, { 0, 0 } } },	// StereoSaturator.h:12, DataSize StereoSaturator.h:56
		{ 123, "SurroundSaturator", "IOOOO", { { 0, 0 }, { 0, 0 } } },	// SurroundSaturator.h:12, DataSize SurroundSaturator.h:56
		{ 130, "MonoConvolution", "IIIIIIIII", { { 0, 0 }, { 0, 0 } } },	// MonoConvolution.h:12, DataSize MonoConvolution.h:61
		{ 142, "MonoLowPass", "IOOOOO", { { 0, 0 }, { 0, 0 } } },	// MonoLowPass.h:12, DataSize MonoLowPass.h:56
		{ 143, "StereoLowPass", "IOOOOO", { { 0, 0 }, { 0, 0 } } },	// StereoLowPass.h:12, DataSize StereoLowPass.h:56
		{ 144, "SurroundLowPass", "IOOOOO", { { 0, 0 }, { 0, 0 } } },	// SurroundLowPass.h:12, DataSize SurroundLowPass.h:56
		{ 145, "MonoHighPass", "IOOOOO", { { 0, 0 }, { 0, 0 } } },	// MonoHighPass.h:12, DataSize MonoHighPass.h:63
		{ 146, "StereoHighPass", "IOOOOO", { { 0, 0 }, { 0, 0 } } },	// StereoHighPass.h:12, DataSize StereoHighPass.h:57
		{ 147, "SurroundHighPass", "IOOOOO", { { 0, 0 }, { 0, 0 } } },	// SurroundHighPass.h:12, DataSize SurroundHighPass.h:57
		{ 148, "MonoResonantLowPass", "IOOOOOOO", { { 0, 0 }, { 0, 0 } } },	// MonoResonantLowPass.h:12, DataSize MonoResonantLowPass.h:60
		{ 149, "StereoResonantLowPass", "IOOOOOOO", { { 0, 0 }, { 0, 0 } } },	// StereoResonantLowPass.h:12, DataSize StereoResonantLowPass.h:60
		{ 150, "SurroundResonantLowPass", "IOOOOOOO", { { 0, 0 }, { 0, 0 } } },	// SurroundResonantLowPass.h:12, DataSize SurroundResonantLowPass.h:60
		{ 154, "Mono24dBResonantLowPass", "IOOOOOOO", { { 0, 0 }, { 0, 0 } } },	// Mono24dBResonantLowPass.h:12, DataSize Mono24dBResonantLowPass.h:60
		{ 156, "Surround24dBResonantLowPass", "IOOOOOOO", { { 0, 0 }, { 0, 0 } } },	// Surround24dBResonantLowPass.h:12, DataSize Surround24dBResonantLowPass.h:60
		{ 161, "StereoHighShelf", "IOOOOOOO", { { 0, 0 }, { 0, 0 } } },	// StereoHighShelf.h:12, DataSize StereoHighShelf.h:66
		{ 163, "MonoPeakEQ", "IOOOOOOO", { { 0, 0 }, { 0, 0 } } },	// MonoPeakEQ.h:12, DataSize MonoPeakEQ.h:60
		{ 164, "StereoPeakEQ", "IOOOOOOO", { { 0, 0 }, { 0, 0 } } },	// StereoPeakEQ.h:12, DataSize StereoPeakEQ.h:62
		{ 165, "SurroundPeakEQ", "IOOOOOOO", { { 0, 0 }, { 0, 0 } } },	// SurroundPeakEQ.h:12, DataSize SurroundPeakEQ.h:63
		{ 166, "MonoCompressor", "IHHOOOOOOO", { { 0, 0 }, { 0, 0 } } },	// Component.h:832, DataSize MonoCompressor.h:35
		{ 167, "StereoCompressor", "IHHOOOOOOO", { { 0, 0 }, { 0, 0 } } },	// Component.h:832, DataSize StereoCompressor.h:29
		{ 168, "SurroundCompressor", "IHHOOOOOOO", { { 0, 0 }, { 0, 0 } } },	// Component.h:832, DataSize SurroundCompressor.h:29
		{ 170, "StereoPitchShift", "IOOOO", { { 0, 0 }, { 0, 0 } } },	// StereoPitchShift.h:12, DataSize StereoPitchShift.h:57
		{ 189, "Script", "IH", { { 4, "I" }, { 0, 0 } } },	// Script.h:12, DataSize Script.h:98
		{ 190, "Vector", "IOOOH", { { 0, 0 }, { 0, 0 } } },	// Vector.h:12, DataSize Vector.h:56
		{ 191, "VectorNearestPointOnLine", "IHHH", { { 0, 0 }, { 0, 0 } } },	// VectorNearestPointOnLine.h:12, DataSize VectorNearestPointOnLine.h:54
		{ 192, "DopplerShift", "IOHHHH", { { 0, 0 }, { 0, 0 } } },	// DopplerShift.h:12, DataSize DopplerShift.h:62
		{ 193, "DebugPrint", "IIHH", { { 0, 0 }, { 0, 0 } } },	// DebugPrint.h:12, DataSize DebugPrint.h:66
		{ 194, "Sequence", "IHHH", { { 8, "IHHO" }, { 0, 0 } } },	// Sequence.h:12, DataSize Sequence.h:65
		{ 195, "MonoSpectralReverb", "IOOOOOOOOOOOOOOOO", { { 0, 0 }, { 0, 0 } } },	// MonoSpectralReverb.h:14, DataSize MonoSpectralReverb.h:112
		{ 196, "MonoToAmbisonicsMixer", "IOOOOHH", { { 36, "I" }, { 38, "OOH" } } },	// MonoToAmbisonicsMixer.h:12, DataSize MonoToAmbisonicsMixer.h:79
		{ 197, "AmbisonicsToSurroundMixer", "IOOOHH", { { 28, "I" }, { 30, "OOH" } } },	// AmbisonicsToSurroundMixer.h:12, DataSize AmbisonicsToSurroundMixer.h:66
		{ 198, "ImpactFilter", "IHHOOHH", { { 0, 0 }, { 0, 0 } } },	// ImpactFilter.h:12, DataSize ImpactFilter.h:63
		{ 199, "MessageDelay", "IOHH", { { 0, 0 }, { 0, 0 } } },	// MessageDelay.h:12, DataSize MessageDelay.h:52
		{ 202, "MonoFlanger", "IIOOOOOO", { { 0, 0 }, { 0, 0 } } },	// Component.h:889, DataSize MonoFlanger.h:35
		{ 204, "SurroundFlanger", "IIOOOOOO", { { 0, 0 }, { 0, 0 } } },	// Component.h:889, DataSize SurroundFlanger.h:35
		{ 205, "MonoBitCrusher", "IOOO", { { 0, 0 }, { 0, 0 } } },	// MonoBitCrusher.h:12, DataSize MonoBitCrusher.h:54
		{ 207, "SurroundBitCrusher", "IOOO", { { 0, 0 }, { 0, 0 } } },	// SurroundBitCrusher.h:12, DataSize SurroundBitCrusher.h:54
		{ 208, "MonoRingModulator", "IOOO", { { 0, 0 }, { 0, 0 } } },	// MonoRingModulator.h:12, DataSize MonoRingModulator.h:54
		{ 210, "SurroundRingModulator", "IOOO", { { 0, 0 }, { 0, 0 } } },	// SurroundRingModulator.h:12, DataSize SurroundRingModulator.h:56
		{ 214, "MonoSend", "IOOH", { { 0, 0 }, { 0, 0 } } },	// MonoSend.h:12, DataSize MonoSend.h:60
		{ 215, "StereoSend", "IOOH", { { 0, 0 }, { 0, 0 } } },	// StereoSend.h:12, DataSize StereoSend.h:60
		{ 217, "MonoReturn", "IOOH", { { 20, "I" }, { 0, 0 } } },	// MonoReturn.h:12, DataSize MonoReturn.h:60
		{ 218, "StereoReturn", "IOOH", { { 20, "I" }, { 0, 0 } } },	// StereoReturn.h:12, DataSize StereoReturn.h:60
		};

		const ComponentLayout* FindLayout(unsigned type)
		{
			for (size_t i = 0; i < sizeof(kLayouts) / sizeof(kLayouts[0]); ++i)
				if (kLayouts[i].type == type)
					return &kLayouts[i];
			return NULL;
		}

		void Swap16(unsigned char* p) { unsigned char t = p[0]; p[0] = p[1]; p[1] = t; }
		void Swap32(unsigned char* p) { unsigned char t = p[0]; p[0] = p[3]; p[3] = t; t = p[1]; p[1] = p[2]; p[2] = t; }

		E_Result Fail(std::string& error, const char* format, unsigned a = 0, unsigned b = 0, unsigned c = 0)
		{
			char buf[256];
			sprintf_s(buf, "aobj: ");
			sprintf_s(buf + 6, sizeof(buf) - 6, format, a, b, c);
			error = buf;
			return E_Result_Failed;
		}

		// Size of a shape with natural alignment, rounded to its largest member.
		unsigned ShapeSize(const char* shape)
		{
			unsigned off = 0, align = 2;
			for (const char* s = shape; *s; ++s)
			{
				if (*s == 'H')
					off = (off + 1) & ~1u, off += 2;
				else if (*s == 'O')
					off = (off + 3) & ~3u, off += 8, align = 4;
				else
					off = (off + 3) & ~3u, off += 4, align = 4;
			}
			return (off + align - 1) & ~(align - 1);
		}

		// Swaps one shape's fields at d (the output copy); false when a Flags word is not the known value.
		bool SwapShape(const char* shape, unsigned char* d)
		{
			unsigned off = 0;
			for (const char* s = shape; *s; ++s)
			{
				switch (*s)
				{
				case 'H':
					off = (off + 1) & ~1u;
					Swap16(d + off);
					off += 2;
					break;
				case 'O':
					off = (off + 3) & ~3u;
					Swap16(d + off);
					Swap16(d + off + 2);
					Swap32(d + off + 4);
					off += 8;
					break;
				case 'F':
					off = (off + 3) & ~3u;
					if (d[off] != 0x10 || d[off + 1] || d[off + 2] || d[off + 3])
						return false;
					off += 4;
					break;
				default:	// 'I'
					off = (off + 3) & ~3u;
					Swap32(d + off);
					off += 4;
					break;
				}
			}
			return true;
		}
	}

	E_Result ConvertAudioObject(const unsigned char* in, size_t inSize, std::vector<unsigned char>& out, std::string& error)
	{
		const size_t kHeader = 24;
		if (inSize < kHeader || memcmp(in, "AOBJ", 4) != 0)
			return Fail(error, "not a big-endian AOBJ file");
		const unsigned version = BE32(in + 4);
		if (version != ((2u << 16) | 11u))
			return Fail(error, "version %u.%u, expected 2.11", version >> 16, version & 0xffff);
		const unsigned table = BE32(in + 8), count = BE32(in + 16);
		if (table < kHeader || table > inSize)
			return Fail(error, "string table offset %u outside the file (%u bytes)", table, (unsigned)inSize);

		out.assign(in, in + inSize);
		unsigned char* d = &out[0];
		for (unsigned i = 0; i < kHeader; i += 4)
			Swap32(d + i);

		size_t at = kHeader;
		for (unsigned c = 0; c < count; ++c)
		{
			if (at + 4 > table)
				return Fail(error, "component %u of %u starts past the string table (0x%x)", c, count, (unsigned)at);
			const unsigned type = BE32(in + at);
			const ComponentLayout* layout = FindLayout(type);
			if (!layout)
				return Fail(error, "component %u at 0x%x has type %u, which has no known layout", c, (unsigned)at, type);
			Swap32(d + at);
			const size_t data = at + 4;
			size_t end = data + ShapeSize(layout->data);
			if (end > table)
				return Fail(error, "component %u (type %u) runs past the string table", c, type);
			if (!SwapShape(layout->data, d + data))
				return Fail(error, "component %u (type %u): mixer flags word is not 10 00 00 00", c, type);
			for (int t = 0; t < 2 && layout->tail[t].element; ++t)
			{
				const unsigned n = BE16(in + data + layout->tail[t].countOffset);
				const unsigned size = ShapeSize(layout->tail[t].element);
				if (end + (size_t)n * size > table)
					return Fail(error, "component %u (type %u): %u array entries run past the string table", c, type, n);
				for (unsigned k = 0; k < n; ++k, end += size)
					SwapShape(layout->tail[t].element, d + end);
			}
			at = end;
		}
		if (at != table)
			return Fail(error, "components end at 0x%x, the string table starts at 0x%x", (unsigned)at, table);
		return E_Result_Converted;
	}
}

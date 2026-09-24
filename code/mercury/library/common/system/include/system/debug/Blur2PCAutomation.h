// [Blur2PC] Scripted automation for unattended test runs.
//
// Active only when the environment variable BLUR2PC_AUTOMATION names a script file. The script
// injects keyboard input into the game's own keyboard device and requests back-buffer screenshots,
// so test runs need neither window focus nor desktop capture and do not disturb whoever is using
// the machine. While a script is active the game also ignores focus loss (no auto-pause).
//
// Script format: one command per line, '#' starts a comment. Times are seconds since the first
// keyboard update (roughly: since the window opened).
//     <time> tap  <KEY> [hold_ms]     press and release (default hold 150 ms)
//     <time> down <KEY>               press and keep held
//     <time> up   <KEY>               release
//     <time> shot <file.png>          save the next presented frame (path relative to the script)
//     <time> quit                     exit the process
// KEY: RETURN ESCAPE SPACE TAB UP DOWN LEFT RIGHT, A-Z, 0-9, F1-F12.
//
// Header-only on purpose: the input, display and framework libraries all include it, and inline
// functions with function-local statics share one instance across translation units.
#ifndef SYSTEM_DEBUG_BLUR2PC_AUTOMATION_H
#define SYSTEM_DEBUG_BLUR2PC_AUTOMATION_H

#if defined(PLATFORM_PC)

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace Blur2PCAutomation
{
	enum E_Kind { E_Down, E_Up, E_Shot, E_Quit };

	struct Event
	{
		double time;
		E_Kind kind;
		int dik;
		char file[MAX_PATH];
	};

	struct State
	{
		bool loaded;
		bool active;
		Event events[512];
		int count;
		int next;
		bool keys[256];
		char pendingShot[MAX_PATH];
		double shotTime;
		char baseDir[MAX_PATH];
		LARGE_INTEGER start;
		LARGE_INTEGER freq;
		bool started;
		FILE* log;
	};

	inline State& Get()
	{
		static State s_state;
		return s_state;
	}

	inline int KeyFromName(const char* name)
	{
		struct Pair { const char* n; int dik; };
		static const Pair table[] = {
			{"RETURN",0x1C},{"ENTER",0x1C},{"ESCAPE",0x01},{"ESC",0x01},{"SPACE",0x39},{"TAB",0x0F},
			{"UP",0xC8},{"DOWN",0xD0},{"LEFT",0xCB},{"RIGHT",0xCD},
			{"Q",0x10},{"W",0x11},{"E",0x12},{"R",0x13},{"T",0x14},{"Y",0x15},{"U",0x16},{"I",0x17},{"O",0x18},{"P",0x19},
			{"A",0x1E},{"S",0x1F},{"D",0x20},{"F",0x21},{"G",0x22},{"H",0x23},{"J",0x24},{"K",0x25},{"L",0x26},
			{"Z",0x2C},{"X",0x2D},{"C",0x2E},{"V",0x2F},{"B",0x30},{"N",0x31},{"M",0x32},
			{"1",0x02},{"2",0x03},{"3",0x04},{"4",0x05},{"5",0x06},{"6",0x07},{"7",0x08},{"8",0x09},{"9",0x0A},{"0",0x0B},
			{"F1",0x3B},{"F2",0x3C},{"F3",0x3D},{"F4",0x3E},{"F5",0x3F},{"F6",0x40},{"F7",0x41},{"F8",0x42},
			{"F9",0x43},{"F10",0x44},{"F11",0x57},{"F12",0x58},
		};
		for (size_t i = 0; i < sizeof(table) / sizeof(table[0]); ++i)
			if (_stricmp(table[i].n, name) == 0)
				return table[i].dik;
		return -1;
	}

	inline void Log(const char* fmt, const char* a = "", double t = 0.0)
	{
		State& s = Get();
		if (s.log) { fprintf(s.log, fmt, t, a); fflush(s.log); }
	}

	inline void AddEvent(double time, E_Kind kind, int dik, const char* file)
	{
		State& s = Get();
		if (s.count >= (int)(sizeof(s.events) / sizeof(s.events[0])))
			return;
		Event& e = s.events[s.count++];
		e.time = time; e.kind = kind; e.dik = dik; e.file[0] = 0;
		if (file)
			_snprintf_s(e.file, MAX_PATH, _TRUNCATE, "%s%s", s.baseDir, file);
		// keep sorted by time (insertion sort, scripts are short)
		for (int i = s.count - 1; i > 0 && s.events[i - 1].time > s.events[i].time; --i)
		{
			Event tmp = s.events[i]; s.events[i] = s.events[i - 1]; s.events[i - 1] = tmp;
		}
	}

	inline void Load()
	{
		State& s = Get();
		if (s.loaded)
			return;
		s.loaded = true;
		const char* path = getenv("BLUR2PC_AUTOMATION");
		if (!path || !*path)
			return;
		FILE* f = fopen(path, "r");
		if (!f)
			return;
		strncpy_s(s.baseDir, path, _TRUNCATE);
		char* slash = strrchr(s.baseDir, '\\');
		if (!slash) slash = strrchr(s.baseDir, '/');
		if (slash) slash[1] = 0; else s.baseDir[0] = 0;
		char logPath[MAX_PATH];
		_snprintf_s(logPath, MAX_PATH, _TRUNCATE, "%sautomation.log", s.baseDir);
		s.log = fopen(logPath, "w");

		char line[512];
		while (fgets(line, sizeof(line), f))
		{
			char* hash = strchr(line, '#');
			if (hash) *hash = 0;
			double t = 0; char cmd[32] = {0}; char arg[400] = {0}; int hold = 150;
			const int n = sscanf_s(line, "%lf %31s %399s %d", &t, cmd, (unsigned)sizeof(cmd), arg, (unsigned)sizeof(arg), &hold);
			if (n < 2)
				continue;
			if (_stricmp(cmd, "quit") == 0) { AddEvent(t, E_Quit, 0, NULL); continue; }
			if (n < 3)
				continue;
			if (_stricmp(cmd, "shot") == 0) { AddEvent(t, E_Shot, 0, arg); continue; }
			const int dik = KeyFromName(arg);
			if (dik < 0) { Log("%8.2f unknown key %s\n", arg, t); continue; }
			if (_stricmp(cmd, "tap") == 0) { AddEvent(t, E_Down, dik, NULL); AddEvent(t + (n >= 4 ? hold : 150) / 1000.0, E_Up, dik, NULL); }
			else if (_stricmp(cmd, "down") == 0) AddEvent(t, E_Down, dik, NULL);
			else if (_stricmp(cmd, "up") == 0) AddEvent(t, E_Up, dik, NULL);
		}
		fclose(f);
		s.active = true;
		Log("%8.2f script loaded: %s\n", path, 0.0);
	}

	inline bool IsActive()
	{
		Load();
		return Get().active;
	}

	//! Called once per keyboard update; applies due events. Returns false when inactive.
	inline bool Update()
	{
		if (!IsActive())
			return false;
		State& s = Get();
		LARGE_INTEGER now;
		QueryPerformanceCounter(&now);
		if (!s.started)
		{
			QueryPerformanceFrequency(&s.freq);
			s.start = now;
			s.started = true;
		}
		const double t = (double)(now.QuadPart - s.start.QuadPart) / (double)s.freq.QuadPart;
		bool pressedNow[256] = { false };
		while (s.next < s.count && s.events[s.next].time <= t)
		{
			// a release in the same update as its press would be invisible to the game (seen on
			// frame hitches); leave it and everything after it for the next update
			if (s.events[s.next].kind == E_Up && pressedNow[s.events[s.next].dik & 0xFF])
				break;
			if (s.events[s.next].kind == E_Down)
				pressedNow[s.events[s.next].dik & 0xFF] = true;
			const Event& e = s.events[s.next++];
			switch (e.kind)
			{
			case E_Down: s.keys[e.dik & 0xFF] = true;  Log("%8.2f key down %s\n", "", t); break;
			case E_Up:   s.keys[e.dik & 0xFF] = false; Log("%8.2f key up %s\n", "", t); break;
			case E_Shot: strncpy_s(s.pendingShot, e.file, _TRUNCATE); s.shotTime = t; break;
			case E_Quit: Log("%8.2f quit%s\n", "", t); if (s.log) fclose(s.log); ExitProcess(0);
			}
		}
		return true;
	}

	//! Scripted state of a DirectInput key (DIK_* code).
	inline bool IsKeyDown(int dik)
	{
		return Get().active && dik >= 0 && dik < 256 && Get().keys[dik];
	}

	//! Returns the requested screenshot path once, or NULL.
	inline const char* TakeShotRequest()
	{
		State& s = Get();
		if (!s.active || !s.pendingShot[0])
			return NULL;
		static char path[MAX_PATH];
		strncpy_s(path, s.pendingShot, _TRUNCATE);
		s.pendingShot[0] = 0;
		Log("%8.2f shot %s\n", path, 0.0);
		return path;
	}
}

#endif // PLATFORM_PC
#endif // SYSTEM_DEBUG_BLUR2PC_AUTOMATION_H

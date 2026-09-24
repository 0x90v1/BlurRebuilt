#pragma once

#define WIN32_LEAN_AND_MEAN
#define SECURITY_WIN32
#define _WIN32_WINNT 0x5000

#include <Windows.h>
#include <WindowsX.h>
#include <CommCtrl.h>
#include <TChar.h>
#include <WinSock2.h>
#include <Security.h>
#include <Math.h>
#include <Assert.h>
#include <StdIo.h>
#include <StdArg.h>
#include <StdLib.h>

#pragma warning(disable:4355)	// This used in initialiser list
#pragma warning(disable:4706)	// Assignment within conditional expression
#pragma warning(disable:4200)	// Zero sized array in struct

typedef unsigned __int8		u8;
typedef unsigned __int16	U16;
typedef unsigned __int32	U32;
typedef unsigned __int64	u64;

typedef signed __int8		i8;
typedef signed __int16		i16;
typedef signed __int32		i32;
typedef signed __int64		i64;

typedef float				f32;
typedef double				f64;
typedef long double			f80;

// Clamp "Value" so it is never less than "Low", or greater than "High".

template<class T> inline T Clamp(const T Value, const T Low, const T High)
{
	return (Low > Value) ? Low : ((Value > High) ? High : Value);
}

// Wrap "Value" between "Low" and "High", will never equal "High", beware if "Value"
// is outside twice the range of "Low" to "High", then the wrapped value will also be
// outside of the "Low" to "High" range.

template<class T> inline T Wrap(const T Value, const T Low, const T High)
{
	return (Low > Value) ? (Value + (High - Low)) : ((Value > High) ? (Value - (High - Low)) : Value);
}

// Return the larger of "Lhs" and "Rhs".

template<class T> inline T Max(const T Lhs, const T Rhs)
{
	return (Rhs > Lhs) ? Rhs : Lhs;
}

// Return the smaller of "Lhs" and "Rhs".

template<class T> inline T Min(const T Lhs, const T Rhs)
{
	return (Lhs > Rhs) ? Rhs : Lhs;
}

// Return the square of "Value".

template<class T> inline T Square(const T Value)
{
	return Value * Value;
}

// Return the square of "Value", but persists the original sign of the number.

template<class T> inline T SquarePersistSign(const T Value)
{
	return (Value < 0) ? -(Value * Value) : (Value * Value);
}

template<> inline float SquarePersistSign(const float Value)
{
	return fabsf(Value) * Value;
}

// Return the cube of "Value".

template<class T> inline T Cube(const T Value)
{
	return Value * Value * Value;
}

// Linear interpolation between "Low" and "High" by "Time".

template<class T, class S> inline T Lerp(const T Low, const T High, const S Time)
{
	return Low + (High - Low) * Time;
}

// Exchange the values of "Lhs" and "Rhs".

template<class T> inline void Swap(T& Lhs, T& Rhs)
{
	const T Tmp(Lhs);
	Lhs = Rhs;
	Rhs = Tmp;
}

// Set "Var" to "Value", then returns the previous value of "Var".

template<class T> inline T PushAssign(T& Var, const T Value)
{
	T Old(Var);
	Var = Value;
	return Old;
}

// Cast the input into a Dword, most useful with SetRenderStates which require a float input.

template<class T> inline DWORD DwordCast(const T Value)
{
	return *((DWORD*)&Value);
}

// Count the number of items in a fixed array.

template<class T> inline ArrayCount(const T& Array)
{
	return sizeof(Array) / sizeof(Array[0]);
}

// Return the byte offset to "Member" in a structure.  Ex. MemberOffset<Vector4::vz>()

template<void* const Member> int MemberOffset()
{
	return (int)Member;
}

void DebugMessage(const char* Prefix, const char* Message, va_list vl);
void DebugInfo(const char* Message, ...);
void DebugWarning(const char* Message, ...);
void DebugError(const char* Message, ...);

class String
{
public:
	inline String(LPCTSTR Text = 0, size_t Length = 0xFFFFFFFF) : m_Text(0) { Set(Text, Length); }
	inline String(String& rhs) : m_Text(0) { Set(rhs.m_Text); }
	inline ~String() { Set(0); }

	inline String& operator=(LPCTSTR Text) { Set(Text); return *this; }
	inline String& operator=(String& rhs) { Set(rhs.m_Text); return *this; }

	inline operator LPCTSTR() const { return Get(); }

public:
	inline void Set(LPCTSTR Text, size_t Length = 0xFFFFFFFF)
	{
		LPTSTR m_OldText = m_Text;

		if (Text)
		{
			if (Length == 0xFFFFFFFF)
				Length = lstrlen(Text);

			if (m_Text = new TCHAR[Length + 1])
			{
				memcpy(m_Text, Text, Length * sizeof(TCHAR));
				m_Text[Length] = '\0';
			}
		}
		else
			m_Text = 0;

		delete [] m_OldText;
	}

	inline LPCTSTR Get() const { return m_Text ? m_Text : _T(""); }

private:
	LPTSTR m_Text;
};

template<class T>
class GenericList
{
public:
	GenericList() : m_List(0), m_Size(0), m_Capacity(0) { }
	~GenericList() { Clear(); }

public:
	GenericList& operator=(GenericList& Rhs)
	{
		Clear();

		if (CheckSpace(Rhs.GetSize()))
		{
			for(int i = 0; i < Rhs.m_Size; i++)
				Add(new T(*Rhs.Get(i)));
		}

		return *this;
	}

public:
	void Clear()
	{
		for(int i = 0; i < m_Size; i++)
			delete m_List[i];
		delete m_List;
		m_Capacity = 0;
		m_Size = 0;
		m_List = 0;
	}

	T* Add(T* ToAdd)
	{
		if (ToAdd)
		{
			if (!CheckSpace())
			{
				delete ToAdd;
				return 0;
			}

			m_List[m_Size++] = ToAdd;
		}

		return ToAdd;
	}

	T* Insert(int Before, T* ToInsert)
	{
		if (ToInsert)
		{
			if (!CheckSpace())
			{
				delete ToInsert;
				return 0;
			}

			if (Before < 0)
				Before = 0;
			else if (Before > m_Size)
				Before = m_Size;

			memmove(&m_List[Before + 1], &m_List[Before], (m_Size - Before) * sizeof(T*));

			m_List[Before] = ToInsert;

			m_Size++;
		}

		return ToInsert;
	}

	T* Detach(int Index)
	{
		if ((Index < 0) || (Index >= m_Size))
			return 0;

		T* Result = m_List[Index];

		memcpy(&m_List[Index], &m_List[Index + 1], (m_Size - Index) * sizeof(T*));
		memset(&m_List[--m_Size], 0, sizeof(T*));

		return Result;
	}

	bool Delete(int Index)
	{
		if ((Index < 0) || (Index >= m_Size))
			return false;

		delete m_List[Index];

		memcpy(&m_List[Index], &m_List[Index + 1], (m_Size - Index) * sizeof(T*));
		memset(&m_List[--m_Size], 0, sizeof(T*));

		return true;
	}

	bool Delete(T* ToDelete)
	{
		return Delete(Find(ToDelete));
	}

	int Find(T* ToFind)
	{
		for(int i = 0; i < m_Size; i++)
			if (m_List[i] == ToFind)
				return i;
		return -1;
	}

	int GetSize() { return m_Size; }
	T* Get(int Index) { return ((Index < 0) || (Index >= m_Size)) ? 0 : m_List[Index]; }

private:
	bool CheckSpace(int NewItems = 1)
	{
		T** List;
		int Capacity;

		if ((m_Size + NewItems) <= m_Capacity)
			return true;

		if ((List = new T*[Capacity = 2 * (m_Size + NewItems + 3)]) == 0)
			return false;

		memcpy(List, m_List, m_Size * sizeof(T*));
		memset(List + m_Size, 0, (Capacity - m_Size) * sizeof(T*));

		delete m_List;

		m_Capacity	= Capacity;
		m_List		= List;

		return true;
	}

protected:
	T** m_List;
	int m_Size;
	int m_Capacity;
};

String SnapAtom(const char*& Text);
int SnapInt(const char*& Text);
char SnapChar(const char*& Text);
String SnapString(const char*& Text);
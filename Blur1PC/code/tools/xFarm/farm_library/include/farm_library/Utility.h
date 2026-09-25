#ifndef __XFARM_UTILITY__
#define __XFARM_UTILITY__

// Set "Var" to "Value", then returns the previous value of "Var".
template<class T> inline T PushAssign(T& Var, const T Value)
{
	T Old(Var);
	Var = Value;
	return Old;
}

class String
{
public:
	inline String(const char* Text = 0, size_t Length = 0xFFFFFFFF) : m_Text(0) { Set(Text, Length); }
	inline String(String& rhs) : m_Text(0) { Set(rhs.m_Text); }
	inline ~String() { Set(0); }

	inline String& operator=(const char* Text) { Set(Text); return *this; }
	inline String& operator=(String& rhs) { Set(rhs.m_Text); return *this; }

	inline operator const char*() const { return Get(); }

public:
	inline void Set(const char* Text, size_t Length = 0xFFFFFFFF)
	{
		char* m_OldText = m_Text;

		if (Text)
		{
			if (Length == 0xFFFFFFFF)
				Length = strlen(Text);

			if (m_Text = new char[Length + 1])
			{
				memcpy(m_Text, Text, Length * sizeof(char));
				m_Text[Length] = '\0';
			}
		}
		else
			m_Text = 0;

		delete [] m_OldText;
	}

	inline const char* Get() const { return m_Text ? m_Text : ""; }

private:
	char* m_Text;
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

void FarmPrint(const char *c, ...);

String SnapAtom(const char*& Text);
int SnapInt(const char*& Text);
char SnapChar(const char*& Text);
String SnapString(const char*& Text);

void InitFarmPrintLog();

#endif

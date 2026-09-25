#pragma once

extern int g_UiMessage;

enum UiMessage
{
	UiM_ClearJobs,
	UiM_ClearMachines,
	UiM_NewJob,
	UiM_NewMachine,
	UiM_SetList,
	UiM_SetServerTime,
	UiM_Quit
};

struct UiJob
{
	int		Idx;
	int		JobNo;
	int		ProcessNo;
	char	State;
	String	Command;
	String	Comment;
	String	User;
	DWORD	TimeStarted;

public:
	UiJob() :
			Idx(-1),
			JobNo(-1),
			ProcessNo(-1),
			State(' '),
			TimeStarted(0)
	{
	}
};

struct UiMachine
{
	int		ProcessNo;
	String	Machine;
	int		JobIdx;
	bool	IsEnabled;

public:
	UiMachine() :
			ProcessNo(-1),
			JobIdx(-1),
			IsEnabled(true)
	{
	}
};

enum UiCommandType
{
	UiCT_Version,
	UiCT_ListJobs,
	UiCT_ListMachines,
	UiCT_Job,
	UiCT_Kill,
	UiCT_Priority,
	UiCT_Requeue,
	UiCT_UpdateCommand,
	UiCT_UpdateComment,
	UiCT_Enable,
	UiCT_Disable,
	UiCT_Quit
};

struct UiCommand
{
	UiCommandType	Type;
	int				JobNo;
	String			Command;
	String			Comment;

public:
	UiCommand(UiCommandType Type_, int JobNo_ = -1, const char* Command_ = "", const char* Comment_ = "") :
			Type(Type_),
			JobNo(JobNo_),
			Command(Command_),
			Comment(Comment_)
	{
	}
};

struct UiNewJob
{
	bool	Editing;
	String	Command;
	String	Comment;

public:
	UiNewJob(bool Editing_) :
			Editing(Editing_)
	{
	}
};

class FarmWindow : public Window
{
public:
	FarmWindow(bool IsManager);
	virtual ~FarmWindow();

public:
	virtual LPCTSTR GetWindowTitle();

public:
	virtual LRESULT WndProc(UINT Msg, WPARAM wParam, LPARAM lParam);

private:
	void Process();
	void ProcessUiMessage(WPARAM wParam, LPARAM lParam);

private:
	enum ViewMode
	{
		VM_Jobs,
		VM_Machines
	};

private:
	void SetViewMode(ViewMode Mode);
	void SetListItems(bool UpdateValues);

private:
	UiCommand* GetCommand();
	bool AddCommand(UiCommand* Cmd);

private:
	static int JobSortProc(const void* A, const void* B);
	static int MachineSortProc(const void* A, const void* B);

private:
	static BOOL CALLBACK NewJobDlgProc(HWND Dlg, int Msg, WPARAM wParam, LPARAM lParam);
	static DWORD CALLBACK ThreadProcessProc(void* Parameter);

private:
	HWND					m_List;
	ViewMode				m_Mode;
	bool					m_IsManager;
	bool					m_IsConnected;
	String					m_UserName;

	HANDLE					m_Thread;
	HANDLE					m_Shutdown;
	DWORD					m_ServerTime;

	UiJob**					m_JobMap;
	GenericList<UiJob>		m_NewJobs;
	GenericList<UiJob>		m_Jobs;

	UiMachine**				m_MachineMap;
	GenericList<UiMachine>	m_NewMachines;
	GenericList<UiMachine>	m_Machines;

	CRITICAL_SECTION		m_Critical;
	GenericList<UiCommand>	m_Commands;

friend class FarmUser;
};
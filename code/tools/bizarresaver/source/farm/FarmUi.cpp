#include "farmpch.h"

#include "farm/Farm.h"
#include "resource/Resource.h"

const int FarmWindowListId = 4321;

int g_UiMessage;

FarmWindow::FarmWindow(bool IsManager) :
		m_List(0),
		m_Mode(VM_Jobs),
		m_IsManager(IsManager),
		m_IsConnected(false),
		m_ServerTime(0),
		m_Thread(INVALID_HANDLE_VALUE),
		m_JobMap(0),
		m_MachineMap(0)
{
	InitializeCriticalSection(&m_Critical);

	char Buffer[4096];
	ULONG Length(sizeof(Buffer));

	if (GetUserNameEx(NameSamCompatible, Buffer, &Length) != FALSE)
	{
		for(char* Ptr = Buffer; *Ptr; Ptr++)
			if (*Ptr == ' ')
				*Ptr = '_';

		m_UserName = Buffer;
	}

	m_Shutdown = CreateEvent(0, TRUE, FALSE, 0);

	g_UiMessage = RegisterWindowMessage("FarmUiMessage {EE64213C-5DE2-49e5-9D40-02B991A641F1}");
}

FarmWindow::~FarmWindow()
{
	delete [] m_JobMap;
	delete [] m_MachineMap;

	m_NewJobs.Clear();
	m_Jobs.Clear();

	m_NewMachines.Clear();
	m_Machines.Clear();

	EnterCriticalSection(&m_Critical);
	m_Commands.Clear();
	LeaveCriticalSection(&m_Critical);

	DeleteCriticalSection(&m_Critical);

	CloseHandle(m_Thread);
	CloseHandle(m_Shutdown);
}

LPCTSTR FarmWindow::GetWindowTitle()
{
	return _T("Farm Manager");
}

LRESULT FarmWindow::WndProc(UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch(Msg)
	{
		case WM_CREATE:
		{
			if (Window::WndProc(Msg, wParam, lParam) != 0)
				return -1;

			SetMenu(m_Wnd, LoadMenu(GetModuleHandle(0), MAKEINTRESOURCE(IDR_MAINMENU)));

			WinRect Rc;

			GetClientRect(m_Wnd, &Rc);

			SendMessage(m_Wnd, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(GetModuleHandle(0), MAKEINTRESOURCE(IDI_FARMWINDOW)));

			if ((m_List = CreateWindow(WC_LISTVIEW, 0,
					WS_VISIBLE | WS_CHILD | LVS_REPORT | LVS_OWNERDATA | LVS_SHOWSELALWAYS,
				0, 0, 0, 0, m_Wnd, (HMENU)FarmWindowListId, 0, 0)) == 0)
			{
				return -1;
			}

			SetWindowPos(m_List, 0, Rc.left, Rc.top, Rc.Width(), Rc.Height(), SWP_NOZORDER);

			ListView_SetExtendedListViewStyleEx(m_List,
					LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP,
					LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP
				);

			LVCOLUMN Col;
			ZeroMemory(&Col, sizeof(Col));
			Col.mask = LVCF_TEXT;
			Col.pszText	= "";
			ListView_InsertColumn(m_List, 0, &Col);

			SetViewMode(VM_Jobs);

			if ((m_Thread = CreateThread(0, 0, ThreadProcessProc, this, 0, 0)) == INVALID_HANDLE_VALUE)
				return -1;

			SetTimer(m_Wnd, 0, 2500, 0);

			SetFocus(m_List);
		}
		return 0;

		case WM_TIMER:
			AddCommand(new UiCommand(UiCT_Version));
		break;

		case WM_NOTIFY:
		{
			NMHDR* Hdr((NMHDR*)lParam);

			if (wParam == FarmWindowListId)
			{
				NMLVDISPINFO* Di((NMLVDISPINFO*)Hdr);

				switch(Di->hdr.code)
				{
					case LVN_GETDISPINFO:
					{
						LVITEM& Itm(Di->item);

						int Idx(Di->item.iItem);

						switch(m_Mode)
						{
							case VM_Jobs:
								if (m_JobMap && (Idx >= 0) && (Idx < m_Jobs.GetSize()))
								{
									if (UiJob* Jb = m_JobMap[Idx])
									{
										if (Itm.mask & LVIF_TEXT)
										{
											switch(Itm.iSubItem + ((!m_IsManager && (Itm.iSubItem > 0)) ? 1 : 0))
											{
												case 0:
													_snprintf(Itm.pszText, Itm.cchTextMax, "%i", Jb->JobNo);
												break;

												case 1:
													_snprintf(Itm.pszText, Itm.cchTextMax, "%s", Jb->Command);
												break;

												case 2:
													_snprintf(Itm.pszText, Itm.cchTextMax, "%s", Jb->Comment);
												break;

												case 3:
													_snprintf(Itm.pszText, Itm.cchTextMax, "%s", Jb->User);
												break;

												case 4:
													_snprintf(Itm.pszText, Itm.cchTextMax, "%s", (Jb->State == 'F') ? "Free" : "Active");
												break;

												default:
													_snprintf(Itm.pszText, Itm.cchTextMax, "");
												break;
											}
										}
									}
								}
							break;

							case VM_Machines:
								if (m_MachineMap && (Idx >= 0) && (Idx < m_Machines.GetSize()))
								{
									if (UiMachine* Mch = m_MachineMap[Idx])
									{
										if (Itm.mask & LVIF_TEXT)
										{
											if (UiJob* Jb = m_Jobs.Get(Mch->JobIdx))
											{
												switch(Itm.iSubItem + ((!m_IsManager && (Itm.iSubItem > 1)) ? 1 : 0))
												{
													case 0:
														_snprintf(Itm.pszText, Itm.cchTextMax, "%s", Mch->Machine);
													break;

													case 1:
														_snprintf(Itm.pszText, Itm.cchTextMax, "%i", Jb->JobNo);
													break;

													case 2:
														_snprintf(Itm.pszText, Itm.cchTextMax, "%s", Jb->Command);
													break;

													case 3:
														_snprintf(Itm.pszText, Itm.cchTextMax, "%s", Jb->Comment);
													break;

													case 4:
														_snprintf(Itm.pszText, Itm.cchTextMax, "%s", Jb->User);
													break;

													case 5:
														_snprintf(Itm.pszText, Itm.cchTextMax, "%s", (Mch->IsEnabled ? ((Jb->State == 'F') ? "Free" : "Active") : "Disabled"));
													break;

													default:
														_snprintf(Itm.pszText, Itm.cchTextMax, "");
													break;
												}
											}
											else
											{
												switch(Itm.iSubItem + ((!m_IsManager && (Itm.iSubItem > 1)) ? 1 : 0))
												{
													case 0:
														_snprintf(Itm.pszText, Itm.cchTextMax, "%s", Mch->Machine);
													break;

													case 5:
														_snprintf(Itm.pszText, Itm.cchTextMax, "%s", Mch->IsEnabled ? "Free" : "Disabled");
													break;

													default:
														_snprintf(Itm.pszText, Itm.cchTextMax, "");
													break;
												}
											}
										}
									}
								}
							break;
						}
					}
					break;

					case NM_DBLCLK:
					{
						NMITEMACTIVATE* Ia((NMITEMACTIVATE*)Hdr);

						if (Ia->iItem >= 0)
						{
							ListView_SetItemState(m_List, Ia->iItem, LVIS_SELECTED, LVIS_SELECTED);
							PostMessage(m_Wnd, WM_COMMAND, MAKELONG(ID_TOOL_EDITJOB, 0), 0);
						}
						else
							PostMessage(m_Wnd, WM_COMMAND, MAKELONG(ID_TOOL_NEWJOB, 0), 0);
					}
					break;
				}
			}
		}
		break;

		case WM_ENTERMENULOOP:
		{
			HMENU MainMenu(GetMenu(m_Wnd));
			HMENU ViewMenu(GetSubMenu(MainMenu, 0));
			HMENU ToolMenu(GetSubMenu(MainMenu, 1));

			CheckMenuRadioItem(ViewMenu, ID_VIEW_JOBS, ID_VIEW_MACHINES,
					(m_Mode == VM_Jobs) ? ID_VIEW_JOBS : ID_VIEW_MACHINES,
				MF_BYCOMMAND);

			if (m_IsManager)
			{
				if (m_IsConnected)
					EnableMenuItem(ToolMenu, ID_TOOL_NEWJOB, MF_BYCOMMAND | MF_ENABLED);
				else
					EnableMenuItem(ToolMenu, ID_TOOL_NEWJOB, MF_BYCOMMAND | MF_GRAYED);

				if (ListView_GetSelectedCount(m_List) > 0)
				{
					EnableMenuItem(ToolMenu, ID_TOOL_EDITJOB, MF_BYCOMMAND | MF_ENABLED);
					EnableMenuItem(ToolMenu, ID_TOOL_PRIORITISEJOB, MF_BYCOMMAND | MF_ENABLED);
					EnableMenuItem(ToolMenu, ID_TOOL_REQUEUEJOB, MF_BYCOMMAND | MF_ENABLED);
					EnableMenuItem(ToolMenu, ID_TOOL_KILLJOB, MF_BYCOMMAND | MF_ENABLED);
					EnableMenuItem(ToolMenu, ID_TOOL_ENABLEMACHINE, MF_BYCOMMAND | MF_ENABLED);
					EnableMenuItem(ToolMenu, ID_TOOL_DISABLEMACHINE, MF_BYCOMMAND | MF_ENABLED);
				}
				else
				{
					EnableMenuItem(ToolMenu, ID_TOOL_EDITJOB, MF_BYCOMMAND | MF_GRAYED);
					EnableMenuItem(ToolMenu, ID_TOOL_PRIORITISEJOB, MF_BYCOMMAND | MF_GRAYED);
					EnableMenuItem(ToolMenu, ID_TOOL_REQUEUEJOB, MF_BYCOMMAND | MF_GRAYED);
					EnableMenuItem(ToolMenu, ID_TOOL_KILLJOB, MF_BYCOMMAND | MF_GRAYED);
					EnableMenuItem(ToolMenu, ID_TOOL_ENABLEMACHINE, MF_BYCOMMAND | MF_GRAYED);
					EnableMenuItem(ToolMenu, ID_TOOL_DISABLEMACHINE, MF_BYCOMMAND | MF_GRAYED);
				}
			}
			else
			{
				EnableMenuItem(ToolMenu, ID_TOOL_NEWJOB, MF_BYCOMMAND | MF_GRAYED);
				EnableMenuItem(ToolMenu, ID_TOOL_EDITJOB, MF_BYCOMMAND | MF_GRAYED);
				EnableMenuItem(ToolMenu, ID_TOOL_PRIORITISEJOB, MF_BYCOMMAND | MF_GRAYED);
				EnableMenuItem(ToolMenu, ID_TOOL_REQUEUEJOB, MF_BYCOMMAND | MF_GRAYED);
				EnableMenuItem(ToolMenu, ID_TOOL_ENABLEMACHINE, MF_BYCOMMAND | MF_GRAYED);
				EnableMenuItem(ToolMenu, ID_TOOL_DISABLEMACHINE, MF_BYCOMMAND | MF_GRAYED);

				if (ListView_GetSelectedCount(m_List) > 0)
					EnableMenuItem(ToolMenu, ID_TOOL_KILLJOB, MF_BYCOMMAND | MF_ENABLED);
				else
					EnableMenuItem(ToolMenu, ID_TOOL_KILLJOB, MF_BYCOMMAND | MF_GRAYED);
			}
		}
		break;

		case WM_CONTEXTMENU:
		{
			HMENU MainMenu(GetMenu(m_Wnd));
			HMENU ToolMenu(GetSubMenu(MainMenu, 1));

			int X(LOWORD(lParam));
			int Y(HIWORD(lParam));

			if ((X == 0xFFFF) || (Y == 0xFFFF))
			{
				WinRect Rc;

				GetWindowRect(m_List, &Rc);

				X = Rc.left;
				Y = Rc.top;
			}

			TrackPopupMenu(ToolMenu, TPM_RIGHTBUTTON, X, Y, 0, m_Wnd, 0);
		}
		break;

		case WM_ACTIVATE:
			if (wParam == WA_ACTIVE)
				SetFocus(m_List);
		return 0;

		case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
				case ID_VIEW_JOBS:
					SetViewMode(VM_Jobs);
				break;

				case ID_VIEW_MACHINES:
					SetViewMode(VM_Machines);
				break;

				case ID_TOOL_NEWJOB:
					if (m_IsManager)
					{
						UiNewJob Job(false);

						if (DialogBoxParam(GetModuleHandle(0), MAKEINTRESOURCE(IDD_NEWJOB), m_Wnd, (DLGPROC)NewJobDlgProc, (LPARAM)&Job) > 0)
						{
							AddCommand(new UiCommand(UiCT_Job, -1, Job.Command, Job.Comment));
							AddCommand(new UiCommand(UiCT_Version));
						}

						SetFocus(m_List);
					}
				break;

				case ID_TOOL_EDITJOB:
					if (m_IsManager)
					{
						int First(-1);

						for(int First(-1); (First = ListView_GetNextItem(m_List, First, LVNI_SELECTED)) != -1; )
						{
							UiJob* Jb(0);

							switch(m_Mode)
							{
								case VM_Jobs:
									if ((First >= 0) && (First < m_Jobs.GetSize()))
										Jb = m_JobMap[First];
								break;

								case VM_Machines:
									if ((First >= 0) && (First < m_Machines.GetSize()))
										Jb = m_Jobs.Get(m_MachineMap[First]->JobIdx);
								break;
							}

							if (Jb)
							{
								UiNewJob Job(true);

								Job.Command = Jb->Command;
								Job.Comment = Jb->Comment;

								if (DialogBoxParam(GetModuleHandle(0), MAKEINTRESOURCE(IDD_NEWJOB), m_Wnd, (DLGPROC)NewJobDlgProc, (LPARAM)&Job) > 0)
								{
									AddCommand(new UiCommand(UiCT_UpdateCommand, Jb->JobNo, Job.Command));
									AddCommand(new UiCommand(UiCT_UpdateComment, Jb->JobNo, "", Job.Comment));
									AddCommand(new UiCommand(UiCT_Version));
								}
								else
									break;
							}
						}

						SetFocus(m_List);
					}
				break;

				case ID_TOOL_KILLJOB:
				{
					bool PermissionDenied(false);
					bool SendRefresh(false);
					int First(-1);

					for(int First(-1); (First = ListView_GetNextItem(m_List, First, LVNI_SELECTED)) != -1; )
					{
						UiJob* Jb(0);

						switch(m_Mode)
						{
							case VM_Jobs:
								if ((First >= 0) && (First < m_Jobs.GetSize()))
									Jb = m_JobMap[First];
							break;

							case VM_Machines:
								if ((First >= 0) && (First < m_Machines.GetSize()))
									Jb = m_Jobs.Get(m_MachineMap[First]->JobIdx);
							break;
						}

						if (Jb)
						{
							if (m_IsManager || (stricmp(Jb->User, m_UserName) == 0))
							{
								AddCommand(new UiCommand(UiCT_Kill, Jb->JobNo));
								SendRefresh = true;
							}
							else
								PermissionDenied = true;
						}
					}

					if (SendRefresh)
						AddCommand(new UiCommand(UiCT_Version));

					if (PermissionDenied)
						MessageBox(m_Wnd, "You do not have access rights to delete this job.", "Warning", MB_OK | MB_ICONEXCLAMATION);
				}
				break;

				case ID_TOOL_PRIORITISEJOB:
					if (m_IsManager)
					{
						int First(-1);

						for(int First(-1); (First = ListView_GetNextItem(m_List, First, LVNI_SELECTED)) != -1; )
						{
							UiJob* Jb(0);

							switch(m_Mode)
							{
								case VM_Jobs:
									if ((First >= 0) && (First < m_Jobs.GetSize()))
										Jb = m_JobMap[First];
								break;
							}

							if (Jb)
								AddCommand(new UiCommand(UiCT_Priority, Jb->JobNo));
						}

						AddCommand(new UiCommand(UiCT_Version));
					}
				break;

				case ID_TOOL_REQUEUEJOB:
					if (m_IsManager)
					{
						int First(-1);

						for(int First(-1); (First = ListView_GetNextItem(m_List, First, LVNI_SELECTED)) != -1; )
						{
							UiJob* Jb(0);

							switch(m_Mode)
							{
								case VM_Jobs:
									if ((First >= 0) && (First < m_Jobs.GetSize()))
										Jb = m_JobMap[First];
								break;

								case VM_Machines:
									if ((First >= 0) && (First < m_Machines.GetSize()))
										Jb = m_Jobs.Get(m_MachineMap[First]->JobIdx);
								break;
							}

							if (Jb)
								AddCommand(new UiCommand(UiCT_Requeue, Jb->JobNo));
						}

						AddCommand(new UiCommand(UiCT_Version));
					}
				break;

				case ID_TOOL_ENABLEMACHINE:
					if (m_IsManager)
					{
						int First(-1);

						for(int First(-1); (First = ListView_GetNextItem(m_List, First, LVNI_SELECTED)) != -1; )
						{
							UiMachine* Mch(0);

							switch(m_Mode)
							{
								case VM_Jobs:
									if ((First >= 0) && (First < m_Jobs.GetSize()))
									{
										if (m_JobMap[First]->ProcessNo != -1)
										{
											for(int i = 0; i < m_Machines.GetSize(); i++)
											{
												if (m_Machines.Get(i)->ProcessNo == m_JobMap[First]->ProcessNo)
												{
													Mch = m_Machines.Get(i);
													break;
												}
											}
										}
									}
								break;

								case VM_Machines:
									if ((First >= 0) && (First < m_Machines.GetSize()))
										Mch = m_MachineMap[First];
								break;
							}

							if (Mch)
								AddCommand(new UiCommand(UiCT_Enable, Mch->ProcessNo));
						}

						AddCommand(new UiCommand(UiCT_Version));
					}
				break;

				case ID_TOOL_DISABLEMACHINE:
					if (m_IsManager)
					{
						int First(-1);

						for(int First(-1); (First = ListView_GetNextItem(m_List, First, LVNI_SELECTED)) != -1; )
						{
							UiMachine* Mch(0);

							switch(m_Mode)
							{
								case VM_Jobs:
									if ((First >= 0) && (First < m_Jobs.GetSize()))
									{
										if (m_JobMap[First]->ProcessNo != -1)
										{
											for(int i = 0; i < m_Machines.GetSize(); i++)
											{
												if (m_Machines.Get(i)->ProcessNo == m_JobMap[First]->ProcessNo)
												{
													Mch = m_Machines.Get(i);
													break;
												}
											}
										}
									}
								break;

								case VM_Machines:
									if ((First >= 0) && (First < m_Machines.GetSize()))
										Mch = m_MachineMap[First];
								break;
							}

							if (Mch)
								AddCommand(new UiCommand(UiCT_Disable, Mch->ProcessNo));
						}

						AddCommand(new UiCommand(UiCT_Version));
					}
				break;
			}
		}
		break;

		case WM_SIZE:
		{
			WinRect Rc;

			GetClientRect(m_Wnd, &Rc);

			SetWindowPos(m_List, 0, Rc.left, Rc.top, Rc.Width(), Rc.Height(), SWP_NOZORDER);
		}
		break;

		case WM_CLOSE:
			SetEvent(m_Shutdown);
			AddCommand(new UiCommand(UiCT_Quit));
		return 0;

		case WM_DESTROY:
			PostQuitMessage(0);
		break;

		default:
			if (Msg == g_UiMessage)
			{
				ProcessUiMessage(wParam, lParam);
				return 0;
			}
		break;
	}

	return Window::WndProc(Msg, wParam, lParam);
}

void FarmWindow::Process()
{
	for(;;)
	{
		if (WaitForSingleObject(m_Shutdown, 0) == WAIT_OBJECT_0)
			break;

		SetWindowText(m_Wnd, "Farm Manager (Disconnected)");

		m_IsConnected = false;

		NwSockAddr ServerAddress;

		EnterCriticalSection(&m_Critical);
		m_Commands.Clear();
		LeaveCriticalSection(&m_Critical);

		if (!(ServerAddress = FindServer()).GetPort())
		{
			if (WaitForSingleObject(m_Shutdown, 5 * 1000) == WAIT_OBJECT_0)
				break;

			continue;
		}

		if (WaitForSingleObject(m_Shutdown, 0) == WAIT_OBJECT_0)
			break;

		EnterCriticalSection(&m_Critical);
		m_Commands.Clear();
		LeaveCriticalSection(&m_Critical);

		NwSocket Cli;

		if (Cli.Connect(ServerAddress))
		{
			if (WaitForSingleObject(m_Shutdown, 0) == WAIT_OBJECT_0)
				break;

			SetWindowText(m_Wnd, "Farm Manager");

			m_IsConnected = true;

			FarmProcess Prc(Cli, new FarmUser(*this));

			if (Prc.Start())
			{
				Prc.IsRunning(INFINITE);
				Prc.Stop();
			}
		}
	}

	PostMessage(m_Wnd, g_UiMessage, UiM_Quit, 0);
}

void FarmWindow::ProcessUiMessage(WPARAM wParam, LPARAM lParam)
{
	switch(wParam)
	{
		case UiM_ClearJobs:
			m_NewJobs.Clear();
		break;

		case UiM_ClearMachines:
			m_NewMachines.Clear();
		break;

		case UiM_NewJob:
			m_NewJobs.Add((UiJob*)lParam);
		break;

		case UiM_NewMachine:
			m_NewMachines.Add((UiMachine*)lParam);
		break;

		case UiM_SetList:
			SetListItems(true);
		break;

		case UiM_SetServerTime:
			m_ServerTime = lParam;
		break;

		case UiM_Quit:
			DestroyWindow(m_Wnd);
		break;
	}
}

void FarmWindow::SetViewMode(ViewMode Mode)
{
	WinRect Rc;
	GetClientRect(m_Wnd, &Rc);

	ListView_SetItemCountEx(m_List, 0, 0);

	LVCOLUMN Col;
	ZeroMemory(&Col, sizeof(Col));
	Col.mask = LVCF_TEXT | LVCF_WIDTH;

	while(ListView_DeleteColumn(m_List, 1))
		;

	delete [] m_JobMap;
	delete [] m_MachineMap;

	m_JobMap		= 0;
	m_MachineMap	= 0;

	int Idx(0);

	switch(m_Mode = Mode)
	{
		case VM_Jobs:
			Col.pszText	= "Job";
			Col.cx		= 60;
			ListView_SetColumn(m_List, Idx++, &Col);

			if (!m_IsManager)
			{
				Col.pszText	= "Comment";
				Col.cx		= Rc.Width() - (60 + 120 + 60);
				ListView_InsertColumn(m_List, Idx++, &Col);
			}
			else
			{
				Col.pszText	= "Command";
				Col.cx		= ((Rc.Width() - (60 + 120 + 60)) * 2) / 3;
				ListView_InsertColumn(m_List, Idx++, &Col);

				Col.pszText	= "Comment";
				Col.cx		= Rc.Width() - (60 + 120 + 60 + Col.cx);
				ListView_InsertColumn(m_List, Idx++, &Col);
			}

			Col.pszText	= "User";
			Col.cx		= 120;
			ListView_InsertColumn(m_List, Idx++, &Col);

			Col.pszText	= "State";
			Col.cx		= 60;
			ListView_InsertColumn(m_List, Idx++, &Col);
		break;

		case VM_Machines:
			Col.pszText	= "Machine";
			Col.cx		= 120;
			ListView_SetColumn(m_List, Idx++, &Col);

			Col.pszText	= "Job";
			Col.cx		= 60;
			ListView_InsertColumn(m_List, Idx++, &Col);

			if (!m_IsManager)
			{
				Col.pszText	= "Comment";
				Col.cx		= Rc.Width() - (120 + 60 + 120 + 60);
				ListView_InsertColumn(m_List, Idx++, &Col);
			}
			else
			{
				Col.pszText	= "Command";
				Col.cx		= ((Rc.Width() - (60 + 120 + 60)) * 2) / 3;
				ListView_InsertColumn(m_List, Idx++, &Col);

				Col.pszText	= "Comment";
				Col.cx		= Rc.Width() - (120 + 60 + 120 + 60 + Col.cx);
				ListView_InsertColumn(m_List, Idx++, &Col);
			}

			Col.pszText	= "User";
			Col.cx		= 120;
			ListView_InsertColumn(m_List, Idx++, &Col);

			Col.pszText	= "State";
			Col.cx		= 60;
			ListView_InsertColumn(m_List, Idx++, &Col);
		break;
	}

	SetListItems(false);
}

void FarmWindow::SetListItems(bool UpdateValues)
{
	switch(m_Mode)
	{
		case VM_Jobs:
		{
			int NoSelected = ListView_GetSelectedCount(m_List);
			int* Selected = new int[NoSelected];
			int Focus(ListView_GetNextItem(m_List, -1, LVNI_FOCUSED));

			if ((Focus >= 0) && (Focus <= m_Jobs.GetSize()))
			{
				ListView_SetItemState(m_List, Focus, 0, LVIS_FOCUSED);
				Focus = m_JobMap[Focus]->JobNo;
			}

			if (m_JobMap && Selected)
			{
				for(int i = 0, First = -1; i < NoSelected; i++)
				{
					if ((First = ListView_GetNextItem(m_List, First, LVNI_SELECTED)) == -1)
					{
						NoSelected = i;
						break;
					}

					Selected[i] = m_JobMap[First]->JobNo;

					ListView_SetItemState(m_List, First, 0, LVIS_SELECTED);
				}
			}
			else
				NoSelected = 0;

			if (UpdateValues)
			{
				m_Jobs		= m_NewJobs;
				m_Machines	= m_NewMachines;
			}

			if (m_JobMap)
				delete [] m_JobMap;

			if ((m_JobMap = new UiJob*[m_Jobs.GetSize()]) != 0)
			{
				for(int i = 0; i < m_Jobs.GetSize(); i++)
					m_JobMap[i] = m_Jobs.Get(i);

				qsort(m_JobMap, m_Jobs.GetSize(), sizeof(UiJob*), JobSortProc);
			}

			ListView_SetItemCountEx(m_List, m_Jobs.GetSize(), LVSICF_NOSCROLL);

			for(int i = 0, k = 0; i < NoSelected; i++)
			{
				for(int j = 0; j < m_Jobs.GetSize(); j++, k = (k + 1) % m_Jobs.GetSize())
				{
					if (m_JobMap[k]->JobNo == Selected[i])
					{
						ListView_SetItemState(m_List, k, LVIS_SELECTED, LVIS_SELECTED);
						break;
					}
				}
			}

			if (Focus >= 0)
			{
				for(int i = 0; i < m_Jobs.GetSize(); i++)
				{
					if (m_JobMap[i]->JobNo == Focus)
					{
						ListView_SetItemState(m_List, i, LVIS_FOCUSED, LVIS_FOCUSED);
						break;
					}
				}
			}

			delete [] Selected;
		}
		break;

		case VM_Machines:
		{
			int NoSelected = ListView_GetSelectedCount(m_List);
			int* Selected = new int[NoSelected];
			int Focus(ListView_GetNextItem(m_List, -1, LVNI_FOCUSED));

			if ((Focus >= 0) && (Focus <= m_Machines.GetSize()))
			{
				ListView_SetItemState(m_List, Focus, 0, LVIS_FOCUSED);
				Focus = m_MachineMap[Focus]->ProcessNo;
			}

			if (m_MachineMap && Selected)
			{
				for(int i = 0, First = -1; i < NoSelected; i++)
				{
					if ((First = ListView_GetNextItem(m_List, First, LVNI_SELECTED)) == -1)
					{
						NoSelected = i;
						break;
					}

					Selected[i] = m_MachineMap[First]->ProcessNo;

					ListView_SetItemState(m_List, First, 0, LVIS_SELECTED);
				}
			}
			else
				NoSelected = 0;

			if (UpdateValues)
			{
				m_Jobs		= m_NewJobs;
				m_Machines	= m_NewMachines;
			}

			if (m_MachineMap)
				delete [] m_MachineMap;

			if ((m_MachineMap = new UiMachine*[m_Machines.GetSize()]) != 0)
			{
				for(int i = 0; i < m_Machines.GetSize(); i++)
					m_MachineMap[i] = m_Machines.Get(i);

				qsort(m_MachineMap, m_Machines.GetSize(), sizeof(UiMachine*), MachineSortProc);
			}

			ListView_SetItemCountEx(m_List, m_Machines.GetSize(), LVSICF_NOSCROLL);

			for(int i = 0, k = 0; i < NoSelected; i++)
			{
				for(int j = 0; j < m_Machines.GetSize(); j++, k = (k + 1) % m_Machines.GetSize())
				{
					if (m_MachineMap[k]->ProcessNo == Selected[i])
					{
						ListView_SetItemState(m_List, k, LVIS_SELECTED, LVIS_SELECTED);
						break;
					}
				}
			}

			if (Focus >= 0)
			{
				for(int i = 0; i < m_Machines.GetSize(); i++)
				{
					if (m_MachineMap[i]->ProcessNo == Focus)
					{
						ListView_SetItemState(m_List, i, LVIS_FOCUSED, LVIS_FOCUSED);
						break;
					}
				}
			}

			delete [] Selected;
		}
		break;
	}

	for(int i = 0; i < m_Machines.GetSize(); i++)
		m_Machines.Get(i)->JobIdx = -1;

	for(int i = 0; i < m_Jobs.GetSize(); i++)
	{
		UiJob& Jb(*m_Jobs.Get(i));

		for(int j = 0; j < m_Machines.GetSize(); j++)
		{
			UiMachine& Mch(*m_Machines.Get(j));

			if (Jb.ProcessNo == Mch.ProcessNo)
				Mch.JobIdx = i;
		}
	}
}

UiCommand* FarmWindow::GetCommand()
{
	EnterCriticalSection(&m_Critical);

	UiCommand* Cmd(m_Commands.Detach(0));

	LeaveCriticalSection(&m_Critical);

	return Cmd;
}

bool FarmWindow::AddCommand(UiCommand* Cmd)
{
	EnterCriticalSection(&m_Critical);

	if (Cmd && ((Cmd->Type == UiCT_Version) || (Cmd->Type == UiCT_ListJobs) || (Cmd->Type == UiCT_ListMachines)))
	{
		for(int i = 0; i < m_Commands.GetSize(); i++)
		{
			UiCommand& ThisCmd(*m_Commands.Get(i));

			if (ThisCmd.Type == Cmd->Type)
				delete m_Commands.Detach(i--);
		}
	}

	bool Result(m_Commands.Add(Cmd) != 0);

	LeaveCriticalSection(&m_Critical);

	return Result;
}

int FarmWindow::JobSortProc(const void* A, const void* B)
{
	UiJob* Lhs(*(UiJob**)A);
	UiJob* Rhs(*(UiJob**)B);

	if (int Result = Lhs->State - Rhs->State)
		return Result;

	return Lhs->Idx - Rhs->Idx;
}

int FarmWindow::MachineSortProc(const void* A, const void* B)
{
	UiMachine* Lhs(*(UiMachine**)A);
	UiMachine* Rhs(*(UiMachine**)B);

	return Lhs->ProcessNo - Rhs->ProcessNo;
}

int FarmWindow::NewJobDlgProc(HWND Dlg, int Msg, WPARAM wParam, LPARAM lParam)
{
	switch(Msg)
	{
		case WM_INITDIALOG:
		{
			SetWindowLong(Dlg, DWL_USER, lParam);

			if (((UiNewJob*)lParam)->Editing)
				SetWindowText(Dlg, "Edit Job...");
			else
				SetWindowText(Dlg, "New Job...");

			SetDlgItemText(Dlg, IDC_COMMAND, ((UiNewJob*)lParam)->Command);
			SetDlgItemText(Dlg, IDC_COMMENT, ((UiNewJob*)lParam)->Comment);
		}
		return TRUE;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDOK:
				{
					if (HIWORD(wParam) == BN_CLICKED)
					{
						UiNewJob* Job((UiNewJob*)GetWindowLong(Dlg, DWL_USER));

						if (Job)
						{
							char Buffer[4096];

							GetDlgItemText(Dlg, IDC_COMMAND, Buffer, sizeof(Buffer));
							Job->Command = Buffer;

							GetDlgItemText(Dlg, IDC_COMMENT, Buffer, sizeof(Buffer));
							Job->Comment = Buffer;
						}

						EndDialog(Dlg, 1);
					}
				}
				break;

				case IDCANCEL:
				{
					if (HIWORD(wParam) == BN_CLICKED)
						EndDialog(Dlg, 0);
				}
				break;
			}
		break;
	}

	return FALSE;
}

DWORD CALLBACK FarmWindow::ThreadProcessProc(void* Parameter)
{
	((FarmWindow*)Parameter)->Process();

	return 0;
}
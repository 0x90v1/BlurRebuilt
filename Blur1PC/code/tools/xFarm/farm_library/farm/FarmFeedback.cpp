#include "StdAfx.h"
#include "Farm.h"

void FeedbackInfo::SetLogFilename(std::string sLogFilename)
{
	const char *p = sLogFilename.c_str();
	const char *p2 = strrchr(p, '\\');
	if (!p2)
	{
		p2 = strrchr(p, '/');
		if (!p2)
			return;
	}

	m_LogFileHTMLName = p2+1;
}

void FeedbackInfo::ConvertQuotes(Util::C_String &s)
{
	s = s.Replace("&", "&amp;");
	s = s.Replace("\"", "&quot;");
	s = s.Replace("'", "&apos;");
	s = s.Replace("<", "&lt;");
	s = s.Replace(">", "&gt;");
}

void FeedbackInfo::Register(const char *psz)
{
	String sCmd = SnapAtom(psz);
	int iType = SnapInt(psz);

	// convert " to '
	if (!stricmp(sCmd,"SET"))
		Set(iType, psz);
	else if (!stricmp(sCmd,"APPEND"))
		Append(iType, psz);
	else
	{
		// unknown command; ignore
	}
}

void FeedbackInfo::Set(int i, const char *psz)
{
	if (m_Data.Size() < i+1) m_Data.Resize(i+1);
	if (strlen(psz) > 4096)
		return;

	m_Data[i] = psz;
	ConvertQuotes(m_Data[i]);
}

void FeedbackInfo::Append(int i, const char *psz)
{
	if (m_Data.Size() < i+1) m_Data.Resize(i+1);

	Util::C_String s(psz);
	ConvertQuotes(s);

	if (m_Data[i].Length() + s.Length() > 4096)
		return;

	m_Data[i] = m_Data[i] + s;
	if (i == FEEDBACK_INFO_FEEDBACK)
		m_Data[i] = m_Data[i] + "\n";
}

bool g_bOutsourcingMode = false;

Util::C_String FeedbackInfo::GetExecuterString()
{
	Util::C_String out, tmp;

	tmp.Printf("<values>\n");
	out = out + tmp;

	// header
	if (m_Data.Size() > FEEDBACK_INFO_TITLE)
	{
		tmp.Printf("<container name='title' display_name='%s' type='basic'>\n", m_Data[FEEDBACK_INFO_TITLE].Chars());
		out = out + tmp;
	}
	else
	{
		out = out + "<container name='title' type='basic'>\n";
	}

	// progress bars
	int nProgressBars = (m_Data.Size()-3)/2;
	for (int i=0; i<nProgressBars; i++)
	{
		tmp.Printf("<container name='progress_%d' display_name='%s' value = '%s' type='progress_bar' pre_format ='cr;'/> \n",
			i, m_Data[i*2+FEEDBACK_INFO_PROGRESS_TEXT_1].Chars(), m_Data[i*2+FEEDBACK_INFO_PROGRESS_VALUE_1].Chars());
		out = out + tmp;
	}

	// feedback text
	if (m_Data.Size()>FEEDBACK_INFO_FEEDBACK && m_Data[FEEDBACK_INFO_FEEDBACK] != "")
	{
		out = out + "<container name='feedback_txt' display_name='' readonly='true' type='text' value='";
		out = out + m_Data[FEEDBACK_INFO_FEEDBACK].Chars();
		out = out + "' pre_format ='cr;'/>\n";
	}

	// click to view log
	if (m_LogFileHTMLName != "")
	{
		if (!g_bOutsourcingMode)
		{
			out = out + "<container name='log_button' type='action_button' display_name='View Log...' pre_format ='cr;' "
				"command='%WINDIR%\\explorer.exe \"http:\\\\farmserver\\logframes?file=logs\\" + m_LogFileHTMLName + "\"' />";
		}
		else
		{
			out = out + "<container name='log_button' type='action_button' display_name='View Log...' pre_format ='cr;' "
				"command='%WINDIR%\\notepad.exe \"c:\\pgr4\\logs\\" + m_LogFileHTMLName + "\"' />";
		}
	}

	out = out + "</container>\n";

	out = out + "</values>\n";

	return out;
}


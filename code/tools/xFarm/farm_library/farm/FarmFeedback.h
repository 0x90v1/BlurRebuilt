#ifndef __FARM_FEEDBACK_H__
#define __FARM_FEEDBACK_H__

#define FEEDBACK_INFO_TITLE 0
#define FEEDBACK_INFO_FEEDBACK 1
#define FEEDBACK_INFO_XML 2
#define FEEDBACK_INFO_PROGRESS_TEXT_1 3
#define FEEDBACK_INFO_PROGRESS_VALUE_1 4

// feedback info to pass onto executer in the form of XML messages
class FeedbackInfo
{
public:
	void SetLogFilename(std::string sLogFilename);
	void ConvertQuotes(Util::C_String &s);
	void Register(const char *psz);
	void Set(int i, const char *psz);
	void Append(int i, const char *psz);
	Util::C_String GetExecuterString();

	Ut::Vector<Util::C_String>	m_Data;
	Util::C_String m_LogFileHTMLName;
};

#endif

#ifndef __SPAWNPROCESS_H__
#define __SPAWNPROCESS_H__

// class that allows us to spawn a process and check when it finishes
class SpawnProcess : public MT::Thread
{
public:
	SpawnProcess(const char *pszCommandLine);
	void	ThreadMain();
	int		GetProcessHandle();
	int		GetExitCode();
	void	Terminate();
	bool	IsActive();
	int		GetProcessID();

private:
	char	m_pszCommandLine[4096];
	int		m_iSpawnProcess;
	int		m_iExitCode;
	int		m_iPID;
	bool	m_bActive;
};


#endif

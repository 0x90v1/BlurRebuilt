#ifndef __FARMUSERINTERFACE_H__
#define __FARMUSERINTERFACE_H__

// This is an instance of a manager
class FarmUserInterface : public Farmer
{
protected:
	bool m_isManager;

public:
	FarmUserInterface(bool isManager, U16 broadcastPort, U16 networkPort);
	~FarmUserInterface();

	bool Start(void);
};

#endif

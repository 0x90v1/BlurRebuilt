#ifndef BIZBRAIN_ALIENBRAIN_H
#define BIZBRAIN_ALIENBRAIN_H

#pragma once

class C_AlienBrainWrapper
{
public:
	C_AlienBrainWrapper();
   ~C_AlienBrainWrapper();

	bool Initialise(
	   const Btl::String& username, 
	   const Btl::String& password, 
	   const Btl::String& project,
	   const Btl::String& server);
	void Shutdown();

	bool GetLatest(const Btl::String& path, U32 nRetries = 0);
	bool SyncLatest(const Btl::String& name, U32 nRetries = 0, bool forceOnFailure = true);

	CNxNSmartIntegrator* Integrator() { return m_pIntegrator; }

private:
	void RecurseDeleteFilesNotInAb(CNxNString folderName);

	CNxNSmartIntegrator*	m_pIntegrator;
	CNxNProject*			m_pProject;
};

#endif

#include <StdAfx.h>
#include <Network\BandwidthEvaluator.h>

namespace NetPlay
{
	// constructor
	C_BandwidthEvaluator::C_BandwidthEvaluator(const U32 minUpstream, const U32 minDownstream) :m_UpStream(minUpstream), m_DownStream(minDownstream), 
																								m_MinUpStream(minUpstream), m_MinDownStream(minDownstream),
																								m_pQos(NULL)
	{
	}


	// destructor
	C_BandwidthEvaluator::~C_BandwidthEvaluator()
	{
		Stop();
	}

	bool C_BandwidthEvaluator::Start(void)
	{
		Stop();

		if (XNetQosServiceLookup(0, NULL, &m_pQos) != 0)
			return false;

		return true;
	}

	void C_BandwidthEvaluator::Update(void)
	{
		if (!IsActive())
			return;

		if (m_pQos->cxnqosPending > 0)
			return; // operation still in progress

		const DWORD flags = m_pQos->axnqosinfo[0].bFlags;
		if (!(flags&XNET_XNQOSINFO_TARGET_DISABLED) && (flags&XNET_XNQOSINFO_COMPLETE))
		{			
			m_UpStream		= m_pQos->axnqosinfo[0].dwUpBitsPerSec > m_MinUpStream ? m_pQos->axnqosinfo[0].dwUpBitsPerSec : m_MinUpStream;
			m_DownStream	= m_pQos->axnqosinfo[0].dwDnBitsPerSec > m_MinDownStream ? m_pQos->axnqosinfo[0].dwDnBitsPerSec : m_MinDownStream;
		}

		Stop();
	}

	bool C_BandwidthEvaluator::IsActive(void)
	{
		return NULL == m_pQos ? false : true;
	}

	void C_BandwidthEvaluator::Stop(void)
	{
		if (!IsActive())
			return;

		XNetQosRelease(m_pQos);
		m_pQos = NULL;
	}
}


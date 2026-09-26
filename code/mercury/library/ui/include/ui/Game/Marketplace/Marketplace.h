#ifndef MARKETPLACE_H
#define MARKETPLACE_H

#ifdef _MSC_VER
#pragma once
#endif

namespace UI
{

// -------------
// C_Marketplace
// -------------
class C_Marketplace
{
public:
	C_Marketplace()
		: m_bEnumerating( false )
		, m_bPurchasing( false )
	#ifdef PLATFORM_XENON
		, m_uTotalOffers( 0 )
		, m_pOfferInfo( NULL )
	#endif
	{
	#ifdef PLATFORM_XENON
		ZeroMemory( &m_AvailableResult, sizeof( m_AvailableResult ) );
	#endif
	}

public:
	static C_Marketplace&				Instance()						{ return m_sInstance; }

public:
	bool								Enumerating()					{ return m_bEnumerating; }
	bool								Purchasing()					{ return m_bPurchasing; }

#ifdef PLATFORM_XENON
	bool								StartEnumeration( const S32 sPadNo );
	bool								ContinueEnumeration();

	bool								StartPurchase( const S32 sPadNo, const U64 uIndex );
	bool								ContinuePurchase();

	XOFFERING_CONTENTAVAILABLE_RESULT&	GetAvailableResult()			{ return m_AvailableResult; }

	XMARKETPLACE_CONTENTOFFER_INFO*		GetOffer( const U64 uIndex );
	U64									GetNumOffers()					{ return m_uTotalOffers; }

	bool								Downloading();

	void								Cleanup();
#endif

private:
	static C_Marketplace				m_sInstance;

private:
	bool								m_bEnumerating;
	bool								m_bPurchasing;

#ifdef PLATFORM_XENON
	XOFFERING_CONTENTAVAILABLE_RESULT	m_AvailableResult;
	U64									m_uTotalOffers;
	XMARKETPLACE_CONTENTOFFER_INFO*		m_pOfferInfo;					// array
	XBACKGROUND_DOWNLOAD_MODE			m_PreviousDownloadMode;
#endif
};

}; //namespace UI

#endif /* MARKETPLACE_H */

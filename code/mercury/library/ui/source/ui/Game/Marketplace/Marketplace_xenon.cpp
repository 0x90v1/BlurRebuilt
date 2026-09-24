#include "stdafx.h"

#include "ui/Game/Marketplace/Marketplace.h"

#ifdef CFG_ALLOW_OPTIMIZE_OFF
#pragma optimize( "", off )
#endif

namespace UI
{

//
static S32							s_sPadNo = -1;
static HANDLE						s_Handle;
static XOVERLAPPED					s_Overlapped;
static bool							s_bInternalEnumerating = false;
static XBACKGROUND_DOWNLOAD_MODE	s_PreviousDownloadMode = XBACKGROUND_DOWNLOAD_MODE_AUTO;
static HRESULT						s_Result;
static ULONGLONG					s_PendingOfferID;
static Btl::Vector<ULONGLONG>		s_DownloadingOfferIDs;
//

bool C_Marketplace::StartEnumeration( const S32 sPadNo )
{
	if ( m_bEnumerating || m_bPurchasing )
		return false;

	m_bEnumerating = false;
	s_bInternalEnumerating = false;

	if ( m_pOfferInfo )
	{
		LOGERROR( g_Log, "C_Marketplace::StartEnumeration() - offer buffer hasn't been freed! - will do it anyway" );
		SAFE_DELETE_ARY( m_pOfferInfo );
		m_uTotalOffers = 0;
	}

	s_sPadNo = sPadNo;

	ZeroMemory( &m_AvailableResult, sizeof( m_AvailableResult ) );
	ZeroMemory( &s_Overlapped, sizeof( s_Overlapped ) );

	// get number of offers
	DWORD ret = XContentGetMarketplaceCounts( 
		s_sPadNo,
		0xffffffff,	// all categories - TODO interface to specify this?
		sizeof( m_AvailableResult ),
		&m_AvailableResult,
		&s_Overlapped
	);

	if ( ret == ERROR_IO_PENDING )
		m_bEnumerating = true;
	else
	{
		LOGERROR( g_Log, "XContentGetMarketplaceCounts() - returned error ( %d )", ret );
	}

	return m_bEnumerating;
}

bool C_Marketplace::ContinueEnumeration()
{
	if ( !m_bEnumerating )
		return true;

	if ( !s_bInternalEnumerating )
	{
		if ( XHasOverlappedIoCompleted( &s_Overlapped ) )
		{
			if ( m_AvailableResult.dwTotalOffers == 0 )
			{
				LOGINFO( g_Log, "C_Marketplace::ContinueEnumeration() - total offers == 0" );
				m_bEnumerating = false;
				return true;
			}

			// start the content enumeration
			DWORD bufferSize = 0;

			U64 uNumToEnumerate = m_AvailableResult.dwTotalOffers;

			DWORD ret = XMarketplaceCreateOfferEnumerator( 
				s_sPadNo,
				XMARKETPLACE_OFFERING_TYPE_CONTENT,		// TODO - interface for this?
				0,										// TODO - interface for this?
				(DWORD)uNumToEnumerate,
				&bufferSize,
				&s_Handle
			);

			if ( ret != ERROR_SUCCESS )
			{
				LOGERROR( g_Log, "XMarketplaceCreateOfferEnumerator() - returned error ( %d )", ret );
				m_bEnumerating = false;
				return true;
			}

			// alloc buffer
			U8* pBuffer = new U8[ bufferSize ];

			if ( !pBuffer )
			{
				LOGERROR( g_Log, "C_Marketplace::ContinueEnumeration() - can't alloc offer buffer!" );
				m_bEnumerating = false;
				return true;
			}

			m_pOfferInfo = (XMARKETPLACE_CONTENTOFFER_INFO*)pBuffer;
			ZeroMemory( m_pOfferInfo, bufferSize );

			ZeroMemory( &s_Overlapped, sizeof( s_Overlapped ) );

			ret = XEnumerate( s_Handle, m_pOfferInfo, bufferSize, 0, &s_Overlapped );

			if ( ret != ERROR_IO_PENDING )
			{
				LOGERROR( g_Log, "C_Marketplace::ContinueEnumeration() - XEnumerate() - returned error ( %d )", ret );
				m_bEnumerating = false;
				return true;
			}

			s_bInternalEnumerating = true;
		}
	}
	else
	{
		if ( XHasOverlappedIoCompleted( &s_Overlapped ) )
		{
			m_bEnumerating = false;
			s_bInternalEnumerating = false;

			m_uTotalOffers = s_Overlapped.InternalHigh;

			CloseHandle( s_Handle );
		}
	}
	
	return !m_bEnumerating;
}

bool C_Marketplace::StartPurchase( const S32 sPadNo, const U64 uIndex )
{
	if ( m_bEnumerating || m_bPurchasing )
		return false;

	XMARKETPLACE_CONTENTOFFER_INFO* pOffer = GetOffer( uIndex );

	if ( !pOffer )
		return false;

	ZeroMemory( &s_Overlapped, sizeof( s_Overlapped ) );

	DWORD ret = XShowMarketplaceDownloadItemsUI( 
		sPadNo,
		XSHOWMARKETPLACEDOWNLOADITEMS_ENTRYPOINT_PAIDITEMS,	// TODO - interface for this?
		&pOffer->qwOfferID,	// TODO - interface for multi-purchase? - XMARKETPLACE_MAX_OFFERIDS
		1,
		&s_Result,
		&s_Overlapped
	);

	if ( ret != ERROR_IO_PENDING )
	{
		LOGERROR( g_Log, "C_Marketplace::StartPurchase() - XShowMarketplaceDownloadItemsUI() - returned error ( %d )", ret );
		return false;
	}
	else
		s_PendingOfferID = pOffer->qwOfferID;

	m_bPurchasing = true;

	return true;
}

bool C_Marketplace::ContinuePurchase()
{
	if ( !m_bPurchasing )
		return true;

	if ( XHasOverlappedIoCompleted( &s_Overlapped ) )
	{
		m_bPurchasing = false;

		switch ( s_Result )
		{
			case S_OK:
				// add to download check list
				s_DownloadingOfferIDs.PushBack( s_PendingOfferID );
				break;

			case MPDI_E_CANCELLED:			LOGINFO(  g_Log, "C_Marketplace::ContinuePurchase() - user cancelled" );	break;
			case MPDI_E_INVALIDARG:			LOGERROR( g_Log, "C_Marketplace::ContinuePurchase() - invalid arg" );		break;
			case MPDI_E_OPERATION_FAILED:	LOGERROR( g_Log, "C_Marketplace::ContinuePurchase() - operation failed" );	break;

			default:																									break;
		}

		return true;
	}

	return false;
}

bool C_Marketplace::Downloading()
{
	bool bDownloading = false;

	for ( Btl::Vector<ULONGLONG>::Iterator it = s_DownloadingOfferIDs.Begin(); it != s_DownloadingOfferIDs.End(); )
	{
		DWORD result;
		DWORD ret = XMarketplaceGetDownloadStatus( s_sPadNo, *it, &result );

		if ( ret == ERROR_SUCCESS )
		{
			if ( result == ERROR_IO_PENDING )
			{
				bDownloading = true;
				break;
			}
			else if ( result == ERROR_SUCCESS  )
			{
				// download has finished
				it = s_DownloadingOfferIDs.Erase( it );
			}
			else
			{
				++it;
			}
		}
		else
		{
			LOGERROR( g_Log, "XMarketplaceGetDownloadStatus() - returned error ( %d )", ret );
			++it;
		}
	}

	return bDownloading;
}

XMARKETPLACE_CONTENTOFFER_INFO* C_Marketplace::GetOffer( const U64 uIndex )
{
	if ( !m_pOfferInfo || ( uIndex < 0 ) || ( uIndex >= m_uTotalOffers ) )
		return NULL;

	return &m_pOfferInfo[ uIndex ];
}

void C_Marketplace::Cleanup()
{
	m_uTotalOffers = 0;
	SAFE_DELETE_ARY( m_pOfferInfo );
}

// TODO - do we need this? - GH:WT seemed to screw it up!
/*
XBACKGROUND_DOWNLOAD_MODE currentMode = XBackgroundDownloadGetMode();

if ( currentMode != XBACKGROUND_DOWNLOAD_MODE_ALWAYS_ALLOW )
	XBackgroundDownloadSetMode( XBACKGROUND_DOWNLOAD_MODE_ALWAYS_ALLOW );

// set this back when we're finished
XBackgroundDownloadSetMode( currentMode );
*/

}; // namespace UI

#ifdef CFG_ALLOW_OPTIMIZE_OFF
#pragma optimize( "", on )
#endif

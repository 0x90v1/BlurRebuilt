#include "Stdafx.h"

#include "ui/Game/Marketplace/MarketplaceDataFinder.h"
#include "ui/Game/Marketplace/Marketplace.h"

#include "ui/Game/UILocaliser.h"

#ifdef CFG_ALLOW_OPTIMIZE_OFF
#pragma optimize( "", off )
#endif

namespace UI
{

// global instance
C_MarketplaceDataFinder C_MarketplaceDataFinder::m_sInstance;

// -----------------------
// C_MarketplaceDataFinder
// -----------------------
void C_MarketplaceDataFinder::Init()
{
	//
}

const WCHAR* C_MarketplaceDataFinder::GetVariableText( const char* pVariableName )
{
	m_VariableText.Clear();

	if ( !strcmp( pVariableName, "TOTAL_OFFERS" ) )
	{
	#ifdef PLATFORM_XENON
		m_VariableText.Format( C_UILocaliser::Instance().ConvertNumberToCommas( C_Marketplace::Instance().GetAvailableResult().dwTotalOffers ) );
	#endif
	}
	else if ( !strcmp( pVariableName, "NEW_OFFERS" ) )
	{
	#ifdef PLATFORM_XENON
		m_VariableText.Format( C_UILocaliser::Instance().ConvertNumberToCommas( C_Marketplace::Instance().GetAvailableResult().dwNewOffers ) );
	#endif
	}
	else if ( !strncmp( pVariableName, "OFFER_", 6 ) )
	{
		S64 sIndex = ParseIndexAdvance( pVariableName, "OFFER_" );

		if ( sIndex == -1 )
			return L"";

	#ifdef PLATFORM_XENON
		XMARKETPLACE_CONTENTOFFER_INFO* pOffer = C_Marketplace::Instance().GetOffer( (U64)sIndex );

		if ( !pOffer )
			return L"";

		if ( !strcmp( pVariableName, "_NAME" ) )
		{
			m_VariableText.Format( pOffer->wszOfferName );
		}
		else if ( !strcmp( pVariableName, "_TITLE" ) )
		{
			m_VariableText.Format( pOffer->wszTitleName );
		}
		else if ( !strcmp( pVariableName, "_SELL" ) )
		{
			m_VariableText.Format( pOffer->wszSellText );
		}
		else if ( !strcmp( pVariableName, "_COST" ) )
		{
			m_VariableText.Format( L"%s ~~{points}~~", C_UILocaliser::Instance().ConvertNumberToCommas( (S64)pOffer->dwPointsPrice ) );
		}
	#endif
	}
	else
	{
		// unhandled
		LOGWARN( g_Log, DBSTR "name \"%s\" not found", pVariableName );
		return L"";
	}

	return m_VariableText;	
}

}; // namespace UI

#ifdef CFG_ALLOW_OPTIMIZE_OFF
#pragma optimize( "", on )
#endif

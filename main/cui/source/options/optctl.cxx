/**************************************************************
 * 
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 * 
 *************************************************************/



// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_cui.hxx"
#include "optctl.hxx"
#include <dialmgr.hxx>
#include "optctl.hrc"
#include <cuires.hrc>
#include <svl/ctloptions.hxx>

// class SvxCTLOptionsPage -----------------------------------------------------

IMPL_LINK( SvxCTLOptionsPage, SequenceCheckingCB_Hdl, void*, EMPTYARG )
{
    sal_Bool bIsSequenceChecking = m_aSequenceCheckingCB.IsChecked();
    m_aRestrictedCB.Enable( bIsSequenceChecking );
    m_aTypeReplaceCB.Enable( bIsSequenceChecking );
    // #i48117#: by default restricted and type&replace have to be switched on
    if(bIsSequenceChecking)
    {        
        m_aTypeReplaceCB.Check( sal_True );
        m_aRestrictedCB.Check( sal_True );
    }
    return 0;
}

SvxCTLOptionsPage::SvxCTLOptionsPage( Window* pParent, const SfxItemSet& rSet ) :

	SfxTabPage( pParent, CUI_RES( RID_SVXPAGE_OPTIONS_CTL ), rSet ),

	m_aSequenceCheckingFL	( this, CUI_RES( FL_SEQUENCECHECKING ) ),
	m_aSequenceCheckingCB	( this, CUI_RES( CB_SEQUENCECHECKING ) ),
	m_aRestrictedCB			( this, CUI_RES( CB_RESTRICTED ) ),
    m_aTypeReplaceCB        ( this, CUI_RES( CB_TYPE_REPLACE ) ),
	m_aCursorControlFL		( this, CUI_RES( FL_CURSORCONTROL ) ),
	m_aMovementFT			( this, CUI_RES( FT_MOVEMENT ) ),
	m_aMovementLogicalRB	( this, CUI_RES( RB_MOVEMENT_LOGICAL ) ),
	m_aMovementVisualRB		( this, CUI_RES( RB_MOVEMENT_VISUAL ) ),
	m_aGeneralFL			( this, CUI_RES( FL_GENERAL ) ),
	m_aNumeralsFT			( this, CUI_RES( FT_NUMERALS ) ),
	m_aNumeralsLB			( this, CUI_RES( LB_NUMERALS ) )

{
	FreeResource();

	m_aSequenceCheckingCB.SetClickHdl( LINK( this, SvxCTLOptionsPage, SequenceCheckingCB_Hdl ) );

	m_aNumeralsLB.SetDropDownLineCount( m_aNumeralsLB.GetEntryCount() );
}
// -----------------------------------------------------------------------------
SvxCTLOptionsPage::~SvxCTLOptionsPage()
{
}
// -----------------------------------------------------------------------------
SfxTabPage*	SvxCTLOptionsPage::Create( Window* pParent, const SfxItemSet& rAttrSet )
{
	return new SvxCTLOptionsPage( pParent, rAttrSet );
}
// -----------------------------------------------------------------------------
sal_Bool SvxCTLOptionsPage::FillItemSet( SfxItemSet& )
{
	sal_Bool bModified = sal_False;
	SvtCTLOptions aCTLOptions;

	// Sequence checking
	sal_Bool bChecked = m_aSequenceCheckingCB.IsChecked();
	if ( bChecked != m_aSequenceCheckingCB.GetSavedValue() )
    {
        aCTLOptions.SetCTLSequenceChecking( bChecked );
        bModified = sal_True;
    }

	bChecked = m_aRestrictedCB.IsChecked();
	if( bChecked != m_aRestrictedCB.GetSavedValue() )
    {
        aCTLOptions.SetCTLSequenceCheckingRestricted( bChecked );
        bModified = sal_True;
    }
    bChecked = m_aTypeReplaceCB.IsChecked();
    if( bChecked != m_aTypeReplaceCB.GetSavedValue())
    {
        aCTLOptions.SetCTLSequenceCheckingTypeAndReplace(bChecked);
        bModified = sal_True;
    }

	sal_Bool bLogicalChecked = m_aMovementLogicalRB.IsChecked();
	sal_Bool bVisualChecked = m_aMovementVisualRB.IsChecked();
	if ( bLogicalChecked != m_aMovementLogicalRB.GetSavedValue() ||
		 bVisualChecked != m_aMovementVisualRB.GetSavedValue() )
    {
		SvtCTLOptions::CursorMovement eMovement =
			bLogicalChecked ? SvtCTLOptions::MOVEMENT_LOGICAL : SvtCTLOptions::MOVEMENT_VISUAL;
        aCTLOptions.SetCTLCursorMovement( eMovement );
        bModified = sal_True;
	}

	sal_uInt16 nPos = m_aNumeralsLB.GetSelectEntryPos();
	if ( nPos != m_aNumeralsLB.GetSavedValue() )
	{
        aCTLOptions.SetCTLTextNumerals( (SvtCTLOptions::TextNumerals)nPos );
        bModified = sal_True;
	}

	return bModified;
}
// -----------------------------------------------------------------------------
void SvxCTLOptionsPage::Reset( const SfxItemSet& )
{
	SvtCTLOptions aCTLOptions;

	m_aSequenceCheckingCB.Check( aCTLOptions.IsCTLSequenceChecking() );
	m_aRestrictedCB.Check( aCTLOptions.IsCTLSequenceCheckingRestricted() );
    m_aTypeReplaceCB.Check( aCTLOptions.IsCTLSequenceCheckingTypeAndReplace() );

	SvtCTLOptions::CursorMovement eMovement = aCTLOptions.GetCTLCursorMovement();
	switch ( eMovement )
	{
		case SvtCTLOptions::MOVEMENT_LOGICAL :
			m_aMovementLogicalRB.Check();
			break;

		case SvtCTLOptions::MOVEMENT_VISUAL :
			m_aMovementVisualRB.Check();
			break;

		default:
			DBG_ERRORFILE( "SvxCTLOptionsPage::Reset(): invalid movement enum" );
	}

	sal_uInt16 nPos = (sal_uInt16)aCTLOptions.GetCTLTextNumerals();
	DBG_ASSERT( nPos < m_aNumeralsLB.GetEntryCount(), "SvxCTLOptionsPage::Reset(): invalid numerals enum" );
	m_aNumeralsLB.SelectEntryPos( nPos );

	m_aSequenceCheckingCB.SaveValue();
	m_aRestrictedCB.SaveValue();
    m_aTypeReplaceCB.SaveValue();
	m_aMovementLogicalRB.SaveValue();
	m_aMovementVisualRB.SaveValue();
	m_aNumeralsLB.SaveValue();

    sal_Bool bIsSequenceChecking = m_aSequenceCheckingCB.IsChecked();
    m_aRestrictedCB.Enable( bIsSequenceChecking );
    m_aTypeReplaceCB.Enable( bIsSequenceChecking );
}


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
#include "precompiled_framework.hxx"

//-----------------------------------------------
// my own includes

#include <algorithm>
#include <helper/statusindicatorfactory.hxx>
#include <helper/statusindicator.hxx>
#include <helper/vclstatusindicator.hxx>
#include <threadhelp/writeguard.hxx>
#include <threadhelp/readguard.hxx>
#include <services.h>
#include <properties.h>

//-----------------------------------------------
// interface includes
#include <com/sun/star/awt/Rectangle.hpp>

#ifndef _COM_SUN_STAR_AWT_XCONTROLS_HPP_
#include <com/sun/star/awt/XControl.hpp>
#endif
#include <com/sun/star/awt/XLayoutConstrains.hpp>
#include <com/sun/star/awt/DeviceInfo.hpp>
#include <com/sun/star/awt/PosSize.hpp>
#include <com/sun/star/awt/WindowAttribute.hpp>
#include <com/sun/star/awt/XTopWindow.hpp>
#include <com/sun/star/awt/XWindow2.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/frame/XLayoutManager.hpp>

#ifndef _TOOLKIT_HELPER_VCLUNOHELPER_HXX_
#include <toolkit/unohlp.hxx>
#endif

//-----------------------------------------------
// includes of other projects
#include <comphelper/sequenceashashmap.hxx>
#include <comphelper/mediadescriptor.hxx>
#include <comphelper/configurationhelper.hxx>
#include <vcl/svapp.hxx>
#include <vos/mutex.hxx>

//-----------------------------------------------
// namespace

namespace framework{

//-----------------------------------------------
// definitions

sal_Int32 StatusIndicatorFactory::m_nInReschedule = 0;	///	static counter for rescheduling
static ::rtl::OUString PROGRESS_RESOURCE = ::rtl::OUString::createFromAscii("private:resource/progressbar/progressbar");

//-----------------------------------------------
DEFINE_XINTERFACE_5(StatusIndicatorFactory                              ,
                    OWeakObject                                         ,
                    DIRECT_INTERFACE(css::lang::XTypeProvider          ),
                    DIRECT_INTERFACE(css::lang::XServiceInfo           ),
                    DIRECT_INTERFACE(css::lang::XInitialization        ),
                    DIRECT_INTERFACE(css::task::XStatusIndicatorFactory),
                    DIRECT_INTERFACE(css::util::XUpdatable             ))

DEFINE_XTYPEPROVIDER_5(StatusIndicatorFactory            ,
                       css::lang::XTypeProvider          ,
                       css::lang::XServiceInfo           ,
                       css::lang::XInitialization        ,
                       css::task::XStatusIndicatorFactory,
                       css::util::XUpdatable             )

DEFINE_XSERVICEINFO_MULTISERVICE(StatusIndicatorFactory                   ,
                                 ::cppu::OWeakObject                      ,
                                 SERVICENAME_STATUSINDICATORFACTORY       ,
                                 IMPLEMENTATIONNAME_STATUSINDICATORFACTORY)

DEFINE_INIT_SERVICE(StatusIndicatorFactory,
                    {
                        /*Attention
                            I think we don't need any mutex or lock here ... because we are called by our own static method impl_createInstance()
                            to create a new instance of this class by our own supported service factory.
                            see macro DEFINE_XSERVICEINFO_MULTISERVICE and "impl_initService()" for further informations!
                        */
                    }
                   )

//-----------------------------------------------
StatusIndicatorFactory::StatusIndicatorFactory(const css::uno::Reference< css::lang::XMultiServiceFactory >& xSMGR)
    : ThreadHelpBase      (         )
    , ::cppu::OWeakObject (         )
    , m_xSMGR             (xSMGR    )
    , m_pWakeUp           (0        )
    , m_bAllowReschedule  (sal_False)
    , m_bAllowParentShow  (sal_False)
    , m_bDisableReschedule(sal_False)
{
}

//-----------------------------------------------
StatusIndicatorFactory::~StatusIndicatorFactory()
{
    impl_stopWakeUpThread();
}

//-----------------------------------------------
void SAL_CALL StatusIndicatorFactory::initialize(const css::uno::Sequence< css::uno::Any >& lArguments)
    throw(css::uno::Exception       ,
          css::uno::RuntimeException)
{
    ::comphelper::SequenceAsHashMap lArgs(lArguments);

    // SAFE -> ----------------------------------
    WriteGuard aWriteLock(m_aLock);

    m_xFrame             = lArgs.getUnpackedValueOrDefault(STATUSINDICATORFACTORY_PROPNAME_FRAME            , css::uno::Reference< css::frame::XFrame >());
    m_xPluggWindow       = lArgs.getUnpackedValueOrDefault(STATUSINDICATORFACTORY_PROPNAME_WINDOW           , css::uno::Reference< css::awt::XWindow >() );
    m_bAllowParentShow   = lArgs.getUnpackedValueOrDefault(STATUSINDICATORFACTORY_PROPNAME_ALLOWPARENTSHOW  , (sal_Bool)sal_False                        );
    m_bDisableReschedule = lArgs.getUnpackedValueOrDefault(STATUSINDICATORFACTORY_PROPNAME_DISABLERESCHEDULE, (sal_Bool)sal_False                        );

    aWriteLock.unlock();
    // <- SAFE ----------------------------------

    impl_createProgress();
}

//-----------------------------------------------
css::uno::Reference< css::task::XStatusIndicator > SAL_CALL StatusIndicatorFactory::createStatusIndicator()
    throw(css::uno::RuntimeException)
{
    StatusIndicator* pIndicator = new StatusIndicator(this);
    css::uno::Reference< css::task::XStatusIndicator > xIndicator(static_cast< ::cppu::OWeakObject* >(pIndicator), css::uno::UNO_QUERY_THROW);

	return xIndicator;
}

//-----------------------------------------------
void SAL_CALL StatusIndicatorFactory::update()
    throw(css::uno::RuntimeException)
{
    // SAFE -> ----------------------------------
    WriteGuard aWriteLock(m_aLock);
    m_bAllowReschedule = sal_True;
    aWriteLock.unlock();
    // <- SAFE ----------------------------------
}

//-----------------------------------------------
void StatusIndicatorFactory::start(const css::uno::Reference< css::task::XStatusIndicator >& xChild,
                                   const ::rtl::OUString&                                    sText ,
                                         sal_Int32                                           nRange)
{
    // SAFE -> ----------------------------------
    WriteGuard aWriteLock(m_aLock);

    // create new info structure for this child or move it to the front of our stack
    IndicatorStack::iterator pItem = ::std::find(m_aStack.begin(), m_aStack.end(), xChild);
    if (pItem != m_aStack.end())
        m_aStack.erase(pItem);
    IndicatorInfo aInfo(xChild, sText, nRange);
    m_aStack.push_back (aInfo                );

    m_xActiveChild = xChild;
    css::uno::Reference< css::task::XStatusIndicator > xProgress = m_xProgress;

    aWriteLock.unlock();
    // <- SAFE ----------------------------------

    implts_makeParentVisibleIfAllowed();

    if (xProgress.is())
        xProgress->start(sText, nRange);

    impl_startWakeUpThread();
    impl_reschedule(sal_True);
}

//-----------------------------------------------
void StatusIndicatorFactory::reset(const css::uno::Reference< css::task::XStatusIndicator >& xChild)
{
    // SAFE -> ----------------------------------
    ReadGuard aReadLock(m_aLock);

    // reset the internal info structure related to this child
    IndicatorStack::iterator pItem = ::std::find(m_aStack.begin(), m_aStack.end(), xChild);
    if (pItem != m_aStack.end())
    {
        pItem->m_nValue = 0;
        pItem->m_sText  = ::rtl::OUString();
    }

    css::uno::Reference< css::task::XStatusIndicator > xActive   = m_xActiveChild;
    css::uno::Reference< css::task::XStatusIndicator > xProgress = m_xProgress;

    aReadLock.unlock();
    // <- SAFE ----------------------------------

    // not the top most child => dont change UI
    // But dont forget Reschedule!
    if (
        (xChild == xActive) &&
        (xProgress.is()   )
       )
        xProgress->reset();

    impl_reschedule(sal_True);
}

//-----------------------------------------------
void StatusIndicatorFactory::end(const css::uno::Reference< css::task::XStatusIndicator >& xChild)
{
    // SAFE -> ----------------------------------
    WriteGuard aWriteLock(m_aLock);

    // remove this child from our stack
    IndicatorStack::iterator pItem = ::std::find(m_aStack.begin(), m_aStack.end(), xChild);
    if (pItem != m_aStack.end())
        m_aStack.erase(pItem);

    // activate next child ... or finish the progress if there is no further one.
    m_xActiveChild.clear();
    ::rtl::OUString                  sText;
    sal_Int32                        nValue = 0;
    IndicatorStack::reverse_iterator pNext  = m_aStack.rbegin();
    if (pNext != m_aStack.rend())
    {
        m_xActiveChild = pNext->m_xIndicator;
        sText          = pNext->m_sText;
        nValue         = pNext->m_nValue;
    }

    css::uno::Reference< css::task::XStatusIndicator > xActive   = m_xActiveChild;
    css::uno::Reference< css::task::XStatusIndicator > xProgress = m_xProgress;

    aWriteLock.unlock();
    // <- SAFE ----------------------------------

    if (xActive.is())
    {
        // There is at least one further child indicator.
        // Actualize our progress, so it shows these values from now.
        if (xProgress.is())
        {
            xProgress->setText (sText );
            xProgress->setValue(nValue);
        }
    }
    else
    {
        // Our stack is empty. No further child exists.
        // Se we must "end" our progress really
        if (xProgress.is())
            xProgress->end();
        // Now hide the progress bar again.
        impl_hideProgress();

        impl_stopWakeUpThread();
    }

    impl_reschedule(sal_True);
}

//-----------------------------------------------
void StatusIndicatorFactory::setText(const css::uno::Reference< css::task::XStatusIndicator >& xChild,
                                     const ::rtl::OUString&                                    sText )
{
    // SAFE -> ----------------------------------
    WriteGuard aWriteLock(m_aLock);

    IndicatorStack::iterator pItem = ::std::find(m_aStack.begin(), m_aStack.end(), xChild);
    if (pItem != m_aStack.end())
        pItem->m_sText = sText;

    css::uno::Reference< css::task::XStatusIndicator > xActive   = m_xActiveChild;
    css::uno::Reference< css::task::XStatusIndicator > xProgress = m_xProgress;

    aWriteLock.unlock();
    // SAFE -> ----------------------------------

    // paint only the top most indicator
    // but dont forget to Reschedule!
    if (
        (xChild == xActive) &&
        (xProgress.is()   )
       )
    {
        xProgress->setText(sText);
    }

    impl_reschedule(sal_True);
}

//-----------------------------------------------
void StatusIndicatorFactory::setValue( const css::uno::Reference< css::task::XStatusIndicator >& xChild ,
                                             sal_Int32                                           nValue )
{
    // SAFE -> ----------------------------------
    WriteGuard aWriteLock(m_aLock);

    sal_Int32 nOldValue = 0;
    IndicatorStack::iterator pItem = ::std::find(m_aStack.begin(), m_aStack.end(), xChild);
    if (pItem != m_aStack.end())
    {
        nOldValue       = pItem->m_nValue;
        pItem->m_nValue = nValue;
    }

    css::uno::Reference< css::task::XStatusIndicator > xActive    = m_xActiveChild;
    css::uno::Reference< css::task::XStatusIndicator > xProgress  = m_xProgress;

    aWriteLock.unlock();
    // SAFE -> ----------------------------------

    if (
        (xChild    == xActive) &&
        (nOldValue != nValue ) &&
        (xProgress.is()      )
       )
    {
        xProgress->setValue(nValue);
    }

    impl_reschedule(sal_False);
}

//-----------------------------------------------
void StatusIndicatorFactory::implts_makeParentVisibleIfAllowed()
{
    // SAFE -> ----------------------------------
    ReadGuard aReadLock(m_aLock);

    if (!m_bAllowParentShow)
        return;

    css::uno::Reference< css::frame::XFrame > xFrame      (m_xFrame.get()      , css::uno::UNO_QUERY);
    css::uno::Reference< css::awt::XWindow >  xPluggWindow(m_xPluggWindow.get(), css::uno::UNO_QUERY);
    css::uno::Reference< css::lang::XMultiServiceFactory > xSMGR( m_xSMGR.get(), css::uno::UNO_QUERY);

    aReadLock.unlock();
    // <- SAFE ----------------------------------

    css::uno::Reference< css::awt::XWindow > xParentWindow;
    if (xFrame.is())
        xParentWindow = xFrame->getContainerWindow();
    else
        xParentWindow = xPluggWindow;

    // dont disturb user in case he put the loading document into the background!
    // Suppress any setVisible() or toFront() call in case the initial show was
    // already made.
    css::uno::Reference< css::awt::XWindow2 > xVisibleCheck(xParentWindow, css::uno::UNO_QUERY);
    sal_Bool bIsVisible = sal_False;
    if (xVisibleCheck.is())
        bIsVisible = xVisibleCheck->isVisible();

    if (bIsVisible)
    {
        impl_showProgress();
        return;
    }

    // Check if the layout manager has been set to invisible state. It this case we are also
    // not allowed to set the frame visible!
    css::uno::Reference< css::beans::XPropertySet > xPropSet(xFrame, css::uno::UNO_QUERY);
    if (xPropSet.is())
    {
        css::uno::Reference< css::frame::XLayoutManager > xLayoutManager;
        xPropSet->getPropertyValue(FRAME_PROPNAME_LAYOUTMANAGER) >>= xLayoutManager;
        if (xLayoutManager.is())
        {
            if ( !xLayoutManager->isVisible() )
                return;
        }
    }

    // Ok the window should be made visible ... because it isn't currently visible.
    // BUT ..!
    // We need a Hack for our applications: They get her progress from the frame directly
    // on saving documents. Because there is no progress set on the MediaDescriptor.
    // But that's wrong. In case the document was opened hidden, they shouldn't use any progress .-(
    // They only possible workaround: dont show the parent window here, if the document was opened hidden.
    sal_Bool bHiddenDoc = sal_False;
    if (xFrame.is())
    {
        css::uno::Reference< css::frame::XController > xController;
        css::uno::Reference< css::frame::XModel >      xModel     ;
        xController = xFrame->getController();
        if (xController.is())
            xModel = xController->getModel();
        if (xModel.is())
        {
            ::comphelper::MediaDescriptor lDocArgs(xModel->getArgs());
            bHiddenDoc = lDocArgs.getUnpackedValueOrDefault(
                ::comphelper::MediaDescriptor::PROP_HIDDEN(),
                (sal_Bool)sal_False);
        }
    }

    if (bHiddenDoc)
        return;

    // OK: The document was not opened in hidden mode ...
    // and the window isn't already visible.
    // Show it and bring it to front.
    // But before we have to be sure, that our internal used helper progress
    // is visible too.
    impl_showProgress();

    ::vos::OClearableGuard aSolarGuard(Application::GetSolarMutex());
    Window* pWindow = VCLUnoHelper::GetWindow(xParentWindow);
    if ( pWindow )
    {
        bool bForceFrontAndFocus(false);
        ::comphelper::ConfigurationHelper::readDirectKey(
            xSMGR,
            ::rtl::OUString::createFromAscii("org.openoffice.Office.Common/View"),
            ::rtl::OUString::createFromAscii("NewDocumentHandling"),
            ::rtl::OUString::createFromAscii("ForceFocusAndToFront"),
            ::comphelper::ConfigurationHelper::E_READONLY) >>= bForceFrontAndFocus;

        pWindow->Show(sal_True, bForceFrontAndFocus ? SHOW_FOREGROUNDTASK : 0 );
    }

    /*
    #i75167# dont disturb window manager handling .-)
    css::uno::Reference< css::awt::XTopWindow > xParentWindowTop(xParentWindow, css::uno::UNO_QUERY);
    if (xParentWindowTop.is())
        xParentWindowTop->toFront();
    */
}

//-----------------------------------------------
void StatusIndicatorFactory::impl_createProgress()
{
    // SAFE -> ----------------------------------
    ReadGuard aReadLock(m_aLock);

    css::uno::Reference< css::frame::XFrame >              xFrame (m_xFrame.get()      , css::uno::UNO_QUERY);
    css::uno::Reference< css::awt::XWindow >               xWindow(m_xPluggWindow.get(), css::uno::UNO_QUERY);
    css::uno::Reference< css::lang::XMultiServiceFactory > xSMGR  = m_xSMGR;

    aReadLock.lock();
    // <- SAFE ----------------------------------

    css::uno::Reference< css::task::XStatusIndicator > xProgress;

    if (xWindow.is())
    {
        // use vcl based progress implementation in plugged mode
        VCLStatusIndicator* pVCLProgress = new VCLStatusIndicator(xSMGR, xWindow);
        xProgress = css::uno::Reference< css::task::XStatusIndicator >(static_cast< css::task::XStatusIndicator* >(pVCLProgress), css::uno::UNO_QUERY);
    }
    else
    if (xFrame.is())
    {
        // use frame layouted progress implementation
        css::uno::Reference< css::beans::XPropertySet > xPropSet(xFrame, css::uno::UNO_QUERY);
        if (xPropSet.is())
        {
            css::uno::Reference< css::frame::XLayoutManager > xLayoutManager;
            xPropSet->getPropertyValue(FRAME_PROPNAME_LAYOUTMANAGER) >>= xLayoutManager;
            if (xLayoutManager.is())
            {
                xLayoutManager->lock();
                xLayoutManager->createElement( PROGRESS_RESOURCE );
                xLayoutManager->hideElement( PROGRESS_RESOURCE );

                css::uno::Reference< css::ui::XUIElement > xProgressBar = xLayoutManager->getElement(PROGRESS_RESOURCE);
                if (xProgressBar.is())
                    xProgress = css::uno::Reference< css::task::XStatusIndicator >(xProgressBar->getRealInterface(), css::uno::UNO_QUERY);
                xLayoutManager->unlock();
            }
        }
    }

    // SAFE -> ----------------------------------
    WriteGuard aWriteLock(m_aLock);
    m_xProgress = xProgress;
    aWriteLock.lock();
    // <- SAFE ----------------------------------
}

//-----------------------------------------------
void StatusIndicatorFactory::impl_showProgress()
{
    // SAFE -> ----------------------------------
    ReadGuard aReadLock(m_aLock);

    css::uno::Reference< css::frame::XFrame >              xFrame (m_xFrame.get()      , css::uno::UNO_QUERY);
    css::uno::Reference< css::awt::XWindow >               xWindow(m_xPluggWindow.get(), css::uno::UNO_QUERY);
    css::uno::Reference< css::lang::XMultiServiceFactory > xSMGR  = m_xSMGR;

    aReadLock.lock();
    // <- SAFE ----------------------------------

    css::uno::Reference< css::task::XStatusIndicator > xProgress;
    
    if (xFrame.is())
    {
        // use frame layouted progress implementation
        css::uno::Reference< css::beans::XPropertySet > xPropSet(xFrame, css::uno::UNO_QUERY);
        if (xPropSet.is())
        {
            css::uno::Reference< css::frame::XLayoutManager > xLayoutManager;
            xPropSet->getPropertyValue(FRAME_PROPNAME_LAYOUTMANAGER) >>= xLayoutManager;
            if (xLayoutManager.is())
            {
                // Be sure that we have always a progress. It can be that our frame
                // was recycled and therefore the progress was destroyed!
                // CreateElement does nothing if there is already a valid progress.
                xLayoutManager->createElement( PROGRESS_RESOURCE );
                xLayoutManager->showElement( PROGRESS_RESOURCE );
                
                css::uno::Reference< css::ui::XUIElement > xProgressBar = xLayoutManager->getElement(PROGRESS_RESOURCE);
                if (xProgressBar.is())
                    xProgress = css::uno::Reference< css::task::XStatusIndicator >(xProgressBar->getRealInterface(), css::uno::UNO_QUERY);
            }
        }

        // SAFE -> ----------------------------------
        WriteGuard aWriteLock(m_aLock);
        m_xProgress = xProgress;
        aWriteLock.lock();
        // <- SAFE ----------------------------------
    }
}

//-----------------------------------------------
void StatusIndicatorFactory::impl_hideProgress()
{
    // SAFE -> ----------------------------------
    ReadGuard aReadLock(m_aLock);

    css::uno::Reference< css::frame::XFrame >              xFrame (m_xFrame.get()      , css::uno::UNO_QUERY);
    css::uno::Reference< css::awt::XWindow >               xWindow(m_xPluggWindow.get(), css::uno::UNO_QUERY);
    css::uno::Reference< css::lang::XMultiServiceFactory > xSMGR  = m_xSMGR;

    aReadLock.lock();
    // <- SAFE ----------------------------------

    if (xFrame.is())
    {
        // use frame layouted progress implementation
        css::uno::Reference< css::beans::XPropertySet > xPropSet(xFrame, css::uno::UNO_QUERY);
        if (xPropSet.is())
        {
            css::uno::Reference< css::frame::XLayoutManager > xLayoutManager;
            xPropSet->getPropertyValue(FRAME_PROPNAME_LAYOUTMANAGER) >>= xLayoutManager;
            if (xLayoutManager.is())
                xLayoutManager->hideElement( PROGRESS_RESOURCE );
        }
    }
}

//-----------------------------------------------
void StatusIndicatorFactory::impl_reschedule(sal_Bool bForce)
{
    // SAFE ->
    ReadGuard aReadLock(m_aLock);
    if (m_bDisableReschedule)
        return;
    aReadLock.unlock();
    // <- SAFE

    sal_Bool bReschedule = bForce;
    if (!bReschedule)
    {
        // SAFE ->
        WriteGuard aWriteLock(m_aLock);
        bReschedule        = m_bAllowReschedule;
        m_bAllowReschedule = sal_False;
        aWriteLock.unlock();
        // <- SAFE
    }

    if (!bReschedule)
        return;

    // SAFE ->
    WriteGuard aGlobalLock(LockHelper::getGlobalLock());

	if (m_nInReschedule == 0)
	{
		++m_nInReschedule;
        aGlobalLock.unlock();
        // <- SAFE

        Application::Reschedule(true);

        // SAFE ->
        aGlobalLock.lock();
		--m_nInReschedule;
	}
}

//-----------------------------------------------
void StatusIndicatorFactory::impl_startWakeUpThread()
{
    // SAFE ->
    WriteGuard aWriteLock(m_aLock);

    if (m_bDisableReschedule)
        return;

    if (!m_pWakeUp)
    {
        m_pWakeUp = new WakeUpThread(this);
        m_pWakeUp->create();
    }
    aWriteLock.unlock();
    // <- SAFE
}

//-----------------------------------------------
void StatusIndicatorFactory::impl_stopWakeUpThread()
{
    // SAFE ->
    WriteGuard aWriteLock(m_aLock);
    if (m_pWakeUp)
    {
        // Thread kill itself after terminate()!
        m_pWakeUp->terminate();
        m_pWakeUp = 0;
    }
    aWriteLock.unlock();
    // <- SAFE
}

} // namespace framework

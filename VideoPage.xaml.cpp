/**************************************************************************
* Copyright (C) 2016 by Savoir-faire Linux                                *
* Author: Jäger Nicolas <nicolas.jager@savoirfairelinux.com>              *
* Author: Traczyk Andreas <andreas.traczyk@savoirfairelinux.com>          *
*                                                                         *
* This program is free software; you can redistribute it and/or modify    *
* it under the terms of the GNU General Public License as published by    *
* the Free Software Foundation; either version 3 of the License, or       *
* (at your option) any later version.                                     *
*                                                                         *
* This program is distributed in the hope that it will be useful,         *
* but WITHOUT ANY WARRANTY; without even the implied warranty of          *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
* GNU General Public License for more details.                            *
*                                                                         *
* You should have received a copy of the GNU General Public License       *
* along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
**************************************************************************/
#include "pch.h"

#include "VideoPage.xaml.h"

using namespace RingClientUWP::Views;

using namespace Concurrency;
using namespace Platform;
using namespace Windows::Devices::Enumeration;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;
using namespace Windows::Media::Capture;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::UI::Core;

VideoPage::VideoPage()
{
    InitializeComponent();
}

void
RingClientUWP::Views::VideoPage::OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e)
{
    updatePageContent();
}

void RingClientUWP::Views::VideoPage::updatePageContent()
{
    auto selectedContact = ViewModel::ContactsViewModel::instance->selectedContact;
    Contact^ contact = selectedContact?
        ViewModel::SmartPanelItemsViewModel::instance->findItem(selectedContact)->_contact:
        nullptr;
    if (!contact)
        return;

    _callee_->Text = contact->name_;
}

void RingClientUWP::Views::VideoPage::Button_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
}


void RingClientUWP::Views::VideoPage::_btnCancel__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{

}

void RingClientUWP::Views::VideoPage::_btnHangUp__Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e)
{
    Contact^ selectedContact = ViewModel::ContactsViewModel::instance->selectedContact;
    Call^ call = selectedContact?
        ViewModel::SmartPanelItemsViewModel::instance->findItem(selectedContact)->_call:
        nullptr;
    if (call)
        RingD::instance->hangUpCall(call);
    pressHangUpCall();
}


void RingClientUWP::Views::VideoPage::_btnPause__Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e)
{
    pauseCall();
}


void RingClientUWP::Views::VideoPage::_btnChat__Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e)
{
    chatPanelCall();
}


void RingClientUWP::Views::VideoPage::_btnAddFriend__Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e)
{
    addContactCall();
}


void RingClientUWP::Views::VideoPage::_btnSwitch__Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e)
{
    transferCall();
}


void RingClientUWP::Views::VideoPage::_btnMicrophone__Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e)
{
    switchMicrophoneStateCall();
}


void RingClientUWP::Views::VideoPage::_btnMemo__Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e)
{
    reccordVideoCall();
}


void RingClientUWP::Views::VideoPage::_btnHQ__Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e)
{
    qualityVideoLevelCall();
}


void RingClientUWP::Views::VideoPage::_btnVideo__Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e)
{
    switchVideoStateCall();
}


void RingClientUWP::Views::VideoPage::_videoControl__PointerMoved(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e)
{
    if (barFading)
        myStoryboard->Begin();
    barFading_ = true;
}


void RingClientUWP::Views::VideoPage::btnAny_entered(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e)
{
    barFading_ = false;
    myStoryboard->Stop();
}


void RingClientUWP::Views::VideoPage::btnAny_exited(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e)
{
    barFading_ = true;
}
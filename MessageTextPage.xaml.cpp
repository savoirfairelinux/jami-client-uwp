/**************************************************************************
* Copyright (C) 2016 by Savoir-faire Linux                                *
* Author: Jäger Nicolas <nicolas.jager@savoirfairelinux.com>              *
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
#include "ContactsViewModel.h"

#include "MainPage.xaml.h"

#include "MessageTextPage.xaml.h"

using namespace RingClientUWP::Views;
using namespace RingClientUWP::ViewModel;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Documents;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;
using namespace Windows::ApplicationModel::Core;
using namespace Platform;
using namespace Windows::UI::Core;

MessageTextPage::MessageTextPage()
{
    InitializeComponent();

    /* bind the source to account only able to be used to contact the contact */
    _associableAccountsList_->ItemsSource = AccountsViewModel::instance->accountsList;
    _associableAccountsList_->SelectionChanged += ref new Windows::UI::Xaml::Controls::SelectionChangedEventHandler(this, &RingClientUWP::Views::MessageTextPage::OnSelectionChanged);

    /* connect to delegates */
    RingD::instance->incomingAccountMessage += ref new IncomingAccountMessage([&](String^ accountId,
    String^ fromRingId, String^ payload) {
        scrollDown();
    });
    RingD::instance->incomingMessage += ref new RingClientUWP::IncomingMessage(this, &RingClientUWP::Views::MessageTextPage::OnincomingMessage);

}

void
RingClientUWP::Views::MessageTextPage::updatePageContent()
{
    auto item = SmartPanelItemsViewModel::instance->_selectedItem;
    auto contact = item->_contact;



    if (!contact) /* should never happen */
        return;


    /* show the name of contact on the page */
    _title_->Text = contact->name_;

    /* show messages */
    _messagesList_->ItemsSource = contact->_conversation->_messages;

    /* select the associated accountId stored with the contact */
    auto accountIdAssociated = contact->_accountIdAssociated;
    auto list = AccountsViewModel::instance->accountsList;
    unsigned int index = 0;
    bool found = true;

    for (auto item : list)
        if (item->accountID_ == accountIdAssociated) {
            found = list->IndexOf(item, &index);
            break;
        }


    if (found)
        _associableAccountsList_->SelectedIndex = index;
    else
        ERR_("mismatch between accountIdAssociated and associable accounts!");

    /* scroll to the last message on the page*/
    scrollDown();

}

void RingClientUWP::Views::MessageTextPage::scrollDown()
{
    _scrollView_->UpdateLayout();
    _scrollView_->ScrollToVerticalOffset(_scrollView_->ScrollableHeight);
}

void
RingClientUWP::Views::MessageTextPage::_sendBtn__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    sendMessage();
}

void
RingClientUWP::Views::MessageTextPage::_messageTextBox__KeyDown(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e)
{
    if (e->Key == Windows::System::VirtualKey::Enter) {
        sendMessage();
    }
}

void
RingClientUWP::Views::MessageTextPage::sendMessage()
{
    auto item = SmartPanelItemsViewModel::instance->_selectedItem;
    auto contact = item->_contact;

    auto txt = _messageTextBox_->Text;

    /* empty the textbox */
    _messageTextBox_->Text = "";

    if (!contact || txt->IsEmpty())
        return;

    RingD::instance->sendAccountTextMessage(txt);
    scrollDown();
}

Object ^ RingClientUWP::Views::BubbleBackground::Convert(Object ^ value, Windows::UI::Xaml::Interop::TypeName targetType, Object ^ parameter, String ^ language)
{
    auto settings = ref new Windows::UI::ViewManagement::UISettings();
    auto color = settings->GetColorValue(Windows::UI::ViewManagement::UIColorType::Accent);

    return ((bool)value) ? ref new SolidColorBrush(color) : ref new SolidColorBrush(Windows::UI::Colors::LightBlue);
}

// we only do OneWay so the next function is not used
Object ^ RingClientUWP::Views::BubbleBackground::ConvertBack(Object ^ value, Windows::UI::Xaml::Interop::TypeName targetType, Object ^ parameter, String ^ language)
{
    throw ref new Platform::NotImplementedException();
}

RingClientUWP::Views::BubbleBackground::BubbleBackground()
{}

Object ^ RingClientUWP::Views::BubbleHorizontalAlignement::Convert(Object ^ value, Windows::UI::Xaml::Interop::TypeName targetType, Object ^ parameter, String ^ language)
{
    return ((bool)value) ? Windows::UI::Xaml::HorizontalAlignment::Left : Windows::UI::Xaml::HorizontalAlignment::Right;
}

// we only do OneWay so the next function is not used
Object ^ RingClientUWP::Views::BubbleHorizontalAlignement::ConvertBack(Object ^ value, Windows::UI::Xaml::Interop::TypeName targetType, Object ^ parameter, String ^ language)
{
    throw ref new Platform::NotImplementedException();
}

RingClientUWP::Views::BubbleHorizontalAlignement::BubbleHorizontalAlignement()
{}


void RingClientUWP::Views::MessageTextPage::OnincomingMessage(Platform::String ^callId, Platform::String ^payload)
{
    scrollDown();
}


void RingClientUWP::Views::MessageTextPage::OnSelectionChanged(Platform::Object ^sender, Windows::UI::Xaml::Controls::SelectionChangedEventArgs ^e)
{
    auto account = dynamic_cast<Account^>(_associableAccountsList_->SelectedItem);
    SmartPanelItemsViewModel::instance->_selectedItem->_contact->_accountIdAssociated = account->accountID_;
}

void RingClientUWP::Views::MessageTextPage::_deleteContact__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    auto item = SmartPanelItemsViewModel::instance->_selectedItem;
    auto contact = item->_contact;

    closeMessageTextPage();

    ContactsViewModel::instance->deleteContact(contact);
    SmartPanelItemsViewModel::instance->removeItem(item);
}

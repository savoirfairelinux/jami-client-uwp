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
#include "ContactListModel.h"

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

using namespace Windows::UI::Popups;

// refacto : the message text page should be
MessageTextPage::MessageTextPage()
{
    InitializeComponent();

    /* connect to delegates */
    RingD::instance->incomingAccountMessage += ref new IncomingAccountMessage([&](String^ accountId, String^ fromRingId, String^ payload) {
        scrollDown();
    });
    RingD::instance->incomingMessage += ref new RingClientUWP::IncomingMessage(this, &RingClientUWP::Views::MessageTextPage::OnincomingMessage);

    SmartPanelItemsViewModel::instance->selectedItemUpdated += ref new SelectedItemUpdated([&]() {
        updatePageContent();
    });

    RingD::instance->messageDataLoaded += ref new MessageDataLoaded([&]() { scrollDown(); });
}

void
MessageTextPage::updatePageContent()
{
    if (auto item = SmartPanelItemsViewModel::instance->_selectedItem) {
        auto contact = item->_contact;

        if (!contact) /* should never happen */
            return;

        /* show the name of contact on the page */
        _contactName_->Text = contact->_name;
        _displayName_->Text = contact->_displayName;
		_contactNameSeperator_->Text = contact->_displayName->IsEmpty() ? "" : "-";
        contact->_unreadMessages = 0;

        String^ image_path = Utils::toPlatformString(RingD::instance->getLocalFolder()) + ".vcards\\" + contact->_vcardUID + ".png";
        if (Utils::fileExists(Utils::toString(image_path))) {
            auto uri = ref new Windows::Foundation::Uri(image_path);
            _contactBarAvatar_->ImageSource = ref new Windows::UI::Xaml::Media::Imaging::BitmapImage(uri);
        }
        else {
            auto uri = ref new Windows::Foundation::Uri("ms-appx:///Assets/TESTS/contactAvatar.png");
            _contactBarAvatar_->ImageSource = ref new Windows::UI::Xaml::Media::Imaging::BitmapImage(uri);
        }

        /* show messages */
        _messagesList_->ItemsSource = contact->_conversation->_messages;

        /* select the associated accountId stored with the contact */
        auto accountIdAssociated = contact->_accountIdAssociated;
        auto list = AccountsViewModel::instance->accountsList;
        unsigned int index = 0;
        bool found = true;

        for (auto item : list) {
            if (item->accountID_ == accountIdAssociated) {
                found = list->IndexOf(item, &index);
                break;
            }
        }

        /* scroll to the last message on the page*/
        scrollDown();
    }
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
    auto color1 = Windows::UI::ColorHelper::FromArgb(255, 0, 76, 96);
    auto color2 = Windows::UI::ColorHelper::FromArgb(255, 58, 192, 210); // refacto : defines colors used by the application as globals
    return ((bool)value) ? ref new SolidColorBrush(color1) : ref new SolidColorBrush(color2);
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
}

void RingClientUWP::Views::MessageTextPage::_deleteContactBtn__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    auto item = SmartPanelItemsViewModel::instance->_selectedItem;
    auto contact = item->_contact;

    auto messageDialog = ref new MessageDialog("Are you sure you want to remove this contact?", "Remove Contact");

    messageDialog->Commands->Append(ref new UICommand("Cancel", ref new UICommandInvokedHandler([this](IUICommand^ command)
    {})));
    messageDialog->Commands->Append(ref new UICommand("Remove", ref new UICommandInvokedHandler([=](IUICommand^ command)
    {
        closeMessageTextPage();
        String^ accountIdAssociated = SmartPanelItemsViewModel::instance->getAssociatedAccountId(item);
        if (auto contactListModel = AccountsViewModel::instance->getContactListModel(Utils::toString(accountIdAssociated)))
            contactListModel->deleteContact(contact);
        SmartPanelItemsViewModel::instance->removeItem(item);
    })));

    messageDialog->DefaultCommandIndex = 1;

    messageDialog->ShowAsync();
}


void RingClientUWP::Views::MessageTextPage::_clearConversationBtn__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    auto item = SmartPanelItemsViewModel::instance->_selectedItem;
    auto contact = item->_contact;

    auto messageDialog = ref new MessageDialog("Are you sure you want to clear the conversation?", "Clear conversation");

    messageDialog->Commands->Append(ref new UICommand("Cancel", ref new UICommandInvokedHandler([this](IUICommand^ command)
    {})));
    messageDialog->Commands->Append(ref new UICommand("Clear", ref new UICommandInvokedHandler([=](IUICommand^ command)
    {
        contact->_conversation->_messages->Clear();
        contact->saveConversationToFile();
    })));

    messageDialog->DefaultCommandIndex = 1;

    messageDialog->ShowAsync();
}


void RingClientUWP::Views::MessageTextPage::_audioCallBtn__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    auto button = dynamic_cast<Button^>(e->OriginalSource);
    if (button) {
        auto item = SmartPanelItemsViewModel::instance->_selectedItem;
        if (item) {
            auto contact = item->_contact;
            if (contact)
                RingD::instance->placeCall(contact);
        }
    }
}


void RingClientUWP::Views::MessageTextPage::_videoCallBtn__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    auto button = dynamic_cast<Button^>(e->OriginalSource);
    if (button) {
        auto item = SmartPanelItemsViewModel::instance->_selectedItem;
        if (item) {
            auto contact = item->_contact;
            if (contact)
                RingD::instance->placeCall(contact);
        }
    }
}


void RingClientUWP::Views::MessageTextPage::Page_Loaded(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{

}

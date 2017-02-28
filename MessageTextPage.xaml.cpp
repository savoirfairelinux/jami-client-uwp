/**************************************************************************
* Copyright (C) 2016 by Savoir-faire Linux                                *
* Author: Jäger Nicolas <nicolas.jager@savoirfairelinux.com>              *
* Author: Traczyk Andreas<andreas.traczyk@savoirfairelinux.com>           *
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
#include "SmartPanel.xaml.h"
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
using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml::Media::Animation;
using namespace Windows::UI::Xaml::Shapes;
using namespace Windows::UI::Popups;
using namespace Windows::ApplicationModel::DataTransfer;
using namespace Windows::System::Threading;

// refacto : the message text page should be
MessageTextPage::MessageTextPage()
{
    InitializeComponent();

    /* connect to delegates */
    RingD::instance->incomingAccountMessage += ref new IncomingAccountMessage([&](String^ accountId, String^ fromRingId, String^ payload) {
        scrollDown();
    });
    RingD::instance->incomingMessage += ref new RingClientUWP::IncomingMessage(this, &RingClientUWP::Views::MessageTextPage::OnincomingMessage);

    RingD::instance->messageDataLoaded += ref new MessageDataLoaded([&]() { scrollDown(); });

    RingD::instance->messageStatusUpdated += ref new MessageStatusUpdated(this, &MessageTextPage::updateMessageStatus);

    lastMessageText = "";

    conversation_ = ref new Conversation();
}

void
MessageTextPage::updateMessageStatus(String^ messageId, int status)
{
    MSG_("message status update [id: " + messageId + ", status: " + status.ToString() + "]");
    if (status < 2)
        return;

    // One does not simply begin a storyboard nested in a datatemplated item.
    // We won't check the validity of the UIElement refs so an InvalidCast exception will be thrown
    // if the xaml gets changed without adjusting this messy workaround.
    auto items = _messagesList_->Items;
    for (int i = 0; i < items->Size; ++i) {
        if (auto message = dynamic_cast<ConversationMessage^>(items->GetAt(i))) {
            if (message->MessageId == messageId) {
                auto depObj = _messagesList_->ItemContainerGenerator->ContainerFromItem(items->GetAt(i));
                auto gridElement = Utils::xaml::FindVisualChildByName(depObj, "_confirmationCheckGrid_");
                auto grid = dynamic_cast<Grid^>(gridElement);
                auto rect = dynamic_cast<Rectangle^>(grid->Children->GetAt(0));
                auto eventTrigger = dynamic_cast<EventTrigger^>(rect->Triggers->GetAt(0));
                auto beginStoryboard = dynamic_cast<BeginStoryboard^>(eventTrigger->Actions->GetAt(0));
                if (beginStoryboard) {
                    beginStoryboard->Storyboard->Begin();
                    grid->Visibility = VIS::Visible;
                }
            }
        }
    }
}

void
MessageTextPage::updatePageContent(SmartPanelItem^ item)
{
    if (auto selectedItem = (item ? item : SmartPanelItemsViewModel::instance->_selectedItem)) {
        auto contact = selectedItem->_contact;

        if (!contact) /* should never happen */
            return;

        /* show the name of contact on the page */
        auto firstChoice = contact->_bestName2;
        auto secondChoice = contact->_bestName3;
        _contactName_->Text = firstChoice;
        _tt_contactName_->Text = firstChoice;
        if (firstChoice == contact->ringID_) {
            _contactName2_->Visibility = VIS::Collapsed;
        }
        else {
            _contactName2_->Visibility = VIS::Visible;
            _contactName2_->Text = secondChoice;
            _tt_contactName2_->Text = secondChoice;
        }

        //_contactBarAvatar_->ImageSource = RingClientUWP::ResourceMananger::instance->imageFromRelativePath(contact->_avatarImage);
        auto avatarImageUri = ref new Windows::Foundation::Uri(contact->_avatarImage);
        _contactBarAvatar_->ImageSource = ref new BitmapImage(avatarImageUri);

        if (contact->_trustStatus == TrustStatus::INCOMING_CONTACT_REQUEST ||
            contact->_trustStatus == TrustStatus::CONTACT_REQUEST_SENT ||
            contact->_trustStatus == TrustStatus::UNKNOWN ||
            contact->_trustStatus == TrustStatus::INGNORED)
            _sendContactRequestBtn_->Visibility = VIS::Visible;
        else
            _sendContactRequestBtn_->Visibility = VIS::Collapsed;

        if (item == nullptr) {
            contact->_unreadMessages = 0;

            /* show messages */
            _messagesList_->ItemsSource = nullptr;
            _fadeInMessagesListStoryBoard_->Begin();
            _fadeInMessagesPageInfoStoryBoard_->Begin();
            Utils::runOnUIThreadDelayed(50, [this, contact]() {
                //auto start = std::chrono::steady_clock::now();
                _messagesList_->ItemsSource = contact->_conversation->_messages;
                _easeUpMessagesListStoryBoard_->Begin();
                scrollDown();
                /*auto end = std::chrono::steady_clock::now();
                auto diff_ns = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
                MSG_("FormattedText : " + diff_ns.count().ToString() + " microseconds");*/
            });
        }

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
RingClientUWP::Views::MessageTextPage::sendMessage()
{
    auto item = SmartPanelItemsViewModel::instance->_selectedItem;
    auto contact = item->_contact;

    auto message = _messageTextBox_->Text;

    IBuffer^ buffUTF8 = CryptographicBuffer::ConvertStringToBinary(message, BinaryStringEncoding::Utf8);
    auto stdMessage = Utils::getData(buffUTF8);
    MSG_(stdMessage);

    if (!contact || message->IsEmpty())
        return;

    /* empty the textbox */
    _messageTextBox_->Text = "";

    RingD::instance->sendAccountTextMessage(message);
    scrollDown();
}

Object ^ RingClientUWP::Views::BubbleBackground::Convert(Object ^ value, Windows::UI::Xaml::Interop::TypeName targetType, Object ^ parameter, String ^ language)
{
    /*auto c1 = Utils::ColorFromString("#ffdbdfdf");
    auto c2 = Utils::ColorFromString("#ffbae5f0");*/
    auto c1 = Utils::ColorFromString("#ffebefef");
    auto c2 = Utils::ColorFromString("#ffcfebf5");
    return ((bool)value) ? ref new SolidColorBrush(c1) : ref new SolidColorBrush(c2);
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
    messageDialog->Commands->Append(ref new UICommand("Remove", ref new UICommandInvokedHandler([this, item, contact](IUICommand^ command)
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
    messageDialog->Commands->Append(ref new UICommand("Clear", ref new UICommandInvokedHandler([contact](IUICommand^ command)
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
        if (auto item = SmartPanelItemsViewModel::instance->_selectedItem) {
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
        if (auto item = SmartPanelItemsViewModel::instance->_selectedItem) {
            auto contact = item->_contact;
            if (contact)
                RingD::instance->placeCall(contact);
        }
    }
}

void
MessageTextPage::_messageTextBox__TextChanged(Platform::Object^ sender, TextChangedEventArgs^ e)
{
    if (_messageTextBox_->Text->Length() == (lastMessageText->Length() + 1)) {
        auto strMessage = Utils::toString(_messageTextBox_->Text);
        if (strMessage.substr(strMessage.length() - 1) == "\r") {
            _messageTextBox_->Text = Utils::toPlatformString(strMessage.substr(0, strMessage.size() - 1));
            if (lastMessageText->Length() != 0)
                sendMessage();
        }
    }

    lastMessageText = _messageTextBox_->Text;
}


void RingClientUWP::Views::MessageTextPage::_sendContactRequestBtn__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    if (auto item = SmartPanelItemsViewModel::instance->_selectedItem) {
        if (auto contact = item->_contact) {
            auto accountId = Utils::toString(AccountListItemsViewModel::instance->getSelectedAccountId());
            auto uri = Utils::toString(contact->ringID_);
            auto vcard = Configuration::UserPreferences::instance->getVCard();
            RingD::instance->sendContactRequest(accountId, uri, vcard->asString());
        }
    }
}

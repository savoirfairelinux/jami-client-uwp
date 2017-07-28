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
    RingD::instance->incomingAccountMessage += ref new IncomingAccountMessage([&](String^ accountId, String^ fromRingId, String^ payload)
    {
        scrollDown();
    });
    RingD::instance->incomingMessage += ref new RingClientUWP::IncomingMessage(this, &RingClientUWP::Views::MessageTextPage::OnincomingMessage);

    RingD::instance->messageDataLoaded += ref new MessageDataLoaded([&]() { scrollDown(); });

    RingD::instance->messageStatusUpdated += ref new MessageStatusUpdated(this, &MessageTextPage::updateMessageStatus);

    RingD::instance->windowResized +=
        ref new WindowResized([=](float width, float height)
    {
        if (RingD::instance->mainPage) {
            bool isSmartPanelOpen = RingD::instance->mainPage->isSmartPanelOpen;
            if (width <= 728 && (width > 638 && isSmartPanelOpen)) {
                _contactButtonsGrid_->SetValue(Grid::ColumnSpanProperty, 3);
                _contactButtonsGrid_->SetValue(Grid::ColumnProperty, static_cast<Object^>(0));
                _contactButtonsGrid_->SetValue(Grid::RowProperty, 1);
                _contactButtonsGrid_->Margin = Windows::UI::Xaml::Thickness(6.0, 0.0, 14.0, 6.0);
                _contactButtonsGrid_->HorizontalAlignment = Windows::UI::Xaml::HorizontalAlignment::Left;
            }
            else {
                _contactButtonsGrid_->SetValue(Grid::ColumnSpanProperty, 1);
                _contactButtonsGrid_->SetValue(Grid::ColumnProperty, 2);
                _contactButtonsGrid_->SetValue(Grid::RowProperty, static_cast<Object^>(0));
                _contactButtonsGrid_->Margin = Windows::UI::Xaml::Thickness(0.0, 0.0, 14.0, 0.0);
                _contactButtonsGrid_->HorizontalAlignment = Windows::UI::Xaml::HorizontalAlignment::Right;
            }
        }
    });

    RingD::instance->vCardUpdated += ref new VCardUpdated([&](Contact^ contact)
    {
        Utils::runOnUIThread([this, contact]() {
            if (auto item = SmartPanelItemsViewModel::instance->findItem(contact)) {
                updatePageContent(item);
            }
        });
    });

    lastMessageText = "";

    conversation_ = ref new Vector<ConversationMessage^>();
    converstationChunk_ = ref new Platform::Array<ConversationMessage^>(20);
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
        if (contact->_avatarImage != " ") {
            auto avatarImageUri = ref new Windows::Foundation::Uri(contact->_avatarImage);
            _contactBarAvatar_->ImageSource = ref new BitmapImage(avatarImageUri);
            _defaultContactBarAvatarGrid_->Visibility = VIS::Collapsed;
            _contactBarAvatarGrid_->Visibility = VIS::Visible;
        }
        else {
            _defaultContactBarAvatarGrid_->Visibility = VIS::Visible;
            _contactBarAvatarGrid_->Visibility = VIS::Collapsed;
            _defaultAvatar_->Fill = contact->_avatarColorBrush;
            _defaultAvatarInitial_->Text = Utils::getUpperInitial(contact->_bestName2);
        }

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
            _fadeInMessagesPageInfoStoryBoard_->Begin();
            Utils::runOnUIThreadDelayed(50, [this, contact]() {
                auto start = std::chrono::steady_clock::now();
                /*auto conversationSize = contact->_conversation->_messages->Size;
                if (conversationSize > 20) {
                    conversation_->Clear();
                    contact->_conversation->_messages->GetMany(conversationSize - 20, converstationChunk_);
                    for (auto message : converstationChunk_) {
                        conversation_->Append(message);
                    }
                    _messagesList_->ItemsSource = conversation_;
                }
                else */{
                    _messagesList_->ItemsSource = contact->_conversation->_messages;
                }
                _fadeInMessagesListStoryBoard_->Begin();
                _easeUpMessagesListStoryBoard_->Begin();
                scrollDown();
                auto end = std::chrono::steady_clock::now();
                auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            });
        }
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
    if (auto item = SmartPanelItemsViewModel::instance->_selectedItem) {
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
}

void RingClientUWP::Views::MessageTextPage::OnincomingMessage(Platform::String ^callId, Platform::String ^payload)
{
    scrollDown();
}


void RingClientUWP::Views::MessageTextPage::OnSelectionChanged(Platform::Object ^sender, Windows::UI::Xaml::Controls::SelectionChangedEventArgs ^e)
{
}

void RingClientUWP::Views::MessageTextPage::_clearConversationBtn__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    if (auto item = SmartPanelItemsViewModel::instance->_selectedItem) {
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
MessageTextPage::_messageTextBox__TextChanged(Object^ sender, TextChangedEventArgs^ e)
{
    bool carriageReturnPressed = false;
    if (_messageTextBox_->Text->Length() == (lastMessageText->Length() + 1) &&
        _messageTextBox_->Text != "\r") {
        unsigned cursorPos = 0;
        auto strMessage = Utils::toString(_messageTextBox_->Text);
        auto strLastMessage = Utils::toString(lastMessageText);
        for (std::string::size_type i = 0; i < strLastMessage.size(); ++i) {
            if (strMessage[i] != strLastMessage[i]) {
                auto changed = strMessage.substr(i, 1);
                if (changed == "\r") {
                    carriageReturnPressed = true;
                }
                break;
            }
        }

        if (strMessage.substr(strMessage.length() - 1) == "\r") {
            if (lastMessageText->Length() != 0) {
                carriageReturnPressed = true;
            }
        }
    }

    if (carriageReturnPressed && !(RingD::instance->isCtrlPressed || RingD::instance->isShiftPressed)) {
        _messageTextBox_->Text = lastMessageText;
        sendMessage();
        lastMessageText = "";
    }

    DependencyObject^ child = VisualTreeHelper::GetChild(_messageTextBox_, 0);
    auto sendBtnElement = Utils::xaml::FindVisualChildByName(child, "_sendBtn_");
    auto sendBtn = dynamic_cast<Button^>(sendBtnElement);
    if (_messageTextBox_->Text != "") {
        sendBtn->IsEnabled = true;
    }
    else {
        sendBtn->IsEnabled = false;
    }

    lastMessageText = _messageTextBox_->Text;
}

void
MessageTextPage::_messageTextBox__KeyDown(Object^ sender, KeyRoutedEventArgs^ e)
{
}

void
MessageTextPage::_sendContactRequestBtn__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
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

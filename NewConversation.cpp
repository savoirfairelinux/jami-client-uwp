/**************************************************************************
* Copyright (C) 2017 by Savoir-faire Linux                                *
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

#include "NewConversation.h"
#include "MessageTextPage.xaml.h"

using namespace Windows::ApplicationModel::Core;
using namespace Platform;
using namespace Windows::UI::Core;

using namespace RingClientUWP;

NewConversationMessage::NewConversationMessage(String^ uid)
{
    message_ = std::make_unique<Models::Conversation::Message::Info>(Utils::toString(uid));
}

void
NewConversationMessage::NotifyPropertyChanged(String^ propertyName)
{
    CoreApplicationView^ view = CoreApplication::MainView;
    view->CoreWindow->Dispatcher->RunAsync(
        CoreDispatcherPriority::High,
        ref new DispatchedHandler([this, propertyName]()
    {
        PropertyChanged(this, ref new PropertyChangedEventArgs(propertyName));
    }));
}

String^
NewConversationMessage::getMessageAvatar()
{
    if (ViewModel::SmartPanelItemsViewModel::instance->_selectedItem)
        return ViewModel::SmartPanelItemsViewModel::instance->_selectedItem->_contact->_avatarImage;
    return L" ";
}

SolidColorBrush^
NewConversationMessage::getMessageAvatarColorBrush()
{
    if (ViewModel::SmartPanelItemsViewModel::instance->_selectedItem)
        return ViewModel::SmartPanelItemsViewModel::instance->_selectedItem->_contact->_avatarColorBrush;
    return ref new SolidColorBrush(Utils::xaml::ColorFromString(L"#ff808080"));
}

String^
NewConversationMessage::getMessageAvatarInitial()
{
    if (ViewModel::SmartPanelItemsViewModel::instance->_selectedItem)
        return Utils::getUpperInitial(ViewModel::SmartPanelItemsViewModel::instance->_selectedItem->_contact->_bestName2);
    return L"?";
}
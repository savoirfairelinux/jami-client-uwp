/**************************************************************************
* Copyright (C) 2016 by Savoir-faire Linux                                *
* Author: Jäger Nicolas <nicolas.jager@savoirfairelinux.com>              *
* Author: Traczyk Andreas <traczyk.andreas@savoirfairelinux.com>          *
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

#include "Contact.h"

using namespace Windows::ApplicationModel::Core;
using namespace Platform;
using namespace Windows::Data::Json;
using namespace Windows::UI::Core;

using namespace RingClientUWP;
using namespace ViewModel;

Contact::Contact(String^ name,
                 String^ ringID)
{
    name_ = name;
    ringID_ = ringID;
    conversation_ = ref new Conversation();
    notificationNewMessage_ = Windows::UI::Xaml::Visibility::Collapsed;
    unreadMessages_ = 0; // not saved on disk yet (TO DO)

    /* connect to delegate */
    ContactsViewModel::instance->notifyNewConversationMessage += ref new NotifyNewConversationMessage([&] () {
        notificationNewMessage = Windows::UI::Xaml::Visibility::Visible;
        unreadMessages_++;
        PropertyChanged(this, ref new PropertyChangedEventArgs("unreadMessages"));
    });
    ContactsViewModel::instance->newContactSelected += ref new RingClientUWP::NewContactSelected([&]() {
        if (ContactsViewModel::instance->selectedContact == this) {
            PropertyChanged(this, ref new PropertyChangedEventArgs("unreadMessages"));
            notificationNewMessage = Windows::UI::Xaml::Visibility::Collapsed;
            unreadMessages_ = 0;
        }
    });


}

void
Contact::NotifyPropertyChanged(String^ propertyName)
{
    CoreApplicationView^ view = CoreApplication::MainView;
    view->CoreWindow->Dispatcher->RunAsync(
        CoreDispatcherPriority::Normal,
        ref new DispatchedHandler([this, propertyName]()
    {
        PropertyChanged(this, ref new PropertyChangedEventArgs(propertyName));

    }));
}

JsonObject^
Contact::ToJsonObject()
{
    JsonObject^ contactObject = ref new JsonObject();
    contactObject->SetNamedValue(nameKey, JsonValue::CreateStringValue(name_));
    contactObject->SetNamedValue(ringIDKey, JsonValue::CreateStringValue(ringID_));

    JsonObject^ jsonObject = ref new JsonObject();
    jsonObject->SetNamedValue(contactKey, contactObject);

    return jsonObject;
}

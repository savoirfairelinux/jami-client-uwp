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

#include "ObjBase.h" // for CoCreateGuid

using namespace Windows::ApplicationModel::Core;
using namespace Platform;
using namespace Windows::Data::Json;
using namespace Windows::UI::Core;

using namespace RingClientUWP;
using namespace ViewModel;

Contact::Contact(String^ name,
                 String^ ringID,
                 String^ GUID,
                 unsigned int unreadmessages)
{
    name_   = name;
    ringID_ = ringID;
    GUID_   = GUID;

    if (GUID_ == nullptr)
        GUID_ = Utils::GetNewGUID();

    //RingDebug::instance->print(Utils::toString(GUID_).c_str());

    // load conversation from disk
    conversation_ = ref new Conversation();
    StorageFolder^ localfolder = ApplicationData::Current->LocalFolder;
    String^ messagesFile = ".messages\\" + GUID_ + ".json";
    Utils::fileExists(ApplicationData::Current->LocalFolder,
                      messagesFile)
    .then([this,messagesFile](bool messages_file_exists)
    {
        if (messages_file_exists) {
            try {
                create_task(ApplicationData::Current->LocalFolder->GetFileAsync(messagesFile))
                .then([this](StorageFile^ file)
                {
                    create_task(FileIO::ReadTextAsync(file))
                    .then([this](String^ fileContents) {
                        if (fileContents != nullptr)
                            DestringifyConversation(fileContents);
                    });
                });
            }
            catch (Exception^ e) {
                RingDebug::instance->print("Exception while opening messages file");
            }
        }
    });

    notificationNewMessage_ = Windows::UI::Xaml::Visibility::Collapsed;
    unreadMessages_ = unreadmessages; // not saved on disk yet (TO DO)

    if(unreadMessages_) {
        notificationNewMessage = Windows::UI::Xaml::Visibility::Visible;
        PropertyChanged(this, ref new PropertyChangedEventArgs("unreadMessages"));
    }

    /* connect to delegate */
    ContactsViewModel::instance->notifyNewConversationMessage += ref new NotifyNewConversationMessage([&] () {
        PropertyChanged(this, ref new PropertyChangedEventArgs("unreadMessages"));
    });
    ContactsViewModel::instance->newContactSelected += ref new RingClientUWP::NewContactSelected([&]() {
        if (ContactsViewModel::instance->selectedContact == this) {
            PropertyChanged(this, ref new PropertyChangedEventArgs("unreadMessages"));
            notificationNewMessage = Windows::UI::Xaml::Visibility::Collapsed;
            unreadMessages_ = 0;
            ContactsViewModel::instance->saveContactsToFile();
        }
    });
}

void
Contact::addNotifyNewConversationMessage()
{
    notificationNewMessage = Windows::UI::Xaml::Visibility::Visible;
    unreadMessages_++;
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
    contactObject->SetNamedValue(GUIDKey, JsonValue::CreateStringValue(GUID_));
    contactObject->SetNamedValue(unreadMessagesKey, JsonValue::CreateNumberValue(unreadMessages_));

    JsonObject^ jsonObject = ref new JsonObject();
    jsonObject->SetNamedValue(contactKey, contactObject);

    return jsonObject;
}

String^
Contact::StringifyConversation()
{
    JsonArray^ jsonArray = ref new JsonArray();

    for (unsigned int i = 0; i < conversation_->_messages->Size; i++) {
        jsonArray->Append(conversation_->_messages->GetAt(i)->ToJsonObject());
    }

    JsonObject^ jsonObject = ref new JsonObject();
    jsonObject->SetNamedValue(conversationKey, jsonArray);

    return jsonObject->Stringify();
}

void
Contact::DestringifyConversation(String^ data)
{
    JsonObject^ jsonObject = JsonObject::Parse(data);
    String^     date;
    bool        fromContact;
    String^     payload;

    JsonArray^ messageList = jsonObject->GetNamedArray(conversationKey, ref new JsonArray());
    for (unsigned int i = 0; i < messageList->Size; i++) {
        IJsonValue^ message = messageList->GetAt(i);
        if (message->ValueType == JsonValueType::Object) {
            JsonObject^ jsonMessageObject = message->GetObject();
            JsonObject^ messageObject = jsonMessageObject->GetNamedObject(messageKey, nullptr);
            if (messageObject != nullptr) {
                date = messageObject->GetNamedString(dateKey, "");
                fromContact = messageObject->GetNamedBoolean(fromContactKey, "");
                payload = messageObject->GetNamedString(payloadKey, "");
            }
            conversation_->addMessage(date, fromContact, payload);
        }
    }
}

void
Contact::saveConversationToFile()
{
    StorageFolder^ localfolder = ApplicationData::Current->LocalFolder;
    String^ messagesFile = ".messages\\" + GUID_ + ".json";

    try {
        create_task(localfolder->CreateFileAsync(messagesFile
                    , Windows::Storage::CreationCollisionOption::ReplaceExisting))
        .then([&](StorageFile^ file) {
            try {
                FileIO::WriteTextAsync(file, StringifyConversation());
            }
            catch (Exception^ e) {
                RingDebug::instance->print("Exception while writing to conversation file");
            }
        });
    }
    catch (Exception^ e) {
        RingDebug::instance->print("Exception while opening conversation file");
    }
}
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

#include <ObjBase.h> // for CoCreateGuid

#include "fileutils.h"
#include "direct.h"

using namespace Windows::ApplicationModel::Core;
using namespace Platform;
using namespace Windows::Data::Json;
using namespace Windows::UI::Core;

using namespace RingClientUWP;
using namespace ViewModel;

Contact::Contact(   String^ accountId,
                    String^ name,
                    String^ ringID,
                    String^ GUID,
                    uint32 unreadmessages,
                    ContactStatus contactStatus,
                    TrustStatus trustStatus,
                    bool isIncognitoContact,
                    String^ avatarColorString)
{
    vCard_ = ref new VCardUtils::VCard(this, accountId);

    name_   = name;
    ringID_ = ringID;
    GUID_   = GUID;

    if (GUID_ == nullptr)
        GUID_ = Utils::GetNewGUID();

    conversation_ = ref new Conversation();
    create_task([this]() { loadConversation(); });

    notificationNewMessage_ = Windows::UI::Xaml::Visibility::Collapsed;
    unreadMessages_ = unreadmessages;

    if(unreadMessages_) {
        notificationNewMessage = Windows::UI::Xaml::Visibility::Visible;
        NotifyPropertyChanged("unreadMessages");
    }

    _accountIdAssociated = accountId;
    _vcardUID = "";
    _avatarImage = ref new String(L" ");
    _avatarColorString = avatarColorString;
    _displayName = "";

    contactStatus_ = contactStatus;
    trustStatus_ = trustStatus;

    unreadContactRequest_ = trustStatus == TrustStatus::INCOMING_CONTACT_REQUEST ? true : false;

    lastTime_ = ResourceMananger::instance->getStringResource("m_never_called_");

    _isIncognitoContact = isIncognitoContact;
    presenceStatus_ = -1;
    subscribed_ = false;
}

void
Contact::loadConversation()
{
    // load conversation from disk
    StorageFolder^ localfolder = ApplicationData::Current->LocalFolder;
    String^ messagesFile = localfolder->Path + "\\" + ".messages\\" + GUID_ + ".json";

    String^ fileContents = Utils::toPlatformString(Utils::getStringFromFile(Utils::toString(messagesFile)));

    CoreApplication::MainView->CoreWindow->Dispatcher->RunAsync(CoreDispatcherPriority::High,
        ref new DispatchedHandler([=]() {
        if (fileContents != nullptr)
            DestringifyConversation(fileContents);
    }));
}

void
Contact::NotifyPropertyChanged(String^ propertyName)
{
    CoreApplicationView^ view = CoreApplication::MainView;
    view->CoreWindow->Dispatcher->RunAsync(
        CoreDispatcherPriority::High,
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
    contactObject->SetNamedValue(displayNameKey, JsonValue::CreateStringValue(displayName_));
    contactObject->SetNamedValue(ringIDKey, JsonValue::CreateStringValue(ringID_));
    contactObject->SetNamedValue(GUIDKey, JsonValue::CreateStringValue(GUID_));
    contactObject->SetNamedValue(unreadMessagesKey, JsonValue::CreateNumberValue(unreadMessages_));
    contactObject->SetNamedValue(unreadContactRequestKey, JsonValue::CreateBooleanValue(unreadContactRequest_));
    contactObject->SetNamedValue(accountIdAssociatedKey, JsonValue::CreateStringValue(_accountIdAssociated));
    contactObject->SetNamedValue(vcardUIDKey, JsonValue::CreateStringValue(_vcardUID));
    contactObject->SetNamedValue(lastTimeKey, JsonValue::CreateStringValue(_lastTime));
    contactObject->SetNamedValue(trustStatusKey, JsonValue::CreateNumberValue(Utils::toUnderlyingValue(trustStatus_)));
    contactObject->SetNamedValue(isIncognitoContactKey, JsonValue::CreateBooleanValue(_isIncognitoContact));
    contactObject->SetNamedValue(avatarColorStringKey, JsonValue::CreateStringValue(avatarColorString_));

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
    bool        fromContact;
    String^     payload;
    std::time_t timeReceived;
    bool        isReceived;
    String^     messageIdStr;

    JsonArray^ messageList = jsonObject->GetNamedArray(conversationKey, ref new JsonArray());
    for (unsigned int i = 0; i < messageList->Size; i++) {
        IJsonValue^ message = messageList->GetAt(i);
        if (message->ValueType == JsonValueType::Object) {
            JsonObject^ jsonMessageObject = message->GetObject();
            JsonObject^ messageObject = jsonMessageObject->GetNamedObject(messageKey, nullptr);
            if (messageObject != nullptr) {
                if (messageObject->HasKey(fromContactKey))
                    fromContact = messageObject->GetNamedBoolean(fromContactKey);
                if (messageObject->HasKey(payloadKey))
                    payload = messageObject->GetNamedString(payloadKey);
                if (messageObject->HasKey(timeReceivedKey))
                    timeReceived = static_cast<std::time_t>(messageObject->GetNamedNumber(timeReceivedKey));
                if (messageObject->HasKey(isReceivedKey))
                    isReceived = messageObject->GetNamedBoolean(isReceivedKey);
                if (messageObject->HasKey(messageIdKey))
                    messageIdStr = messageObject->GetNamedString(messageIdKey);
            }
            conversation_->addMessage(fromContact, payload, timeReceived, isReceived, messageIdStr);
        }
    }
}

void
Contact::deleteConversationFile()
{
    StorageFolder^ localfolder = ApplicationData::Current->LocalFolder;
    String^ messagesFile = ".messages\\" + GUID_ + ".json";

    // refacto : Utils::fileExists fails here if the file doesn't exist, code below should replace fileExist everywhere
    create_task(localfolder->TryGetItemAsync(messagesFile)).then([](IStorageItem^ storageFile) {
        if (storageFile)
            storageFile->DeleteAsync();
    });

}

void
Contact::saveConversationToFile()
{
    StorageFolder^ localfolder = ApplicationData::Current->LocalFolder;
    String^ messagesFile = localfolder->Path + "\\" + ".messages\\" + GUID_ + ".json";

    if (_mkdir(Utils::toString(localfolder->Path + "\\" + ".messages\\").c_str())) {
        std::ofstream file(Utils::toString(messagesFile).c_str());
        if (file.is_open()) {
            file << Utils::toString(StringifyConversation());
            file.close();
        }
    }
}

VCardUtils::VCard^
Contact::getVCard()
{
    return vCard_;
}

void
Contact::raiseNotifyPropertyChanged(String^ propertyName)
{
    NotifyPropertyChanged(propertyName);
}

/***************************************************************************
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
#include "ContactListModel.h"

#include "fileutils.h"
#include "presencemanager_interface.h"

using namespace Windows::ApplicationModel::Core;
using namespace Windows::Data::Json;
using namespace Windows::Storage;
using namespace Windows::Storage::Streams;
using namespace Windows::UI::Core;

using namespace RingClientUWP;
using namespace ViewModel;

ContactListModel::ContactListModel(String^ account) : m_Owner(account)
{
    contactsList_ = ref new Vector<Contact^>();
    openContactsFromFile();

    /* connect delegates. */
    RingD::instance->registeredNameFound +=
        ref new RingClientUWP::RegisteredNameFound(this, &ContactListModel::OnregisteredNameFound);
}

Contact^ // refacto : remove "byName"
ContactListModel::findContactByName(String^ name)
{
    auto trimmedName = Utils::Trim(name);
    for each (Contact^ contact in contactsList_)
        if (contact->_name == trimmedName)
            return contact;

    return nullptr;
}

Contact^
ContactListModel::findContactByRingId(String^ ringId)
{
    for each (Contact^ contact in contactsList_)
        if (contact->ringID_ == ringId)
            return contact;

    return nullptr;
}

Contact^
ContactListModel::addNewContact(String^ name, String^ ringId, TrustStatus trustStatus, bool isIncognitoContact, ContactStatus contactStatus)
{
    auto trimmedName = Utils::Trim(name);
    if (contactsList_ && !findContactByName(trimmedName)) {
        String^ avatarColorString = Utils::getRandomColorString();
        if (auto acc = AccountsViewModel::instance->findItem(m_Owner)) {
            if (acc->accountType_ == "RING") {
                if (ringId)
                    avatarColorString = Utils::getRandomColorStringFromString(ringId);
                else
                    avatarColorString = Utils::getRandomColorStringFromString(name);
            }
            else if (name != "") {
                avatarColorString = Utils::getRandomColorStringFromString(name);
            }
        }
        Contact^ contact = ref new Contact(m_Owner, trimmedName, ringId, nullptr, 0, contactStatus, trustStatus, isIncognitoContact, avatarColorString);
        contactsList_->Append(contact);
        saveContactsToFile();
        AccountsViewModel::instance->raiseContactAdded(m_Owner, contact);
        return contact;
    }

    return nullptr;
}

void
ContactListModel::saveContactsToFile()
{
    StorageFolder^ localfolder = ApplicationData::Current->LocalFolder;
    String^ contactsFile = localfolder->Path + "\\" + ".profile\\" + m_Owner + "\\contacts.json";

    if (ring::fileutils::recursive_mkdir(Utils::toString(localfolder->Path + "\\" + ".profile\\" + m_Owner).c_str())) {
        std::ofstream file(Utils::toString(contactsFile).c_str());
        if (file.is_open())
        {
            file << Utils::toString(Stringify());
            file.close();
        }
    }
}

void
ContactListModel::openContactsFromFile()
{
    StorageFolder^ localfolder = ApplicationData::Current->LocalFolder;
    String^ contactsFile = localfolder->Path + "\\" + ".profile\\" + m_Owner + "\\contacts.json";

    String^ fileContents = Utils::toPlatformString(Utils::getStringFromFile(Utils::toString(contactsFile)));

    CoreApplication::MainView->CoreWindow->Dispatcher->RunAsync(CoreDispatcherPriority::High,
    ref new DispatchedHandler([=]() {
        if (fileContents != nullptr)
            Destringify(fileContents);
    }));
}

String^
ContactListModel::Stringify()
{
    JsonArray^ jsonArray = ref new JsonArray();

    for (int i = contactsList_->Size - 1; i >= 0; i--) {
        auto contact = contactsList_->GetAt(i);
        if (contact->_contactStatus != ContactStatus::WAITING_FOR_ACTIVATION)
            jsonArray->Append(contact->ToJsonObject());
    }

    JsonObject^ jsonObject = ref new JsonObject();
    jsonObject->SetNamedValue(contactListKey, jsonArray);

    return jsonObject->Stringify();
}

void
ContactListModel::Destringify(String^ data)
{
    JsonObject^     jsonObject = JsonObject::Parse(data);
    String^         name = "";
    String^         displayname = "";
    String^         ringid = "";
    String^         guid = "";
    uint32          unreadMessages = 0;
    String^         accountIdAssociated = "";
    String^         vcardUID = "";
    String^         lastTime = "";
    uint8           trustStatus = Utils::toUnderlyingValue(TrustStatus::TRUSTED);
    bool            unreadContactRequest = false;
    bool            isIncognitoContact = false;
    String^         avatarColorString;

    JsonArray^ contactlist = jsonObject->GetNamedArray(contactListKey, ref new JsonArray());
    for (int i = contactlist->Size - 1; i >= 0; i--) {
        IJsonValue^ contact = contactlist->GetAt(i);
        if (contact->ValueType == JsonValueType::Object) {
            JsonObject^ jsonContactObject = contact->GetObject();
            JsonObject^ contactObject = jsonContactObject->GetNamedObject(contactKey, nullptr);
            if (contactObject != nullptr) {
                if (contactObject->HasKey(nameKey))
                    name = contactObject->GetNamedString(nameKey);
                if (contactObject->HasKey(displayNameKey))
                    displayname = contactObject->GetNamedString(displayNameKey);
                if (contactObject->HasKey(ringIDKey))
                    ringid = contactObject->GetNamedString(ringIDKey);
                if (contactObject->HasKey(GUIDKey))
                    guid = contactObject->GetNamedString(GUIDKey);
                if (contactObject->HasKey(unreadMessagesKey))
                    unreadMessages = static_cast<uint32>(contactObject->GetNamedNumber(unreadMessagesKey));
                if (contactObject->HasKey(unreadContactRequestKey))
                    unreadContactRequest = contactObject->GetNamedBoolean(unreadContactRequestKey);
                if (contactObject->HasKey(accountIdAssociatedKey))
                    accountIdAssociated = contactObject->GetNamedString(accountIdAssociatedKey);
                if (contactObject->HasKey(vcardUIDKey))
                    vcardUID = contactObject->GetNamedString(vcardUIDKey);
                if (contactObject->HasKey(lastTimeKey))
                    lastTime = contactObject->GetNamedString(lastTimeKey);
                if (contactObject->HasKey(trustStatusKey))
                    trustStatus = static_cast<uint8>(contactObject->GetNamedNumber(trustStatusKey));
                if (contactObject->HasKey(isIncognitoContactKey))
                    isIncognitoContact = contactObject->GetNamedBoolean(isIncognitoContactKey);
                if (contactObject->HasKey(avatarColorStringKey)) {
                    auto oldColorString = contactObject->GetNamedString(avatarColorStringKey);
                    if (oldColorString != "") {
                        avatarColorString = oldColorString;
                    }
                    else {
                        if (auto acc = AccountsViewModel::instance->findItem(m_Owner)) {
                            if (acc->accountType_ == "RING") {
                                avatarColorString = Utils::getRandomColorStringFromString(ringid);
                            }
                            else if (name != "") {
                                avatarColorString = Utils::getRandomColorStringFromString(name);
                            }
                            else {
                                avatarColorString = Utils::getRandomColorString();
                            }
                        }
                        else
                            avatarColorString = Utils::getRandomColorString();
                    }
                }
            }
            auto contact = ref new Contact( m_Owner,
                                            name,
                                            ringid,
                                            guid,
                                            unreadMessages,
                                            ContactStatus::READY,
                                            Utils::toEnum<TrustStatus>(trustStatus),
                                            isIncognitoContact,
                                            avatarColorString);
            contact->_unreadContactRequest = unreadContactRequest;
            contact->_displayName = displayname;
            contact->_accountIdAssociated = accountIdAssociated;
            // contact image
            contact->_vcardUID = vcardUID;
            if (lastTime)
                contact->_lastTime = lastTime;

            std::string vcardDir = RingD::instance->getLocalFolder() + ".vcards\\";
            std::string pngFile = Utils::toString(contact->_vcardUID) + ".png";
            std::string contactImageFile = vcardDir + pngFile;
            if (Utils::fileExists(contactImageFile)) {
                //RingClientUWP::ResourceMananger::instance->preloadImage(Utils::toPlatformString(contactImageFile));
                contact->_avatarImage = Utils::toPlatformString(contactImageFile);
            }
            contactsList_->Append(contact);
            AccountsViewModel::instance->raiseContactAdded(m_Owner, contact);
        }
    }
}

void
ContactListModel::deleteContact(Contact ^ contact)
{
    unsigned int index;

    if (contactsList_->IndexOf(contact, &index)) {
        contact->deleteConversationFile();
        RingD::instance->removeContact(
            Utils::toString(contact->_accountIdAssociated),
            Utils::toString(contact->ringID_));
        contactsList_->RemoveAt(index);
    }

    saveContactsToFile();
}

void
ContactListModel::modifyContact(Contact^ contact)
{
    AccountsViewModel::instance->raiseContactDataModified(m_Owner, contact);
}

void
ContactListModel::OnregisteredNameFound(RingClientUWP::LookupStatus status,  const std::string& accountId, const std::string &address, const std::string &name)
{
    if (status == LookupStatus::SUCCESS) {
        for each (Contact^ contact in contactsList_) {
            if (contact->ringID_ == Utils::toPlatformString(address)) {
                contact->_name = Utils::toPlatformString(name);
                saveContactsToFile();
            }
        }
    }
}

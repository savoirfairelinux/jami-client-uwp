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
ContactListModel::addNewContact(String^ name, String^ ringId, ContactStatus contactStatus)
{
    auto trimmedName = Utils::Trim(name);
    if (contactsList_ && !findContactByName(trimmedName)) {
        //if (contactsList_ && !findContactByName(trimmedName) && !findContactByRingId(ringId)) {
        Contact^ contact = ref new Contact(m_Owner, trimmedName, ringId, nullptr, 0, contactStatus);
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
        jsonArray->Append(contactsList_->GetAt(i)->ToJsonObject());
    }

    JsonObject^ jsonObject = ref new JsonObject();
    jsonObject->SetNamedValue(contactListKey, jsonArray);

    return jsonObject->Stringify();
}

void
ContactListModel::Destringify(String^ data)
{
    JsonObject^     jsonObject = JsonObject::Parse(data);
    String^         name;
    String^         displayname;
    String^         ringid;
    String^         guid;
    unsigned int    unreadmessages;
    String^			accountIdAssociated;
    String^         vcardUID;
    String^			lastTime;

    JsonArray^ contactlist = jsonObject->GetNamedArray(contactListKey, ref new JsonArray());
    for (int i = contactlist->Size - 1; i >= 0; i--) {
        IJsonValue^ contact = contactlist->GetAt(i);
        if (contact->ValueType == JsonValueType::Object) {
            JsonObject^ jsonContactObject = contact->GetObject();
            JsonObject^ contactObject = jsonContactObject->GetNamedObject(contactKey, nullptr);
            if (contactObject != nullptr) {
                name = contactObject->GetNamedString(nameKey);
                displayname = contactObject->GetNamedString(displayNameKey);
                ringid = contactObject->GetNamedString(ringIDKey);
                guid = contactObject->GetNamedString(GUIDKey);
                unreadmessages = static_cast<uint16_t>(contactObject->GetNamedNumber(unreadMessagesKey));
                accountIdAssociated = contactObject->GetNamedString(accountIdAssociatedKey);
                vcardUID = contactObject->GetNamedString(vcardUIDKey);

                if (contactObject->HasKey(lastTimeKey))
                    lastTime = contactObject->GetNamedString(lastTimeKey);
            }
            auto contact = ref new Contact(m_Owner, name, ringid, guid, unreadmessages, ContactStatus::READY);
            contact->_displayName = displayname;
            contact->_accountIdAssociated = accountIdAssociated;
            // contact image
            contact->_vcardUID = vcardUID;
            if (lastTime)
                contact->_lastTime = lastTime;

            std::string contactImageFile = RingD::instance->getLocalFolder() + ".vcards\\"
                                           + Utils::toString(contact->_vcardUID) + ".png";
            if (Utils::fileExists(contactImageFile)) {
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
    auto itemsList = SmartPanelItemsViewModel::instance->itemsList;
    auto item = SmartPanelItemsViewModel::instance->_selectedItem;

    if (contactsList_->IndexOf(contact, &index)) {
        contact->deleteConversationFile();
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
ContactListModel::OnregisteredNameFound(RingClientUWP::LookupStatus status, const std::string &address, const std::string &name)
{
    if (status == LookupStatus::SUCCESS) {
        for each (Contact^ contact in contactsList_)
            if (contact->ringID_ == Utils::toPlatformString(address)) {
                contact->_name = Utils::toPlatformString(name);
                saveContactsToFile();
            }
    }
}

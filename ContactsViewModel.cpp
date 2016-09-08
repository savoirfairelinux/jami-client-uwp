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

#include "ContactsViewModel.h"

using namespace Windows::ApplicationModel::Core;
using namespace Windows::Data::Json;
using namespace Windows::Storage;
using namespace Windows::Storage::Streams;
using namespace Windows::UI::Core;


using namespace RingClientUWP;
using namespace ViewModel;

ContactsViewModel::ContactsViewModel()
{
    contactsList_ = ref new Vector<Contact^>();
    openContactsFromFile();

    /* connect delegates. */
    RingD::instance->incomingAccountMessage += ref new IncomingAccountMessage([&](String^ accountId,
    String^ from, String^ payload) {
        auto contact = findContactByName(from);

        if (contact == nullptr)
            contact = addNewContact(from, from); // contact checked inside addNewContact.

        bool isNotSelected = (contact != ContactsViewModel::instance->selectedContact) ? true : false;

        if (contact == nullptr) {
            ERR_("contact not handled!");
            return;
        }

        contact->_conversation->addMessage(""/* date not yet used*/, MSG_FROM_CONTACT, payload);

        /* save contacts conversation to disk */
        contact->saveConversationToFile();

        if (contact->ringID_ == from && isNotSelected) {
            // increment contact's unread message count
            contact->addNotifyNewConversationMessage();
            // update the xaml for all contacts
            notifyNewConversationMessage();
            // save to disk
            saveContactsToFile();
        }
    });
}

Contact^ // refacto : remove "byName"
ContactsViewModel::findContactByName(String^ name)
{
    for each (Contact^ contact in contactsList_)
        if (contact->name_ == name)
            return contact;

    return nullptr;
}

Contact^
ContactsViewModel::addNewContact(String^ name, String^ ringId)
{
    auto trimedName = Utils::Trim(name);
    if (contactsList_ && !findContactByName(trimedName)) {
        Contact^ contact = ref new Contact(trimedName, trimedName, nullptr, 0);
        contactsList_->Append(contact);
        saveContactsToFile();
        contactAdded(contact);
        return contact;
    }

    return nullptr;
}

void
ContactsViewModel::saveContactsToFile()
{
    StorageFolder^ localfolder = ApplicationData::Current->LocalFolder;
    String^ contactsFile = ".profile\\contacts.json";

    try {
        create_task(localfolder->CreateFileAsync(contactsFile
                    , Windows::Storage::CreationCollisionOption::ReplaceExisting))
        .then([&](StorageFile^ newFile) {
            try {
                FileIO::WriteTextAsync(newFile, Stringify());
            }
            catch (Exception^ e) {
                RingDebug::instance->print("Exception while writing to contacts file");
            }
        });
    }
    catch (Exception^ e) {
        RingDebug::instance->print("Exception while opening contacts file");
    }
}

void
ContactsViewModel::openContactsFromFile()
{
    String^ contactsFile = ".profile\\contacts.json";

    Utils::fileExists(ApplicationData::Current->LocalFolder,
                      contactsFile)
    .then([this,contactsFile](bool contacts_file_exists)
    {
        if (contacts_file_exists) {
            try {
                create_task(ApplicationData::Current->LocalFolder->GetFileAsync(contactsFile))
                .then([this](StorageFile^ file)
                {
                    create_task(FileIO::ReadTextAsync(file))
                    .then([this](String^ fileContents) {
                        if (fileContents != nullptr)
                            Destringify(fileContents);
                    });
                });
            }
            catch (Exception^ e) {
                RingDebug::instance->print("Exception while opening contacts file");
            }
        }
    });
}

String^
ContactsViewModel::Stringify()
{
    JsonArray^ jsonArray = ref new JsonArray();

    for (unsigned int i = 0; i < contactsList_->Size; i++) {
        jsonArray->Append(contactsList_->GetAt(i)->ToJsonObject());
    }

    JsonObject^ jsonObject = ref new JsonObject();
    jsonObject->SetNamedValue(contactListKey, jsonArray);

    return jsonObject->Stringify();
}

void
ContactsViewModel::Destringify(String^ data)
{
    JsonObject^     jsonObject = JsonObject::Parse(data);
    String^         name;
    String^         ringid;
    String^         guid;
    unsigned int    unreadmessages;

    JsonArray^ contactlist = jsonObject->GetNamedArray(contactListKey, ref new JsonArray());
    for (unsigned int i = 0; i < contactlist->Size; i++) {
        IJsonValue^ contact = contactlist->GetAt(i);
        if (contact->ValueType == JsonValueType::Object) {
            JsonObject^ jsonContactObject = contact->GetObject();
            JsonObject^ contactObject = jsonContactObject->GetNamedObject(contactKey, nullptr);
            if (contactObject != nullptr) {
                name = contactObject->GetNamedString(nameKey);
                ringid = contactObject->GetNamedString(ringIDKey);
                guid = contactObject->GetNamedString(GUIDKey);
                unreadmessages = static_cast<uint16_t>(contactObject->GetNamedNumber(unreadMessagesKey));
            }
            auto contact = ref new Contact(name, ringid, guid, unreadmessages);
            contactsList_->Append(contact);
            contactAdded(contact);
        }
    }
}

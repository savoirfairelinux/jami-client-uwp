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

#include "fileutils.h"

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
    String^ fromRingId, String^ payload) {
        auto contact = findContactByName(fromRingId);

        if (contact == nullptr)
            contact = addNewContact(fromRingId, fromRingId); // contact checked inside addNewContact.

        auto item = SmartPanelItemsViewModel::instance->_selectedItem;

        if (contact == nullptr) {
            ERR_("contact not handled!");
            return;
        }

        contact->_conversation->addMessage(""/* date not yet used*/, MSG_FROM_CONTACT, payload);

        /* save contacts conversation to disk */
        contact->saveConversationToFile();


        auto selectedContact = (item) ? item->_contact : nullptr;

        if (contact->ringID_ == fromRingId && contact != selectedContact) {
            contact->_unreadMessages++;
            /* saveContactsToFile used to save the notification */
            saveContactsToFile();
        }
    });
    RingD::instance->incomingMessage += ref new RingClientUWP::IncomingMessage(this, &RingClientUWP::ViewModel::ContactsViewModel::OnincomingMessage);
}

Contact^ // refacto : remove "byName"
ContactsViewModel::findContactByName(String^ name)
{
    auto trimmedName = Utils::Trim(name);
    for each (Contact^ contact in contactsList_)
        if (contact->name_ == trimmedName)
            return contact;

    return nullptr;
}

Contact^
ContactsViewModel::addNewContact(String^ name, String^ ringId)
{
    auto trimmedName = Utils::Trim(name);
    if (contactsList_ && !findContactByName(trimmedName)) {
        Contact^ contact = ref new Contact(trimmedName, trimmedName, nullptr, 0);
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
    String^ contactsFile = localfolder->Path + "\\" + ".profile\\contacts.json";

    if (ring::fileutils::recursive_mkdir(Utils::toString(localfolder->Path + "\\" + ".profile\\").c_str())) {
        std::ofstream file(Utils::toString(contactsFile).c_str());
        if (file.is_open())
        {
            file << Utils::toString(Stringify());
            file.close();
        }
    }
}

void
ContactsViewModel::openContactsFromFile()
{
    StorageFolder^ localfolder = ApplicationData::Current->LocalFolder;
    String^ contactsFile = localfolder->Path + "\\" + ".profile\\contacts.json";

    String^ fileContents = Utils::toPlatformString(Utils::getStringFromFile(Utils::toString(contactsFile)));

    CoreApplication::MainView->CoreWindow->Dispatcher->RunAsync(CoreDispatcherPriority::High,
    ref new DispatchedHandler([=]() {
        if (fileContents != nullptr)
            Destringify(fileContents);
    }));
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
    String^			accountIdAssociated;

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
                accountIdAssociated = contactObject->GetNamedString(accountIdAssociatedKey);

            }
            auto contact = ref new Contact(name, ringid, guid, unreadmessages);
            contact->_accountIdAssociated = accountIdAssociated;
            contactsList_->Append(contact);
            contactAdded(contact);
        }
    }
}


void RingClientUWP::ViewModel::ContactsViewModel::OnincomingMessage(Platform::String ^callId, Platform::String ^payload)
{
    auto itemlist = SmartPanelItemsViewModel::instance->itemsList;
    auto item = SmartPanelItemsViewModel::instance->findItem(callId);
    auto contact = item->_contact;


    /* the contact HAS TO BE already registered */
    if (contact) {
        auto item = SmartPanelItemsViewModel::instance->_selectedItem;

        contact->_conversation->addMessage(""/* date not yet used*/, MSG_FROM_CONTACT, payload);

        /* save contacts conversation to disk */
        contact->saveConversationToFile();

        auto selectedContact = (item) ? item->_contact : nullptr;

        if (contact != selectedContact) {
            contact->_unreadMessages++;
            /* saveContactsToFile used to save the notification */
            saveContactsToFile();
        }
    }
}

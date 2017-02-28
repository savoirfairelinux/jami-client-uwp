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
#pragma once

using namespace Platform::Collections;
using namespace Concurrency;

namespace RingClientUWP
{
namespace ViewModel
{

public ref class ContactListModel sealed
{
public:
    ContactListModel(String^ m_Owner);

internal:
    /* functions */
    Contact^    findContactByName(String^ name);
    Contact^    findContactByRingId(String^ ringId);
    Contact^    addNewContact(  String^ name,
                                String^ ringId,
                                TrustStatus trustStatus,
                                bool isIncognitoContact,
                                ContactStatus contactStatus = ContactStatus::READY);
    void        saveContactsToFile();
    void        openContactsFromFile();
    String^     Stringify();
    void        Destringify(String^ data);
    void        deleteContact(Contact^ contact);
    void        modifyContact(Contact^ contact);

    /* properties */
    property IVector<Contact^>^ _contactsList {
        IVector<Contact^>^ get() {
            return contactsList_;
        }
        void set(IVector<Contact^>^ value) {
            contactsList_ = value;
        }
    }

private:
    IVector<Contact^>^ contactsList_;
    Contact^ currentItem_;
    Contact^ oldItem_;
    String^ m_Owner;

    void OnregisteredNameFound(RingClientUWP::LookupStatus status, const std::string& accountId, const std::string &address, const std::string &name);
};
}
}

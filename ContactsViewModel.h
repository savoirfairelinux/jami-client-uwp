#pragma once
/**************************************************************************
* Copyright (C) 2016 by Savoir-faire Linux                                *
* Author: Jäger Nicolas <nicolas.jager@savoirfairelinux.com>              *
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
using namespace Concurrency;
using namespace Platform::Collections;

namespace RingClientUWP
{

delegate void NewContactSelected();
delegate void NoContactSelected();

namespace ViewModel {
public ref class ContactsViewModel sealed
{
internal:
    /* singleton */
    static property ContactsViewModel^ instance
    {
        ContactsViewModel^ get()
        {
            static ContactsViewModel^ instance_ = ref new ContactsViewModel();
            return instance_;
        }
    }

    /* functions */
    Contact^    findContactByName(String^ name);
    Contact^    addNewContact(String^ name, String^ ringId);
    void        saveContactsToFile();
    void        openContactsFromFile();
    String^     ContactsViewModel::Stringify();
    void        ContactsViewModel::Destringify(String^ data);

    /* properties */
    property Contact^ selectedContact
    {
        Contact^ get()
        {
            return currentItem_;
        }
        void set(Contact^ value)
        {
            oldItem_ = currentItem_;
            currentItem_ = value;
            if (value)
                newContactSelected();
            else
                noContactSelected();
        }
    }

    property Vector<Contact^>^ contactsList
    {
        Vector<Contact^>^ get()
        {
            return contactsList_;
        }
    }

    /* events */
    event NewContactSelected^ newContactSelected;
    event NoContactSelected^ noContactSelected;

private:
    ContactsViewModel(); // singleton
    Vector<Contact^>^ contactsList_;
    Contact^ currentItem_;
    Contact^ oldItem_;

};
}
}

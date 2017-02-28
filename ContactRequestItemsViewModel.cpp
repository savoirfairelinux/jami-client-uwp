/**************************************************************************
* Copyright (C) 2016 by Savoir-faire Linux                                *
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

#include "ContactRequestItemsViewModel.h"

using namespace Windows::ApplicationModel::Core;
using namespace Windows::Data::Json;
using namespace Windows::Storage;
using namespace Windows::Storage::Streams;
using namespace Windows::UI::Core;
using namespace Windows::Globalization::DateTimeFormatting;


using namespace RingClientUWP;
using namespace ViewModel;

ContactRequestItemsViewModel::ContactRequestItemsViewModel()
{
    itemsList_ = ref new Vector<ContactRequestItem^>();
    itemsListFiltered_ = ref new Vector<ContactRequestItem^>();
}

ContactRequestItem^
ContactRequestItemsViewModel::findItem(Contact^ contact)
{
    for each (ContactRequestItem^ item in itemsList)
        if (item->_contact == contact)
            return item;

    return nullptr;
}

unsigned int
ContactRequestItemsViewModel::getIndex(Contact^ contact)
{
    unsigned int i;
    for (i = 0; i < itemsList_->Size; i++) {
        if (itemsList_->GetAt(i)->_contact == contact)
            break;
    }
    return i;
}

void
ContactRequestItemsViewModel::removeItem(ContactRequestItem ^ item)
{
    unsigned int index;

    if (itemsList->IndexOf(item, &index)) {
        itemsList->RemoveAt(index);

        // If the same item exists in the filtered list, remove it as well.
        // Maintaining both lists in tandem, reduces flickering caused by a complete
        // refresh of the filtered list. This is a pretty alternative to the more
        // elegant solution of XAML visibility filtration through IValueConverter object
        // bindings, which causes a buggy scrollbar to appear when the collapsed list items
        // are before the visible ones.
        if (itemsListFiltered->IndexOf(item, &index)) {
            itemsListFiltered->RemoveAt(index);
        }
    }
}

String^
ContactRequestItemsViewModel::getAssociatedAccountId(ContactRequestItem^ item)
{
    if (item->_contact->_accountIdAssociated->IsEmpty())
        return AccountListItemsViewModel::instance->_selectedItem->_account->accountID_;
    else
        return item->_contact->_accountIdAssociated;
}

void
ContactRequestItemsViewModel::update(const std::vector<std::string>& properties)
{
    for each (ContactRequestItem^ item in itemsList)
        for each (std::string prop in properties)
            item->raiseNotifyPropertyChanged(Utils::toPlatformString(prop));
}

void
ContactRequestItemsViewModel::refreshFilteredItemsList()
{
    auto selectedAccountId = AccountListItemsViewModel::instance->getSelectedAccountId();

    std::for_each(begin(itemsList), end(itemsList),
        [selectedAccountId, this](ContactRequestItem^ item) {
        static unsigned spi_index;
        auto isInList = itemsListFiltered->IndexOf(item, &spi_index);

        if ((item->_contact->_accountIdAssociated == selectedAccountId) &&
            item->_contact->_trustStatus == TrustStatus::INCOMING_CONTACT_REQUEST) {
            if (!isInList) {
                    itemsListFiltered->InsertAt(0, item);
            }
        }
        else if (isInList) {
            itemsListFiltered->RemoveAt(spi_index);
        }
    });
}
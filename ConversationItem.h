/**************************************************************************
* Copyright (C) 2017 by Savoir-faire Linux                                *
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

using namespace Platform;
using namespace Windows::Data::Json;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Data;

#include <chrono>

#include "Call.h"
#include "AccountItem.h"
#include "Globals.h"

namespace RingClientUWP
{

namespace Controls
{

public ref class ConversationItem sealed
    : public INotifyPropertyChanged
{
protected:
    void NotifyPropertyChanged(String^ propertyName);
public:
    virtual event PropertyChangedEventHandler^ PropertyChanged;

    property String^ _uri {
        String^ get() {
            if (auto contact = As<ContactItem^>())
                return contact->_uri;
            return nullptr;
        }
    }

    property String^ _displayName {
        String^ get() {
            if (auto contact = As<ContactItem^>())
                return contact->_displayName;
            return nullptr;
        }
        void set(String^ value) {
            if (auto contact = As<ContactItem^>()) {
                contact->_displayName = value;
                NotifyPropertyChanged("_displayName");
            }
        }
    }

    property String^ _registeredName {
        String^ get() {
            if (auto contact = As<ContactItem^>())
                return contact->_registeredName;
            return nullptr;
        }
        void set(String^ value) {
            if (auto contact = As<ContactItem^>()) {
                contact->_registeredName = value;
                NotifyPropertyChanged("_registeredName");
            }
        }
    }

    property String^ _alias {
        String^ get() {
            if (auto contact = As<ContactItem^>())
                return contact->_alias;
            return nullptr;
        }
        void set(String^ value) {
            if (auto contact = As<ContactItem^>()) {
                contact->_alias = value;
                NotifyPropertyChanged("_alias");
            }
        }
    }

    property String^ _isTrusted {
        String^ get() {
            if (auto contact = As<ContactItem^>())
                return contact->_isTrusted;
            return nullptr;
        }
        void set(String^ value) {
            if (auto contact = As<ContactItem^>()) {
                contact->_isTrusted = value;
                NotifyPropertyChanged("_isTrusted");
            }
        }
    }

    property String^ _type {
        String^ get() {
            if (auto contact = As<ContactItem^>())
                return contact->_type;
            return nullptr;
        }
        void set(String^ value) {
            if (auto contact = As<ContactItem^>()) {
                contact->_type = value;
                NotifyPropertyChanged("_type");
            }
        }
    }

    property String^ _bestName {
        String^ get() {
            String^ bestName;
            if (_displayName)
                bestName += _displayName + " - ";
            if (_registeredName)
                bestName += _registeredName;
            else if (_uri)
                bestName += _uri;
            return bestName;
        }
    }

    property String^ _bestName2 {
        String^ get() {
            String^ bestName;
            if (_displayName)
                bestName = _displayName;
            else if (_registeredName)
                bestName = _registeredName;
            else if (_uri)
                bestName = _uri;
            return bestName;
        }
    }

    //////////////////////////////
    //
    // Common properties
    //
    //////////////////////////////
    property Visibility _isVisible {
        Visibility get() { return isVisible_; }
        void set(Visibility value) {
            isVisible_ = value;
            NotifyPropertyChanged("_isVisible");
        }
    }

    property bool _isSelected {
        bool get() { return isSelected_; }
        void set(bool value) {
            isSelected_ = value;
            NotifyPropertyChanged("_isSelected");
        }
    }

    property bool _isHovered {
        bool get() { return isHovered_; }
        void set(bool value) {
            isHovered_ = value;
            NotifyPropertyChanged("_isHovered");
        }
    }

    property CallStatus _callStatus {
        CallStatus get() { return callStatus_; }
        void set(CallStatus value) {
            callStatus_ = value;
            NotifyPropertyChanged("_callStatus");
        }
    }

    property int _presenceStatus {
        int get() { return presenceStatus_; }
        void set(int value) {
            presenceStatus_ = value;
            NotifyPropertyChanged("_presenceStatus");
        }
    }

    property uint32 _unreadMessages {
        uint32 get() { return unreadMessages_; }
        void set(uint32 value) {
            unreadMessages_ = value;
            NotifyPropertyChanged("_unreadMessages");
        }
    }

    property uint32 _unreadContactRequests {
        uint32 get() { return unreadContactRequests_; }
        void set(uint32 value) {
            unreadContactRequests_ = value;
            NotifyPropertyChanged("_unreadContactRequests");
        }
    }

    property String^ _avatarImage {
        String^ get() { return avatarImage_; }
        void set(String^ value) {
            avatarImage_ = value;
            NotifyPropertyChanged("_avatarImage");
        }
    }

    property ContactStatus _contactStatus {
        ContactStatus get() { return contactStatus_; }
        void set(ContactStatus value) {
            contactStatus_ = value;
            NotifyPropertyChanged("_contactStatus");
        }
    }

    property String^ _lastTime {
        String^ get() { return lastTime_; }
        void set(String^ value) {
            lastTime_ = value;
            NotifyPropertyChanged("_lastTime");
        }
    }

    property String^ _callId {
        String^ get() { return callId_; }
        void set(String^ value) {
            callId_ = value;
            NotifyPropertyChanged("_callId");
        }
    }

    property bool _isCallable {
        bool get() {
            return ((callStatus_ == CallStatus::ENDED ||
                callStatus_ == CallStatus::NONE)
                && isHovered_) ? true : false;
        }
    }

    property bool _isBanned {
        bool get() {
            return isBanned_;
        }
        void set(bool value) {
            isBanned_ = value;
            NotifyPropertyChanged("_isBanned");
        }
    }

    Platform::Type^
        getItemType() {
        return itemBase_->GetType();
    }

internal:
    ConversationItem() {};
    ConversationItem(Controls::ContactItem^ contact)
        :itemBase_(contact)
    {};

private:
    template<typename T>
    T As() {
        return dynamic_cast<T>(itemBase_);
    }

    Platform::Object^       itemBase_;

    String^                 uid_;
    AccountItem^            account_;
    Vector<ContactItem^>    participants_;
    Conversation^           conversation_;

    Visibility              isVisible_;
    bool                    isSelected_;
    bool                    isHovered_;
    CallStatus              callStatus_;

    int                     presenceStatus_;
    uint32                  unreadMessages_;
    uint32                  unreadContactRequests_;
    String^                 avatarImage_;
    ContactStatus           contactStatus_;
    String^                 lastTime_;
    String^                 callId_;
    bool                    isBanned_;
};

} /*namespace Controls*/
} /*namespce RingClientUWP*/
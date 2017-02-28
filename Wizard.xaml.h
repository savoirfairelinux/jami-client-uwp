/**************************************************************************
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

#include "Wizard.g.h"

namespace RingClientUWP
{
namespace Views
{

public ref class Wizard sealed
{
public:
    Wizard();

protected:
    virtual void OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e) override;

private:
    void _createAccountYes__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    void _showCreateAccountMenuBtn__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    void _showAddAccountMenuBtn__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    void _avatarWebcamCaptureBtn__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    void _addAccountYes__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    void _usernameTextBox__KeyUp(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e);
    void _RegisterState__Toggled(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    void _fullnameTextBox__KeyUp(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e);
    void collapseMenus(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    void OnregisteredNameFound(LookupStatus status,  const std::string& accountId, const std::string& address, const std::string& name);
    void _password__PasswordChanged(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    void _passwordCheck__PasswordChanged(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    void checkState();

    /*members*/
    bool isPasswordValid;
    bool isPasswordsMatching;
    bool isPublic = true;
    bool isUsernameValid; // available
    bool isFullNameValid;
    void OnregistrationStateErrorGeneric(const std::string &accountId);
    void _PINTextBox__GotFocus(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    void _ArchivePassword__KeyUp(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e);
    void _PINTextBox__KeyUp(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e);
    void _createAccountNo__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
};

}
}
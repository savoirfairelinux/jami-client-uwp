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

#include "pch.h"
#include <direct.h>
#include "Wizard.xaml.h"
#include "MainPage.xaml.h"

using namespace RingClientUWP::Views;

using namespace Concurrency;
using namespace Platform;
using namespace Windows::Devices::Enumeration;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;
using namespace Windows::Media::Capture;
using namespace Windows::Storage;
using namespace Windows::UI::Xaml::Media::Imaging;
using namespace Windows::UI::Xaml::Shapes;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::UI::Core;

Wizard::Wizard()
{
    InitializeComponent();
    /* connect to delegates */
    RingD::instance->registeredNameFound += ref new RingClientUWP::RegisteredNameFound(this, &RingClientUWP::Views::Wizard::OnregisteredNameFound);
    RingD::instance->registrationStateErrorGeneric += ref new RingClientUWP::RegistrationStateErrorGeneric(this, &RingClientUWP::Views::Wizard::OnregistrationStateErrorGeneric);
}

void RingClientUWP::Views::Wizard::OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs ^ e)
{
    if (!RingD::instance->daemonRunning) {
        RingD::instance->init();
    }

    collapseMenus(nullptr, nullptr);
    _password_->Password = "";
    _passwordCheck_->Password = "";
    _ArchivePassword_->Password = "";
    _PINTextBox_->Text = "";
    _fullnameTextBox_->Text = "";
    _usernameTextBox_->Text = "";
    _RegisterState_->IsOn = true;

    RingD::instance->isInWizard = true;
}

void
Wizard::_createAccountYes__Click(Object^ sender, RoutedEventArgs^ e)
{
    this->Dispatcher->RunAsync(Windows::UI::Core::CoreDispatcherPriority::High, ref new Windows::UI::Core::DispatchedHandler([this] () {
        RingD::instance->isInWizard = false;
        this->Frame->Navigate(Windows::UI::Xaml::Interop::TypeName(RingClientUWP::MainPage::typeid));
        RingD::instance->createRINGAccount(_fullnameTextBox_->Text
                                           , _password_->Password
                                           , true // upnp by default set to true
                                           , (_RegisterState_->IsOn)? _usernameTextBox_->Text : "");
    }));
}

void
Wizard::_showCreateAccountMenuBtn__Click(Object^ sender, RoutedEventArgs^ e)
{
    _addStack_->Visibility = VIS::Collapsed;
    _createStack_->Margin = Windows::UI::Xaml::Thickness(0.0, 0.0, 0.0, 50.0);

    _accountAddMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _showAddAccountMenuTitle_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _showAddAccountMenuBtn_->Visibility = Windows::UI::Xaml::Visibility::Visible;

    _accountCreationMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Visible;
    _showCreateAccountMenuTitle_->Visibility = Windows::UI::Xaml::Visibility::Visible;
    _showCreateAccountMenuBtn_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
}

void
Wizard::_showAddAccountMenuBtn__Click(Object^ sender, RoutedEventArgs^ e)
{
    _createStack_->Visibility = VIS::Collapsed;
    _addStack_->Margin = Windows::UI::Xaml::Thickness(0.0, 0.0, 0.0, 50.0);

    _accountCreationMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _showCreateAccountMenuTitle_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _showCreateAccountMenuBtn_->Visibility = Windows::UI::Xaml::Visibility::Visible;

    _accountAddMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Visible;
    _showAddAccountMenuTitle_->Visibility = Windows::UI::Xaml::Visibility::Visible;
    _showAddAccountMenuBtn_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
}

void
Wizard::_avatarWebcamCaptureBtn__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    create_task(Configuration::getProfileImageAsync()).then([&](task<BitmapImage^> image){
        try {
            if (auto bitmapImage = image.get()) {
                auto brush = ref new ImageBrush();
                auto circle = ref new Ellipse();
                circle->Height = 100;
                circle->Width = 100;
                brush->ImageSource = bitmapImage;
                circle->Fill = brush;
                _avatarWebcamCaptureBtn_->Content = circle;
            }
        }
        catch (Platform::Exception^ e) {
            EXC_(e);
        }
    });
}

void RingClientUWP::Views::Wizard::_addAccountYes__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    if (_PINTextBox_->Text->IsEmpty() || _ArchivePassword_->Password->IsEmpty()) {
        _addAccountYes_->IsEnabled = false;
        return;
    }

    RingD::instance->_startingStatus = StartingStatus::REGISTERING_THIS_DEVICE;

    this->Dispatcher->RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal, ref new Windows::UI::Core::DispatchedHandler([this]() {
        this->Frame->Navigate(Windows::UI::Xaml::Interop::TypeName(RingClientUWP::MainPage::typeid));
        RingD::instance->registerThisDevice(_PINTextBox_->Text, _ArchivePassword_->Password);
        _ArchivePassword_->Password = "";
        _PINTextBox_->Text = "";
    }));
}

void RingClientUWP::Views::Wizard::_password__PasswordChanged(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    isPasswordValid = (_password_->Password->Length() > 0)
                      ? true : false;

    if (isPasswordValid) {
        _createAccountYes_->IsEnabled = true;
        _passwordValid_->Visibility = Windows::UI::Xaml::Visibility::Visible;
        _passwordInvalid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    } else {
        _createAccountYes_->IsEnabled = false;
        _passwordValid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        _passwordInvalid_->Visibility = Windows::UI::Xaml::Visibility::Visible;
    }

    isPasswordsMatching = (_password_->Password
                           == _passwordCheck_->Password
                           && _password_->Password->Length() > 0)
                          ? true : false;

    if (isPasswordsMatching) {
        _passwordCheckValid_->Visibility = Windows::UI::Xaml::Visibility::Visible;
        _passwordCheckInvalid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    }
    else {
        _passwordCheckValid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        _passwordCheckInvalid_->Visibility = Windows::UI::Xaml::Visibility::Visible;
    }

    checkState();
}

void RingClientUWP::Views::Wizard::_passwordCheck__PasswordChanged(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    isPasswordsMatching = (_password_->Password
                           == _passwordCheck_->Password
                           && _password_->Password->Length() > 0)
                          ? true : false;

    if (isPasswordsMatching) {
        _passwordCheckValid_->Visibility = Windows::UI::Xaml::Visibility::Visible;
        _passwordCheckInvalid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    }
    else {
        _passwordCheckValid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        _passwordCheckInvalid_->Visibility = Windows::UI::Xaml::Visibility::Visible;
    }

    checkState();
}

void RingClientUWP::Views::Wizard::checkState()
{
    if ((isPublic && isPasswordValid && isPasswordsMatching && isUsernameValid && isFullNameValid)
            ||(!isPublic && isPasswordValid && isPasswordsMatching && isFullNameValid))
        _createAccountYes_->IsEnabled = true;
    else
        _createAccountYes_->IsEnabled = false;
}

void RingClientUWP::Views::Wizard::_usernameTextBox__KeyUp(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e)
{
    auto alias = dynamic_cast<TextBox^>(sender)->Text;

    if (alias->IsEmpty()) {
        _usernameValid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        _usernameInvalid_->Visibility = Windows::UI::Xaml::Visibility::Visible;
    } else {
        auto accountId = ViewModel::AccountListItemsViewModel::instance->getSelectedAccountId();
        RingD::instance->lookUpName(Utils::toString(accountId), alias);
        _usernameValid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        _usernameInvalid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    }
}

void RingClientUWP::Views::Wizard::_RegisterState__Toggled(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    auto toggleSwitch =  dynamic_cast<ToggleSwitch^>(sender);

    if (_usernameTextBox_ == nullptr) // avoid trouble when InitializeComponent is called for Wizard.
        return;

    isPublic = toggleSwitch->IsOn;

    if (isPublic) {
        _usernameTextBox_->IsEnabled = true;
        auto loader = ref new Windows::ApplicationModel::Resources::ResourceLoader();
        _whatWillHappen_->Text = loader->GetString("_whatWillHappenEditionUid_.Text");
    } else {
        _usernameTextBox_->IsEnabled = false;
        auto loader = ref new Windows::ApplicationModel::Resources::ResourceLoader();
        _whatWillHappen_->Text = loader->GetString("_whatWillHappen_2_");
    }

    checkState();
}

void RingClientUWP::Views::Wizard::_fullnameTextBox__KeyUp(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e)
{
    isFullNameValid = !_fullnameTextBox_->Text->IsEmpty();

    if (isFullNameValid) {
        _fullnameValid_->Visibility = Windows::UI::Xaml::Visibility::Visible;
        _fullnameInvalid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    } else {
        _fullnameValid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        _fullnameInvalid_->Visibility = Windows::UI::Xaml::Visibility::Visible;
    }

    checkState();
}

void RingClientUWP::Views::Wizard::collapseMenus(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    _createStack_->Visibility = VIS::Visible;
    _addStack_->Visibility = VIS::Visible;

    _createStack_->Margin = Windows::UI::Xaml::Thickness(0.0, 0.0, 0.0, 0.0);
    _addStack_->Margin = Windows::UI::Xaml::Thickness(0.0, 36.0, 0.0, 50.0);

    _accountAddMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _showAddAccountMenuTitle_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _showAddAccountMenuBtn_->Visibility = Windows::UI::Xaml::Visibility::Visible;

    _accountCreationMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _showCreateAccountMenuTitle_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _showCreateAccountMenuBtn_->Visibility = Windows::UI::Xaml::Visibility::Visible;
}

void RingClientUWP::Views::Wizard::OnregisteredNameFound(LookupStatus status,  const std::string& accountId, const std::string& address, const std::string& name)
{
    switch (status)
    {
    case LookupStatus::SUCCESS:
    case LookupStatus::INVALID_NAME:
    case LookupStatus::ERRORR:
        _usernameValid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        _usernameInvalid_->Visibility = Windows::UI::Xaml::Visibility::Visible;
        isUsernameValid = false;
        break;
    case LookupStatus::NOT_FOUND:
        _usernameValid_->Visibility = Windows::UI::Xaml::Visibility::Visible;
        _usernameInvalid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        isUsernameValid = true;
        break;
    }

    checkState();
}

void RingClientUWP::Views::Wizard::OnregistrationStateErrorGeneric(const std::string &accountId)
{
    auto loader = ref new Windows::ApplicationModel::Resources::ResourceLoader();
    _response_->Text = loader->GetString("_accountsCredentialsExpired_");
    _addAccountYes_->IsEnabled = false;
}

void RingClientUWP::Views::Wizard::_PINTextBox__GotFocus(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    _response_->Text = "";
}

void RingClientUWP::Views::Wizard::_ArchivePassword__KeyUp(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e)
{
    if (_PINTextBox_->Text->IsEmpty() || _ArchivePassword_->Password->IsEmpty())
        _addAccountYes_->IsEnabled = false;
    else
        _addAccountYes_->IsEnabled = true;
}

void RingClientUWP::Views::Wizard::_PINTextBox__KeyUp(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e)
{
    if (_PINTextBox_->Text->IsEmpty() || _ArchivePassword_->Password->IsEmpty())
        _addAccountYes_->IsEnabled = false;
    else
        _addAccountYes_->IsEnabled = true;
}

void
Wizard::_createAccountNo__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{

}

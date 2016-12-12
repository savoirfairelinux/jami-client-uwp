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
    void _step1button__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    void _step2button__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    void OnregisteredNameFound(LookupStatus status, const std::string& address, const std::string& name);
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
};

}
}
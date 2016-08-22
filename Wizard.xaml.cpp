#include "pch.h"

#include "Wizard.xaml.h"

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

Wizard::Wizard()
{
    InitializeComponent();
}

void
Wizard::_createAccountYes__Click(Object^ sender, RoutedEventArgs^ e)
{
    // you must ensure alias not null for the daemon
    auto alias = _aliasTextBox_->Text;
    if (alias->IsEmpty())
        alias = "windows user";
    std::wstring wstr(alias->Begin());
    std::string str(wstr.begin(), wstr.end()); // --> what daemon wants
    LPCWSTR debugTxt = wstr.c_str();
    OutputDebugString(debugTxt);
}

void
Wizard::_showCreateAccountMenuBtn__Click(Object^ sender, RoutedEventArgs^ e)
{
    switchMenu();
}

void
Wizard::_showAddAccountMenuBtn__Click(Object^ sender, RoutedEventArgs^ e)
{
    switchMenu();
}

void
Wizard::switchMenu()
{
    if (_accountCreationMenuGrid_->Visibility == Windows::UI::Xaml::Visibility::Visible) {
        _accountCreationMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        _showCreateAccountMenuTitle_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        _showCreateAccountMenuBtn_->Visibility = Windows::UI::Xaml::Visibility::Visible;
    }
    else {
        _accountCreationMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Visible;
        _showCreateAccountMenuTitle_->Visibility = Windows::UI::Xaml::Visibility::Visible;
        _showCreateAccountMenuBtn_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    }

    if (_accountAddMenuGrid_->Visibility == Windows::UI::Xaml::Visibility::Visible) {
        _accountAddMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        _showAddAccountMenuTitle_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        _showAddAccountMenuBtn_->Visibility = Windows::UI::Xaml::Visibility::Visible;
    }
    else {
        _accountAddMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Visible;
        _showAddAccountMenuTitle_->Visibility = Windows::UI::Xaml::Visibility::Visible;
        _showAddAccountMenuBtn_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    }
}


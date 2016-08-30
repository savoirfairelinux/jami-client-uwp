#include "pch.h"

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

Wizard::Wizard()
{
    InitializeComponent();
}

void
Wizard::_createAccountYes__Click(Object^ sender, RoutedEventArgs^ e)
{
    auto alias = _aliasTextBox_->Text;
    if (alias->IsEmpty())
        alias = "windows user";
    std::wstring wstr(alias->Begin());
    std::string str(wstr.begin(), wstr.end());
    RingD::instance->hasConfig = false;
    RingD::instance->accountName = std::string(wstr.begin(), wstr.end());
    this->Dispatcher->RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal, ref new Windows::UI::Core::DispatchedHandler([this] () {
        this->Frame->Navigate(Windows::UI::Xaml::Interop::TypeName(RingClientUWP::MainPage::typeid));
    }));
}

void
Wizard::_showCreateAccountMenuBtn__Click(Object^ sender, RoutedEventArgs^ e)
{
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
    _accountCreationMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _showCreateAccountMenuTitle_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _showCreateAccountMenuBtn_->Visibility = Windows::UI::Xaml::Visibility::Visible;

    _accountAddMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Visible;
    _showAddAccountMenuTitle_->Visibility = Windows::UI::Xaml::Visibility::Visible;
    _showAddAccountMenuBtn_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
}
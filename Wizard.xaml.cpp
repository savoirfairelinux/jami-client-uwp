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
    RingD::instance->init();
}

void
Wizard::_createAccountYes__Click(Object^ sender, RoutedEventArgs^ e)
{
    //RingD::instance->_startingStatus = StartingStatus::REGISTERING_ON_THIS_PC;
    this->Dispatcher->RunAsync(Windows::UI::Core::CoreDispatcherPriority::High, ref new Windows::UI::Core::DispatchedHandler([this] () {
        this->Frame->Navigate(Windows::UI::Xaml::Interop::TypeName(RingClientUWP::MainPage::typeid));
        RingD::instance->createRINGAccount(_fullnameTextBox_->Text
                                           , _password_->Password
                                           , true // upnp by default set to true
                                           , (_RegisterState_->IsOn)? _usernameTextBox_->Text : "");
        _password_->Password = "";
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

void
Wizard::_avatarWebcamCaptureBtn__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    CameraCaptureUI^ cameraCaptureUI = ref new CameraCaptureUI();
    cameraCaptureUI->PhotoSettings->Format = CameraCaptureUIPhotoFormat::Jpeg;
    cameraCaptureUI->PhotoSettings->CroppedSizeInPixels = Size(80, 80);

    create_task(cameraCaptureUI->CaptureFileAsync(CameraCaptureUIMode::Photo))
    .then([this](StorageFile^ photoFile)
    {
        if (photoFile != nullptr) {
            auto brush = ref new ImageBrush();

            auto circle = ref new Ellipse();
            circle->Height = 80;
            circle->Width = 80;
            auto path = photoFile->Path;
            auto uri = ref new Windows::Foundation::Uri(path);
            auto bitmapImage = ref new Windows::UI::Xaml::Media::Imaging::BitmapImage();
            bitmapImage->UriSource = uri;

            std::string fileBuffer = Utils::getStringFromFile(Utils::toString(photoFile->Path));
            std::string profilePath = RingD::instance->getLocalFolder() + ".profile";
            _mkdir(profilePath.c_str());
            std::ofstream file((profilePath + "\\profile_image.png"),
                               std::ios::out | std::ios::trunc | std::ios::binary);
            if (file.is_open()) {
                file << fileBuffer;
                file.close();
            }

            Configuration::UserPreferences::instance->PREF_PROFILE_HASPHOTO = true;
            Configuration::UserPreferences::instance->save();

            brush->ImageSource = bitmapImage;
            circle->Fill = brush;
            _avatarWebcamCaptureBtn_->Content = circle;
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
        RingD::instance->lookUpName(alias);
        _usernameValid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        _usernameInvalid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    }

    // checkState(); is made in OnregisteredNameFound
}

void RingClientUWP::Views::Wizard::_RegisterState__Toggled(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    auto toggleSwitch =  dynamic_cast<ToggleSwitch^>(sender);

    if (_usernameTextBox_ == nullptr) // avoid trouble when InitializeComponent is called for Wizard.
        return;

    isPublic = toggleSwitch->IsOn;

    if (isPublic) {
        _usernameTextBox_->IsEnabled = true;
        _whatWilHappen_->Text = "peoples will find you with your username.";
    } else {
        _usernameTextBox_->IsEnabled = false;
        _whatWilHappen_->Text = "you'll have to send your ringId.";
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
    _accountAddMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _showAddAccountMenuTitle_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _showAddAccountMenuBtn_->Visibility = Windows::UI::Xaml::Visibility::Visible;

    _accountCreationMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _showCreateAccountMenuTitle_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _showCreateAccountMenuBtn_->Visibility = Windows::UI::Xaml::Visibility::Visible;
}

void RingClientUWP::Views::Wizard::_step1button__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    _step1Menu_->Visibility = Windows::UI::Xaml::Visibility::Visible;
    _step2Menu_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;

    _nextstep_->Visibility = Windows::UI::Xaml::Visibility::Visible;
    _addAccountYes_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
}

void RingClientUWP::Views::Wizard::_step2button__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    _step1Menu_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _step2Menu_->Visibility = Windows::UI::Xaml::Visibility::Visible;

    _nextstep_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _addAccountYes_->Visibility = Windows::UI::Xaml::Visibility::Visible;
}

void RingClientUWP::Views::Wizard::OnregisteredNameFound(LookupStatus status, const std::string& address, const std::string& name)
{
    switch (status)
    {
    case LookupStatus::SUCCESS:
        _usernameValid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        _usernameInvalid_->Visibility = Windows::UI::Xaml::Visibility::Visible;
        isUsernameValid = false;
        break;
    case LookupStatus::INVALID_NAME:
        _usernameValid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        _usernameInvalid_->Visibility = Windows::UI::Xaml::Visibility::Visible;
        isUsernameValid = false;
        break;
    case LookupStatus::NOT_FOUND:
        _usernameValid_->Visibility = Windows::UI::Xaml::Visibility::Visible;
        _usernameInvalid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        isUsernameValid = true;
        break;
    case LookupStatus::ERRORR:
        _usernameValid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        _usernameInvalid_->Visibility = Windows::UI::Xaml::Visibility::Visible;
        isUsernameValid = false;
        break;
    }

    checkState();
}



void RingClientUWP::Views::Wizard::OnregistrationStateErrorGeneric(const std::string &accountId)
{
    _response_->Text = "Credentials error or PIN expired.";
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

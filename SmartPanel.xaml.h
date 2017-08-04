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

#include "SmartPanel.g.h"

#include <regex>

using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Interop;
using namespace Windows::UI::Xaml::Documents;

namespace RingClientUWP
{

delegate void ToggleSmartPan();
delegate void SummonMessageTextPage();
delegate void SummonVideoPage();
delegate void SummonWelcomePage();
delegate void SummonPreviewPage();
delegate void HidePreviewPage();

namespace Views
{

public ref class SmartPanel sealed
{
public:
    SmartPanel();
    void updatePageContent();

internal:
    enum class Mode { Minimized, Normal };
    event ToggleSmartPan^ toggleSmartPan;
    event SummonMessageTextPage^ summonMessageTextPage;
    event SummonVideoPage^ summonVideoPage;
    event SummonWelcomePage^ summonWelcomePage;
    event SummonPreviewPage^ summonPreviewPage;
    event HidePreviewPage^ hidePreviewPage;
    void setMode(RingClientUWP::Views::SmartPanel::Mode mode);

private:
    enum class MenuOpen {
        CONTACTS_LIST,
        CONTACTREQUEST_LIST,
        ACCOUNTS_LIST,
        SHARE,
        DEVICE,
        SETTINGS
    } menuOpen;

    /* functions */
    void setBannedListState(bool visible);
    void _addAccountBtn__Click(Platform::Object^ sender, RoutedEventArgs^ e);
    void _createAccountYes__Click(Platform::Object^ sender, RoutedEventArgs^ e);
    void _createAccountNo__Click(Platform::Object^ sender, RoutedEventArgs^ e);
    void _smartList__SelectionChanged(Platform::Object^ sender, SelectionChangedEventArgs^ e);
    void _accountList__SelectionChanged(Platform::Object^ sender, SelectionChangedEventArgs^ e);
    void _rejectIncomingCallBtn__Click(Platform::Object^ sender, RoutedEventArgs^ e);
    void _acceptIncomingCallBtn__Click(Platform::Object^ sender, RoutedEventArgs^ e);
    void _callBtn__Click(Platform::Object^ sender, RoutedEventArgs^ e);
    void _cancelCallBtn__Click(Platform::Object^ sender, RoutedEventArgs^ e);
    void SmartPanelItem_Grid_PointerEntered(Platform::Object^ sender, PointerRoutedEventArgs^ e);
    void SmartPanelItem_Grid_PointerExited(Platform::Object^ sender, PointerRoutedEventArgs^ e);
    void _videoDeviceComboBox__SelectionChanged(Platform::Object^ sender, RoutedEventArgs^);
    void _videoResolutionComboBox__SelectionChanged(Platform::Object^ sender, RoutedEventArgs^);
    void _videoRateComboBox__SelectionChanged(Platform::Object^ sender, RoutedEventArgs^);
    void populateVideoDeviceSettingsComboBox();
    void populateVideoResolutionSettingsComboBox();
    void populateVideoRateSettingsComboBox();
    void checkStateAddAccountMenu();
    void checkStateEditionMenu();
    void ringTxtBxPlaceHolderDelay(String^ placeHolderText, int delayInMilliSeconds);
    void showLinkThisDeviceStep1();
    void OnstateChange(Platform::String ^callId, RingClientUWP::CallStatus state, int code);
    void addToContactList(String^ name);
    void undoListBoxDeselection(ListBox^ listBox, SelectionChangedEventArgs^ e);
    void placeCall(SmartPanelItem^ item);
    void _addDevice__Click(Platform::Object^ sender, RoutedEventArgs^ e);
    void OndevicesListRefreshed(Map<String^, String^>^ deviceMap);
    void _pinGeneratorYes__Click(Platform::Object^ sender, RoutedEventArgs^ e);
    void _pinGeneratorNo__Click(Platform::Object^ sender, RoutedEventArgs^ e);
    void OnexportOnRingEnded(Platform::String ^accountId, Platform::String ^pin);
    void _closePin__Click(Platform::Object^ sender, RoutedEventArgs^ e);
    void _editAccountMenuButton__Click(Platform::Object^ sender, RoutedEventArgs^ e);
    void _acceptAccountModification__Click(Platform::Object^ sender, RoutedEventArgs^ e);
    void _cancelAccountModification__Click(Platform::Object^ sender, RoutedEventArgs^ e);
    void OnaccountUpdated(RingClientUWP::Account ^account);
    void _passwordBoxAccountCreationCheck__PasswordChanged(Platform::Object^ sender, RoutedEventArgs^ e);
    void _accountTypeComboBox__SelectionChanged(Platform::Object^ sender, SelectionChangedEventArgs^ e);
    void _accountAliasTextBox__TextChanged(Platform::Object^ sender, TextChangedEventArgs^ e);
    void _accountAliasTextBoxEdition__TextChanged(Platform::Object^ sender, TextChangedEventArgs^ e);
    void _selectedAccountAvatarContainer__PointerEntered(Platform::Object^ sender, PointerRoutedEventArgs^ e);
    void _selectedAccountAvatarContainer__PointerReleased(Platform::Object^ sender, PointerRoutedEventArgs^ e);
    void _selectedAccountAvatarContainer__PointerExited(Platform::Object^ sender, PointerRoutedEventArgs^ e);
    void _smartList__PointerExited(Platform::Object^ sender, PointerRoutedEventArgs^ e);
    void SmartPanelItem_Grid_PointerMoved(Platform::Object^ sender, PointerRoutedEventArgs^ e);
    void _registerOnBlockchainEdition__Click(Platform::Object^ sender, RoutedEventArgs^ e);
    void _usernameTextBoxEdition__KeyUp(Platform::Object^ sender, KeyRoutedEventArgs^ e);
    void OnregisteredNameFound(RingClientUWP::LookupStatus status, const std::string& accountId, const std::string& address, const std::string& name);
    void _RegisterState__Toggled(Platform::Object^ sender, RoutedEventArgs^ e);
    void _usernameTextBox__KeyUp(Platform::Object^ sender, KeyRoutedEventArgs^ e);
    void _RegisterStateEdition__Toggled(Platform::Object^ sender, RoutedEventArgs^ e);
    void _ringTxtBx__KeyUp(Platform::Object^ sender, KeyRoutedEventArgs^ e);
    void _linkThisDeviceBtn__Click(Platform::Object^ sender, RoutedEventArgs^ e);
    void _step2button__Click(Platform::Object^ sender, RoutedEventArgs^ e);
    void _step1button__Click(Platform::Object^ sender, RoutedEventArgs^ e);
    void _addAccountNo__Click(Platform::Object^ sender, RoutedEventArgs^ e);
    void _addAccountYes__Click(Platform::Object^ sender, RoutedEventArgs^ e);
    void OnregistrationStateErrorGeneric(const std::string& accountId);
    void _PINTextBox__GotFocus(Platform::Object^ sender, RoutedEventArgs^ e);
    void OnnameRegistred(bool status, String ^accountId);
    void OnregistrationStateRegistered(const std::string& accountId);
    void OncallPlaced(Platform::String ^callId);
    void OncontactDataModified(Platform::String ^account, Contact^ contact);
    void OnnewBuddyNotification(const std::string& accountId, const std::string& address, int status);
    void updateContactNotificationsState(Contact^ contact);
    void updateNotificationsState();
    void selectMenu(MenuOpen menu);
    void _addAccountInputValidation__KeyUp(Platform::Object^ sender, RoutedEventArgs^ e);
    void OnregistrationStateChanged(const std::string& accountId);
    void updateCallAnimationState(SmartPanelItem^ item, bool state);
    void OnincomingAccountMessage(Platform::String ^accountId, Platform::String ^from, Platform::String ^payload);
    void _ringTxtBx__Click(Platform::Object^ sender, RoutedEventArgs^ e);
    void _contactsListMenuButton__Click(Platform::Object^ sender, RoutedEventArgs^ e);
    void _accountsMenuButton__Click(Platform::Object^ sender, RoutedEventArgs^ e);
    void _shareMenuButton__Click(Platform::Object^ sender, RoutedEventArgs^ e);
    void _devicesMenuButton__Click(Platform::Object^ sender, RoutedEventArgs^ e);
    void _settingsMenuButton__Click(Platform::Object^ sender, RoutedEventArgs^ e);
    void showWelcomePage();
    void _passwordForPinGenerator__KeyUp(Platform::Object^ sender, KeyRoutedEventArgs^ e);
    void requestPin();
    void ContactRequestItem_Grid_PointerReleased(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e);
    void ContactRequestItem_Grid_PointerEntered(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e);
    void ContactRequestItem_Grid_PointerExited(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e);
    void ContactRequestItem_Grid_PointerMoved(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e);
    void _contactRequestListMenuButton__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    void _acceptContactRequestBtn__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    void _rejectContactRequestBtn__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    void _blockContactBtn__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    void _smartList__GotFocus(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    void _showBannedList__PointerPressed(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e);
    void _addBannedContactBtn__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    void SmartPanelItem_Grid_RightTapped(Platform::Object^ sender, Windows::UI::Xaml::Input::RightTappedRoutedEventArgs^ e);
    void _videocall_MenuFlyoutItem_Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e);
    void _addToConference_MenuFlyoutItem__Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e);
    void _copyRingID_MenuFlyoutItem__Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e);
    void SmartPanelItem_Grid_DoubleTapped(Platform::Object^ sender, Windows::UI::Xaml::Input::DoubleTappedRoutedEventArgs^ e);
    void _revokeDeviceButton__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    void _deviceName__KeyUp(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e);
    void _deviceName__LostFocus(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    void _turnEnabledToggle__Toggled(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    void _deleteAccountButton__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    void _removeContact_MenuFlyoutItem__Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e);
};
}
}

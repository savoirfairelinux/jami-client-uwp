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
#include "SmartPanel.g.h"

namespace RingClientUWP
{

delegate void ToggleSmartPan();
delegate void SumonMessageTextPage();
delegate void SumonVideoPage();

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
    event SumonMessageTextPage^ sumonMessageTextPage;
    event SumonVideoPage^ sumonVideoPage;
    void setMode(RingClientUWP::Views::SmartPanel::Mode mode);

private:
    /* functions */
    void _accountsMenuButton__Checked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    void _accountsMenuButton__Unchecked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    void _settings__Checked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    void _settings__Unchecked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    void _shareMenuButton__Checked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    void _shareMenuButton__Unchecked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    void _addAccountBtn__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    void _createAccountYes__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    void _createAccountNo__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    void _smartList__SelectionChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::SelectionChangedEventArgs^ e);
    void _accountList__SelectionChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::SelectionChangedEventArgs^ e);
    void _ringTxtBx__KeyDown(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e);
    void _ringTxtBx__Click(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e);
    void _rejectIncomingCallBtn__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    void _acceptIncomingCallBtn__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    void _callContact__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    void _cancelCallBtn__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    void Grid_PointerEntered(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e);
    void Grid_PointerExited(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e);
    void _contactItem__PointerReleased(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e);

    /* members */
};
}
}
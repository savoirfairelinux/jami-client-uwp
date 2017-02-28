/**************************************************************************
* Copyright (C) 2016 by Savoir-faire Linux                                *
* Author: Jäger Nicolas <nicolas.jager@savoirfairelinux.com>              *
* Author: Traczyk Andreas<andreas.traczyk@savoirfairelinux.com>           *
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

#include "TextBlockExtension.h"
#include "MessageTextPage.g.h"

namespace RingClientUWP
{
delegate void CloseMessageTextPage();

namespace Views
{

public ref class BubbleBackground sealed : IValueConverter {
public:
    virtual Object^ Convert(Object^ value, Windows::UI::Xaml::Interop::TypeName targetType, Object^ parameter, String^ language);
    virtual Object^ ConvertBack(Object^ value, Windows::UI::Xaml::Interop::TypeName  targetType, Object^ parameter, String^ language);
    BubbleBackground();
};

public ref class BubbleHorizontalAlignement sealed : IValueConverter {
public:
    virtual Object^ Convert(Object^ value, Windows::UI::Xaml::Interop::TypeName targetType, Object^ parameter, String^ language);
    virtual Object^ ConvertBack(Object^ value, Windows::UI::Xaml::Interop::TypeName  targetType, Object^ parameter, String^ language);
    BubbleHorizontalAlignement();
};

public ref class MessageTextPage sealed
{
public:
    MessageTextPage();

    void scrollDown();
    void updateMessageStatus(String^ messageId, int status);

internal:
    void updatePageContent(SmartPanelItem^ item = nullptr);
    event CloseMessageTextPage^ closeMessageTextPage;

private:
    String^ lastMessageText;

    void _sendBtn__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    void sendMessage();
    void OnincomingMessage(Platform::String ^callId, Platform::String ^payload);
    void OnSelectionChanged(Platform::Object ^sender, Windows::UI::Xaml::Controls::SelectionChangedEventArgs ^e);
    void _deleteContactBtn__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    void _clearConversationBtn__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    void _audioCallBtn__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    void _videoCallBtn__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    void _messageTextBox__TextChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e);
    void _sendContactRequestBtn__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
};
}
}

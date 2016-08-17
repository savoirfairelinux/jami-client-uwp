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
#include "pch.h"

#include "RingConsolePanel.xaml.h"

using namespace RingClientUWP;
using namespace RingClientUWP::Views;
using namespace Windows::UI::Xaml::Documents;

RingConsolePanel::RingConsolePanel()
{
    InitializeComponent();

    RingDebug::instance->messageToScreen += ref new debugMessageToScreen([this](Platform::String^ message) {
        output(message);
    });
}

void
RingConsolePanel::output(Platform::String^ message)
{
    try {
        Run^ inlineText = ref new Run();
        inlineText->Text = message;
        Paragraph^ paragraph = ref new Paragraph();
        paragraph->Inlines->Append(inlineText);
        _debugWindowOutput_->Blocks->Append(paragraph);
    }
    catch (Platform::Exception^ e) {
        return;
    }
    _scrollView_->UpdateLayout();
    _scrollView_->ScrollToVerticalOffset(_scrollView_->ScrollableHeight);
}
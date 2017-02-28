/**************************************************************************
* Copyright (C) 2016 by Savoir-faire Linux                                *
* Author: Traczyk Andreas <traczyk.andreas@savoirfairelinux.com>          *
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

using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Interop;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Interop;
using namespace Windows::UI::Xaml::Documents;
using namespace Platform;

#include "pch.h"
#include <regex>

namespace RingClientUWP
{

namespace UserAndCustomControls {

public ref class TextBlockExtension sealed : public Control
{
private:
    static DependencyProperty^ FormattedTextProperty;

public:
    TextBlockExtension::TextBlockExtension();

    static String^ GetFormattedText(DependencyObject^ obj) {
        return (String^)obj->GetValue(FormattedTextProperty);
    };
    static void SetFormattedText(DependencyObject^ obj, String^ value) {
        obj->SetValue(FormattedTextProperty, value);
    }

};

}
}
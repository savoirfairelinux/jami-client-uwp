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

#include "WelcomePage.xaml.h"

using namespace RingClientUWP;
using namespace RingClientUWP::Views;

WelcomePage::WelcomePage()
{
    InitializeComponent();
    Window::Current->SizeChanged += ref new WindowSizeChangedEventHandler(this, &WelcomePage::OnResize);
    OnResize(nullptr, nullptr);
};

void
WelcomePage::PositionImage()
{
    Rect imageBounds;
    imageBounds.Width = static_cast<float>(_welcomePage_->ActualWidth);
    imageBounds.Height = static_cast<float>(_welcomePage_->ActualHeight);

    _welcomeImage_->SetValue(Canvas::LeftProperty, imageBounds.Width * 0.5 - _welcomeImage_->Width * 0.5);
    _welcomeImage_->SetValue(Canvas::TopProperty, imageBounds.Height * 0.5 - _welcomeImage_->Height * 0.5);
}

void
WelcomePage::OnResize(Platform::Object^ sender, Windows::UI::Core::WindowSizeChangedEventArgs^ e)
{
    PositionImage();
}
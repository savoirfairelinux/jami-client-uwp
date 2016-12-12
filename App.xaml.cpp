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

#include "LoadingPage.xaml.h"
#include "MainPage.xaml.h"
#include "Wizard.xaml.h"

using namespace Windows::ApplicationModel::Core;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;
using namespace Windows::UI;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Interop;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::ViewManagement;
using namespace Windows::UI::Core;

using namespace RingClientUWP;

App::App()
{
    InitializeComponent(); // summon partial class, form generated files trough App.xaml

    /* connect to delegate */
    RingD::instance->summonWizard += ref new RingClientUWP::SummonWizard(this, &RingClientUWP::App::OnsummonWizard);
}

void
App::OnLaunched(LaunchActivatedEventArgs^ e)
{
    rootFrame = dynamic_cast<Frame^>(Window::Current->Content);

    if (rootFrame == nullptr) {
        rootFrame = ref new Frame();

        if (rootFrame->Content == nullptr)
            rootFrame->Navigate(TypeName(Views::LoadingPage::typeid), e->Arguments);

        Window::Current->Content = rootFrame;
    }
    else {
        rootFrame->Navigate(TypeName(Views::LoadingPage::typeid), e->Arguments);
        Window::Current->Content = rootFrame;
    }

    Windows::UI::ViewManagement::ApplicationView::PreferredLaunchViewSize = Size(800, 700);
    Windows::UI::ViewManagement::ApplicationView::PreferredLaunchWindowingMode
        = Windows::UI::ViewManagement::ApplicationViewWindowingMode::PreferredLaunchViewSize;

    Window::Current->Activate();

    auto color = Windows::UI::ColorHelper::FromArgb(255, 59, 193, 211);

    ApplicationView::GetForCurrentView()->TitleBar->ButtonBackgroundColor = color;
    ApplicationView::GetForCurrentView()->TitleBar->InactiveBackgroundColor = color;
    ApplicationView::GetForCurrentView()->TitleBar->ButtonInactiveBackgroundColor = color;
    ApplicationView::GetForCurrentView()->TitleBar->BackgroundColor = color;
    ApplicationView::GetForCurrentView()->TitleBar->ForegroundColor = Colors::White;
    ApplicationView::GetForCurrentView()->TitleBar->ButtonForegroundColor = Colors::White;
}

void App::OnsummonWizard()
{
    rootFrame->Navigate(Windows::UI::Xaml::Interop::TypeName(Views::Wizard::typeid));
}
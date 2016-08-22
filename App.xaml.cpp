/***************************************************************************
 * Copyright (C) 2016 by Savoir-faire Linux                                *
 * Author: Jäger Nicolas <nicolas.jager@savoirfairelinux.com>              *
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
#include "pch.h"

#include "MainPage.xaml.h"
#include "Wizard.xaml.h"

using namespace Concurrency;
using namespace Windows::Storage;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;
using namespace Windows::UI;
using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Interop;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::ViewManagement;

using namespace RingClientUWP;

App::App()
{
    InitializeComponent(); // summon partial class, form generated files trough App.xaml

}

void
App::OnLaunched(LaunchActivatedEventArgs^ e)
{
    bool noAccountFound = true;
    rootFrame = dynamic_cast<Frame^>(Window::Current->Content);

    if (rootFrame == nullptr) {
        rootFrame = ref new Frame();

        if (rootFrame->Content == nullptr)
            if(noAccountFound)
                rootFrame->Navigate(TypeName(Views::Wizard::typeid), e->Arguments);
            else
                rootFrame->Navigate(TypeName(MainPage::typeid), e->Arguments);

        Window::Current->Content = rootFrame;
        Window::Current->Activate();
    } else if (noAccountFound)
        rootFrame->Navigate(TypeName(Views::Wizard::typeid), e->Arguments);
    else
        rootFrame->Navigate(TypeName(MainPage::typeid), e->Arguments);

    CoreApplication::GetCurrentView()->TitleBar->ExtendViewIntoTitleBar = true;
    ApplicationView::GetForCurrentView()->TitleBar->ButtonBackgroundColor = Colors::LightBlue;
    ApplicationView::GetForCurrentView()->TitleBar->ButtonInactiveBackgroundColor = Colors::LightBlue;
    ApplicationView::GetForCurrentView()->TitleBar->ForegroundColor = Colors::White;
    ApplicationView::GetForCurrentView()->TitleBar->ButtonForegroundColor = Colors::White;

    /* summon the daemon */
    RingD::instance->startDaemon();
}
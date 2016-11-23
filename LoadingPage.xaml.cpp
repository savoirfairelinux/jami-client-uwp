/**************************************************************************
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

#include "LoadingPage.xaml.h"

#include "MainPage.xaml.h"
#include "Wizard.xaml.h"

using namespace RingClientUWP;
using namespace RingClientUWP::Views;
using namespace RingClientUWP::ViewModel;

using namespace Platform;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::ViewManagement;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Interop;
using namespace Windows::UI::Xaml::Navigation;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::Graphics::Display;
using namespace Windows::System;
using namespace Utils;

LoadingPage::LoadingPage()
{
    InitializeComponent();

    std::string configFile = RingD::instance->getLocalFolder() + ".config\\dring.yml";
    std::string tokenFile = RingD::instance->getLocalFolder() + "creation.token";
    if (fileExists(configFile)) {
        if (fileExists(tokenFile)) {
            /* we have a token, the config has to be considered as corrupted, delete the config, summon the wizard */
            fileDelete(configFile);
            this->Dispatcher->RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal,
                ref new Windows::UI::Core::DispatchedHandler([this]()
            {
                this->Frame->Navigate(Windows::UI::Xaml::Interop::TypeName(Wizard::typeid));
            }));
        }
        else {
            /* there is no token and we have a config.yml, summon the main page */
            this->Dispatcher->RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal,
                ref new Windows::UI::Core::DispatchedHandler([this]()
            {
                this->Frame->Navigate(Windows::UI::Xaml::Interop::TypeName(MainPage::typeid));
            }));
        }
    }
    else {
        /* no config file, create the token and summon the wizard*/
        std::ofstream fs;
        fs.open(tokenFile, std::ios::out);
        fs.close();

        this->Dispatcher->RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal,
            ref new Windows::UI::Core::DispatchedHandler([this] ()
        {
            this->Frame->Navigate(Windows::UI::Xaml::Interop::TypeName(Wizard::typeid));
        }));
    }
}

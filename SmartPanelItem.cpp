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

#include "SmartPanelItem.h"

using namespace Windows::ApplicationModel::Core;
using namespace Platform;
using namespace Windows::Data::Json;
using namespace Windows::UI::Core;

using namespace RingClientUWP;
using namespace RingClientUWP::Controls;

SmartPanelItem::SmartPanelItem()
{
    _callId = "";
    videoMuted_ = false;
    isSelected_ = false;
    isHovered_ = false;
    _callStatus = CallStatus::NONE;

    RingD::instance->callPlaced += ref new RingClientUWP::CallPlaced(this, &SmartPanelItem::OncallPlaced);
}

void
SmartPanelItem::muteVideo(bool state)
{
    videoMuted_ = state;
    RingD::instance->muteVideo(_callId, state);
}

void
SmartPanelItem::startCallTimer()
{
    call_.callStartTime = std::chrono::steady_clock::now();;
}

void
SmartPanelItem::NotifyPropertyChanged(String^ propertyName)
{
    CoreApplicationView^ view = CoreApplication::MainView;
    view->CoreWindow->Dispatcher->RunAsync(
        CoreDispatcherPriority::High,
        ref new DispatchedHandler([this, propertyName]()
    {
        PropertyChanged(this, ref new PropertyChangedEventArgs(propertyName));
    }));
}

void
SmartPanelItem::OncallPlaced(Platform::String ^callId)
{
    if (_callId == callId) {
        _callStatus = CallStatus::SEARCHING;
    }
}

void
SmartPanelItem::raiseNotifyPropertyChanged(String^ propertyName)
{
    NotifyPropertyChanged(propertyName);
}
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

#include <pch.h>

using namespace Platform;
using namespace Platform::Collections;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Storage;
using namespace Windows::Storage::Streams;
using namespace Windows::System;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Media::Imaging;
using namespace Windows::ApplicationModel::Resources;

namespace RingClientUWP
{

public ref class ResourceMananger sealed
{
public:
    /* properties */
    static property ResourceMananger^ instance {
        ResourceMananger^ get() {
            static ResourceMananger^ instance_ = ref new ResourceMananger();
            return instance_;
        }
    }

    void            preloadImage(String^ path);
    BitmapImage^    imageFromRelativePath(String^ path);

    String^         getStringResource(String^ key);

private:
    ResourceMananger();

    Map<String^ , BitmapImage^> preload;
    ResourceLoader^ stringLoader;

};

}
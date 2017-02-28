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

#include "ResourceManager.h"

using namespace RingClientUWP;

using namespace Windows::UI::Core;
using namespace Windows::ApplicationModel::Resources;

String^
getFriendlyResourceName(String^ path)
{
    return Utils::toPlatformString(Utils::fileNameOnly(Utils::toString(path)));
}

ResourceMananger::ResourceMananger()
{
    stringLoader = ref new ResourceLoader();
}

void
ResourceMananger::preloadImage(String^ path)
{
    IOMSG_("attempting to preload image: " + path);
    auto friendlyName = getFriendlyResourceName(path);
    if (!preload.HasKey(friendlyName)) {
        preload.Insert(friendlyName, nullptr);
        IAsyncOperation<StorageFile^>^ getFileTask;
        if (Utils::toString(path).find("ms-appx") == std::string::npos) {
            getFileTask = StorageFile::GetFileFromPathAsync(path);
        }
        else {
            auto uri = ref new Uri(path);
            getFileTask = StorageFile::GetFileFromApplicationUriAsync(uri);
        }
        create_task(getFileTask)
            .then([=](task<StorageFile^> t)
        {
            StorageFile^ storageFile;
            try {
                storageFile = t.get();
                create_task(storageFile->OpenAsync(FileAccessMode::Read))
                    .then([=](task<IRandomAccessStream^> fileStreamTask)
                {
                    try {
                        auto fileStream = fileStreamTask.get();
                        auto bmp = ref new BitmapImage();
                        bmp->SetSource(fileStream);
                        IOMSG_("preloading image: " + friendlyName);
                        preload.Insert(friendlyName, bmp);
                    }
                    catch (Platform::Exception^ e) {
                        EXC_(e);
                    }
                });
            }
            catch (Platform::Exception^ e) {
                EXC_(e);
            }
        });
    }
}

BitmapImage^
ResourceMananger::imageFromRelativePath(String^ path)
{
    auto friendlyName = getFriendlyResourceName(path);
    if (preload.HasKey(friendlyName)) {
        IOMSG_("loading preloaded image: " + friendlyName);
        return preload.Lookup(friendlyName);
    }
    else {
        preloadImage(path);
    }
    auto uri = ref new Uri(path);
    auto bmp = ref new BitmapImage();
    bmp->UriSource = uri;
    IOMSG_("loading image from disk: " + friendlyName);
    return bmp;
}

String^
ResourceMananger::getStringResource(String^ key)
{
    return stringLoader->GetString(key);
}
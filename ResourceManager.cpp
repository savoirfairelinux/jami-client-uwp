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

#include <ResourceManager.h>

using namespace RingClientUWP;

using namespace Windows::UI::Core;
using namespace Windows::ApplicationModel::Resources;

ResourceMananger::ResourceMananger()
{
    stringLoader = ref new ResourceLoader();
}

void
ResourceMananger::preloadImage(String^ path)
{
    if (!preload.HasKey(path)) {
        create_task(StorageFile::GetFileFromPathAsync(path))
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
                        if (!preload.HasKey(path)) {
                            bmp->SetSource(fileStream);
                            preload.Insert(path, bmp);
                        }
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
    if (preload.HasKey(path)) {
        return preload.Lookup(path);
    }
    else {
        auto uri = ref new Uri(path);
        auto bmp = ref new BitmapImage();
        bmp->UriSource = uri;
        return bmp;
    }
}

String^
ResourceMananger::getStringResource(String^ key)
{
    return stringLoader->GetString(key);
}
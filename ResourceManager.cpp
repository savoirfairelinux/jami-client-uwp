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

#define FILE_NAME_ONLY(X) (strrchr(X, '\\') ? strrchr(X, '\\') + 1 : X)

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
    auto fileName = Utils::toPlatformString(Utils::fileNameOnly(Utils::toString(path)));
    auto bmp = ref new BitmapImage();
    if (preload.HasKey(path)){
        bmp = preload.Lookup(path);
        if (bmp) {
            MSG_("loading preloaded image: " + fileName);
            return bmp;
        }
        else {
            MSG_("preloading image: " + fileName);
            preloadImage(path);
        }
    }
    auto uri = ref new Uri(path);
    bmp->UriSource = uri;
    MSG_("loading image from disk: " + fileName);
    return bmp;
}

String^
ResourceMananger::getStringResource(String^ key)
{
    return stringLoader->GetString(key);
}
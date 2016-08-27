/***************************************************************************
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

using namespace Windows::Data::Json;
using namespace Windows::Storage;

using namespace RingClientUWP;
using namespace Platform;
using namespace Configuration;

void
UserPreferences::save()
{
    StorageFolder^ localfolder = ApplicationData::Current->LocalFolder;
    String^ preferencesFile = "preferences.json";

    try {
        create_task(localfolder->CreateFileAsync(preferencesFile
            ,Windows::Storage::CreationCollisionOption::ReplaceExisting))
            .then([&](StorageFile^ newFile){
            try {
                FileIO::WriteTextAsync(newFile,Stringify());
            }
            catch (Exception^ e) {
                RingDebug::instance->print("Exception while writing to preferences file");
            }
        });
    }
    catch (Exception^ e) {
        RingDebug::instance->print("Exception while opening preferences file");
    }
}

void
UserPreferences::load()
{
    String^ preferencesFile = "preferences.json";

    Utils::fileExists(ApplicationData::Current->LocalFolder,
        preferencesFile)
        .then([this,preferencesFile](bool contacts_file_exists)
    {
        if (contacts_file_exists) {
            RingDebug::instance->print("opened preferences file");
            try {
                create_task(ApplicationData::Current->LocalFolder->GetFileAsync(preferencesFile))
                    .then([this](StorageFile^ file)
                {
                    create_task(FileIO::ReadTextAsync(file))
                        .then([this](String^ fileContents){
                        RingDebug::instance->print("reading preferences file");
                        if (fileContents != nullptr) {
                            Destringify(fileContents);
                            // select account index after loading preferences
                            selectIndex(PREF_ACCOUNT_INDEX);
                        }
                    });
                });
            }
            catch (Exception^ e) {
                RingDebug::instance->print("Exception while opening preferences file");
            }
        }
        else {
            selectIndex(0);
        }
    });
}

String^
UserPreferences::Stringify()
{
    JsonObject^ preferencesObject = ref new JsonObject();
    preferencesObject->SetNamedValue("PREF_ACCOUNT_INDEX", JsonValue::CreateNumberValue(PREF_ACCOUNT_INDEX));
    return preferencesObject->Stringify();
}

void
UserPreferences::Destringify(String^ data)
{
    JsonObject^ jsonObject = JsonObject::Parse(data);
    PREF_ACCOUNT_INDEX = static_cast<int>(jsonObject->GetNamedNumber("PREF_ACCOUNT_INDEX"));
    JsonArray^ preferencesList = jsonObject->GetNamedArray("Account.index", ref new JsonArray());
}
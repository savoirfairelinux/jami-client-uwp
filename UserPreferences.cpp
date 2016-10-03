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
using namespace Windows::UI::Core;
using namespace Windows::ApplicationModel::Core;

using namespace RingClientUWP;
using namespace Platform;
using namespace Configuration;

void
UserPreferences::save()
{
    StorageFolder^ localfolder = ApplicationData::Current->LocalFolder;
    String^ preferencesFile = localfolder->Path + "\\" + "preferences.json";

    std::ofstream file(Utils::toString(preferencesFile).c_str());
    if (file.is_open())
    {
        file << Utils::toString(Stringify());
        file.close();
    }
}

void
UserPreferences::load()
{
    StorageFolder^ localfolder = ApplicationData::Current->LocalFolder;
    String^ preferencesFile = localfolder->Path + "\\preferences.json";

    String^ fileContents = Utils::toPlatformString(Utils::getStringFromFile(Utils::toString(preferencesFile)));

    CoreApplication::MainView->CoreWindow->Dispatcher->RunAsync(CoreDispatcherPriority::High,
    ref new DispatchedHandler([=]() {
        if (fileContents != nullptr) {
            Destringify(fileContents);
            selectIndex(PREF_ACCOUNT_INDEX);
            if (PREF_PROFILE_PHOTO)
                loadProfileImage();
        }
        else
            selectIndex(0);
    }));
}

String^
UserPreferences::Stringify()
{
    JsonObject^ preferencesObject = ref new JsonObject();

    preferencesObject->SetNamedValue("PREF_ACCOUNT_INDEX", JsonValue::CreateNumberValue(    PREF_ACCOUNT_INDEX));
    preferencesObject->SetNamedValue("PREF_PROFILE_PHOTO", JsonValue::CreateBooleanValue(   PREF_PROFILE_PHOTO));

    return preferencesObject->Stringify();
}

void
UserPreferences::Destringify(String^ data)
{
    JsonObject^ jsonObject = JsonObject::Parse(data);

    PREF_ACCOUNT_INDEX = static_cast<int>(jsonObject->GetNamedNumber(   "PREF_ACCOUNT_INDEX"    ));
    PREF_PROFILE_PHOTO = jsonObject->GetNamedBoolean(                   "PREF_PROFILE_PHOTO"    );

    JsonArray^ preferencesList = jsonObject->GetNamedArray("Account.index", ref new JsonArray());
}
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

#include "base64.h"

using namespace Windows::Data::Json;
using namespace Windows::Storage;
using namespace Windows::UI::Core;
using namespace Windows::ApplicationModel::Core;

using namespace RingClientUWP;
using namespace Platform;
using namespace Configuration;

UserPreferences::UserPreferences()
{
    vCard_ = ref new VCardUtils::VCard(nullptr);
    PREF_PROFILE_HASPHOTO = false;
    PREF_PROFILE_UID = stoull(Utils::genID(0LL, 9999999999999LL));
}

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
    saveProfileToVCard();
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
            saveProfileToVCard();
            selectIndex(PREF_ACCOUNT_INDEX);
            if (PREF_PROFILE_HASPHOTO)
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

    preferencesObject->SetNamedValue("PREF_ACCOUNT_INDEX", JsonValue::CreateNumberValue(        PREF_ACCOUNT_INDEX   ));
    preferencesObject->SetNamedValue("PREF_PROFILE_HASPHOTO", JsonValue::CreateBooleanValue(    PREF_PROFILE_HASPHOTO));
    preferencesObject->SetNamedValue("PREF_PROFILE_UID",    JsonValue::CreateNumberValue(static_cast<double>(PREF_PROFILE_UID)));

    return preferencesObject->Stringify();
}

void
UserPreferences::Destringify(String^ data)
{
    JsonObject^ jsonObject = JsonObject::Parse(data);

    PREF_ACCOUNT_INDEX      = static_cast<int>(jsonObject->GetNamedNumber("PREF_ACCOUNT_INDEX"      ));
    PREF_PROFILE_HASPHOTO   = jsonObject->GetNamedBoolean(                "PREF_PROFILE_HASPHOTO"   );
    PREF_PROFILE_UID        = static_cast<uint64_t>(jsonObject->GetNamedNumber( "PREF_PROFILE_UID"  ));

    JsonArray^ preferencesList = jsonObject->GetNamedArray("Account.index", ref new JsonArray());
}

VCardUtils::VCard^
UserPreferences::getVCard()
{
    return vCard_;
}

void
UserPreferences::saveProfileToVCard()
{
    std::map<std::string, std::string> vcfData;
    vcfData[VCardUtils::Property::UID] = std::to_string(PREF_PROFILE_UID);
    std::string imageFile(RingD::instance->getLocalFolder() + "\\.profile\\profile_image.png");
    std::basic_ifstream<uint8_t> stream(imageFile, std::ios::in | std::ios::binary);
    auto eos = std::istreambuf_iterator<uint8_t>();
    auto buffer = std::vector<uint8_t>(std::istreambuf_iterator<uint8_t>(stream), eos);
    auto accountListItem = ViewModel::AccountListItemsViewModel::instance->_selectedItem;
    vcfData[VCardUtils::Property::FN] = accountListItem ? Utils::toString(accountListItem->_account->name_) : "Unknown";
    vcfData[VCardUtils::Property::PHOTO] = ring::base64::encode( buffer );
    vCard_->setData(vcfData);
    vCard_->saveToFile();
}

void
UserPreferences::sendVCard(std::string callID)
{
    vCard_->send(callID,
        (RingD::instance->getLocalFolder() + "\\.vcards\\" + std::to_string(PREF_PROFILE_UID) + ".vcard").c_str());
}
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
#pragma once

#include "VCardUtils.h"

namespace RingClientUWP
{

delegate void SelectIndex(int index);
delegate void LoadProfileImage();

namespace Configuration
{

/* delegates */

public ref class UserPreferences sealed
{
public:
    /* singleton */
    static property UserPreferences^ instance
    {
        UserPreferences^ get()
        {
            static UserPreferences^ instance_ = ref new UserPreferences();
            return instance_;
        }
    }

    /* properties */
    property int            PREF_ACCOUNT_INDEX;
    property String^        PREF_ACCOUNT_ID;
    property uint64_t       PREF_PROFILE_UID;
    property bool           PREF_PROFILE_HASPHOTO;
    property String^        PREF_PROFILE_FN;

    property bool           profileImageLoaded;

    /* functions */
    void                    save();
    void                    load();
    String^                 Stringify();
    void                    Destringify(String^ data);
    VCardUtils::VCard^      getVCard();
    void                    saveProfileToVCard();

    void                    raiseSelectIndex(int index);

internal:
    void                    sendVCard(std::string callID);

    /* events */
    event SelectIndex^      selectIndex;
    event LoadProfileImage^ loadProfileImage;

private:
    bool loaded_;
    VCardUtils::VCard^ vCard_;
    UserPreferences();
};

task<Windows::UI::Xaml::Media::Imaging::BitmapImage^> getProfileImageAsync();
//void    getProfileImageAsync();
}
}
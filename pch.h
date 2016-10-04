﻿#pragma once
/**************************************************************************
* Copyright (C) 2016 by Savoir-faire Linux                                *
* Author: Jäger Nicolas <nicolas.jager@savoirfairelinux.com>              *
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

/* standard system include files. */
#include <iomanip>
#include <ppltasks.h>
#include <queue>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

/* project's globals */
#include "Globals.h"

/* required by generated headers. */
#include "App.xaml.h"
#include "Account.h"
#include "AccountsViewModel.h"
#include "Contact.h"
#include "ContactsViewModel.h"
#include "Conversation.h"
#include "MainPage.xaml.h"
#include "SmartPanelItem.h"
#include "SmartPanelItemsViewModel.h"

/* ensure to be accessed from anywhere */
#include "RingD.h"
#include "RingDebug.h"
#include "Utils.h"
#include "UserPreferences.h"

/* video headers */
#include "Video.h"
#include "VideoCaptureManager.h"
#include "VideoManager.h"
#include "VideoRendererManager.h"
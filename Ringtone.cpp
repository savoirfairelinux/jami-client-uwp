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

using namespace Windows::UI::Core;

using namespace RingClientUWP;

Ringtone::Ringtone(String^ fileName)
{
    fileName_ = fileName;
    CreateGraph()
        .then([this](task<void> t) {
        t.get();
        CreateDefaultDeviceOutputNode()
            .then([this](task<void> t) {
            t.get();
            CreateFileInputNode()
                .then([](task<void> t) {
                t.get();
            });
        });
    });
}

void
Ringtone::Start()
{
    _graph->Start();
}

void
Ringtone::Stop()
{
    _graph->Stop();
    _graph->ResetAllNodes();
}

task<void>
Ringtone::CreateGraph()
{
    AudioGraphSettings^ settings = ref new AudioGraphSettings(Windows::Media::Render::AudioRenderCategory::Media);
    return create_task(AudioGraph::CreateAsync(settings))
        .then([=](CreateAudioGraphResult^ result){
        if (result->Status != AudioGraphCreationStatus::Success) {
            MSG_("CreateGraph failed");
        }
        _graph = result->Graph;
    });
}

task<void>
Ringtone::CreateDefaultDeviceOutputNode()
{
    return create_task(_graph->CreateDeviceOutputNodeAsync())
        .then([=](CreateAudioDeviceOutputNodeResult^ result){
        if (result->Status != AudioDeviceNodeCreationStatus::Success) {
            MSG_("CreateDefaultDeviceOutputNode failed");
        }
        _deviceOutputNode = result->DeviceOutputNode;
    });
}

task<void>
Ringtone::CreateFileInputNode()
{
    Windows::ApplicationModel::Package^ package = Windows::ApplicationModel::Package::Current;
    Windows::Storage::StorageFolder^ installedLocation = package->InstalledLocation;
    return create_task(installedLocation->GetFolderAsync("Assets"))
        .then([=](StorageFolder^ assetsFolder){
        create_task(assetsFolder->GetFileAsync(fileName_))
            .then([=](StorageFile^ ringtoneFile){
            // functions as a Background task but will force a SMTC panel into the thumbnail
            //mp = ref new MediaPlayer();
            //mp->Source = MediaSource::CreateFromStorageFile(ringtoneFile);
            ////mp->CommandManager->IsEnabled = false;
            //mp->IsLoopingEnabled = true;
            //mp->Play();
            create_task(_graph->CreateFileInputNodeAsync(ringtoneFile))
                .then([=](CreateAudioFileInputNodeResult^ result){
                if (result->Status != AudioFileNodeCreationStatus::Success) {
                    MSG_("CreateFileInputNode failed");
                }
                _fileInputNode = result->FileInputNode;
                _fileInputNode->LoopCount = nullptr;
                _fileInputNode->AddOutgoingConnection(_deviceOutputNode);
            });
        });
    });
}
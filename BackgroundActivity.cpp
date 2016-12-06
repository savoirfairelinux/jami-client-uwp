#include "pch.h"
#include "MainPage.xaml.h"
#include "BackgroundActivity.h"

using namespace RingClientUWP;
using namespace Windows::ApplicationModel::Background;

void BackgroundActivity::Run(IBackgroundTaskInstance^ taskInstance)
{
    auto details = safe_cast<ToastNotificationActionTriggerDetail^>(taskInstance->TriggerDetails);

    if (details != nullptr) {
        auto userInput = details->UserInput;
        std::string args = Utils::toString(details->Argument);
        RingD::instance->refuseIncommingCall(Utils::toPlatformString(args));
    }
}

void BackgroundActivity::Start(IBackgroundTaskInstance^ taskInstance)
{
    auto activity = ref new BackgroundActivity();
    activity->Run(taskInstance);
}
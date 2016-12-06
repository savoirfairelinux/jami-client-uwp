#pragma once

namespace RingClientUWP
{
    [Windows::Foundation::Metadata::WebHostHidden]
    public ref class BackgroundActivity sealed
    {
    public:
        void Run(Windows::ApplicationModel::Background::IBackgroundTaskInstance^ taskInstance);
        static void Start(Windows::ApplicationModel::Background::IBackgroundTaskInstance^ taskInstance);
    };
}

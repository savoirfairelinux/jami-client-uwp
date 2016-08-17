#pragma once

#include "VideoPage.g.h"

using namespace Windows::Media::Capture;
using namespace Windows::UI::Xaml::Navigation;

namespace RingClientUWP
{
/* delegate */
delegate void PressHangUpCall();
delegate void PauseCall();
delegate void ChatPanelCall();
delegate void AddContactCall();
delegate void TransferCall();
delegate void SwitchMicrophoneStateCall();
delegate void SwitchVideoStateCall();
delegate void ReccordVideoCall();
delegate void QualityVideoLevelCall();

namespace Views
{

public ref class VideoPage sealed
{
public:
    VideoPage();
    property bool barFading
    {
        bool get()
        {
            return barFading_;
        }
        void set(bool value)
        {
            barFading_ = value;
        }
    }

protected:
    // Template Support
    virtual void OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e) override;
    /*virtual void OnNavigatedFrom(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e) override;*/


internal:
    /* events */
    event PressHangUpCall^ pressHangUpCall;
    event PauseCall^ pauseCall;
    event ChatPanelCall^ chatPanelCall;
    event AddContactCall^ addContactCall;
    event TransferCall^ transferCall;
    event SwitchMicrophoneStateCall^ switchMicrophoneStateCall;
    event SwitchVideoStateCall^ switchVideoStateCall;
    event ReccordVideoCall^ reccordVideoCall;
    event QualityVideoLevelCall^ qualityVideoLevelCall;

private:
    bool barFading_;

    void updatePageContent();

    //void OnNavigatedToPage(Object^ sender, NavigationEventArgs^ e);

    void Button_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    void _btnCancel__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    void _btnHangUp__Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e);
    void _btnPause__Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e);
    void _btnChat__Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e);
    void _btnAddFriend__Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e);
    void _btnSwitch__Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e);
    void _btnMicrophone__Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e);
    void _btnMemo__Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e);
    void _btnHQ__Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e);
    void _btnVideo__Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e);
    void _videoControl__PointerMoved(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e);
    void btnAny_entered(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e);
    void btnAny_exited(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e);
};
}
}
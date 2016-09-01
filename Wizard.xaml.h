#pragma once

#include "Wizard.g.h"

namespace RingClientUWP
{
namespace Views
{

public ref class Wizard sealed
{
public:
    Wizard();
private:
    void _createAccountYes__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    void _showCreateAccountMenuBtn__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    void _showAddAccountMenuBtn__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    void _avatarWebcamCaptureBtn__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
};

}
}
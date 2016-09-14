//
// RingMessageBubble.xaml.h
// Declaration of the RingMessageBubble class.
//

#pragma once

#include "RingMessageBubble.g.h"

namespace RingClientUWP
{


ref class MainPage;
namespace Controls {
public ref class RingMessageBubble sealed
{
public:
    RingMessageBubble();

    property Windows::UI::Xaml::DependencyProperty^ DPOnPageProperty
    {
        Windows::UI::Xaml::DependencyProperty^ get() {
            return _DPOnPage;
        }
    }

    property Platform::String^ DPOnPage
    {
        Platform::String^ get()
        {
            return dynamic_cast<Platform::String^>(GetValue(DPOnPageProperty));
        }
        void set(Platform::String^ value)
        {
            SetValue(DPOnPageProperty, value);
        }
    }


private:
    Windows::UI::Xaml::DependencyProperty^ _DPOnPage;
};
}
}

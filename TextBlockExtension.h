//
// TextBlockExtension.xaml.h
// Declaration of the TextBlockExtension class
//
#pragma once

using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Interop;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Interop;
using namespace Windows::UI::Xaml::Documents;
using namespace Platform;

#include "pch.h"
#include <regex>

namespace RingClientUWP
{

namespace UserAndCustomControls {

public ref class TextBlockExtension sealed : public Control
{
private:
    static DependencyProperty^ FormattedTextProperty;

public:
    TextBlockExtension::TextBlockExtension();

    static String^ GetFormattedText(DependencyObject^ obj) {
        return (String^)obj->GetValue(FormattedTextProperty);
    };
    static void SetFormattedText(DependencyObject^ obj, String^ value) {
        obj->SetValue(FormattedTextProperty, value);
    }

};

}
}
//
// TextBlockExtension.cpp
// Implementation of the TextBlockExtension class
//

#include "pch.h"
#include "TextBlockExtension.h"

using namespace RingClientUWP;
using namespace UserAndCustomControls;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Media::Imaging;
using namespace Windows::UI::Xaml::Navigation;

using namespace Windows::UI::Xaml::Interop;
using namespace Windows::UI::Xaml::Documents;

TextBlockExtension::TextBlockExtension()
{
}

DependencyProperty^ TextBlockExtension::FormattedTextProperty =
DependencyProperty::Register("FormattedText", String::typeid, TextBlockExtension::typeid,
    ref new PropertyMetadata(nullptr,
        ref new PropertyChangedCallback([](DependencyObject^ sender, DependencyPropertyChangedEventArgs^ e)
{
    auto text = Utils::toString(static_cast<String^>(e->NewValue));
    auto rtextBlock = static_cast<RichTextBlock^>(sender);
    rtextBlock->Blocks->Clear();
    auto paragraph = ref new Paragraph();
    if (paragraph != nullptr) {
        paragraph->Inlines->Clear();
        std::regex abs_url_regex("((((http[s]?):\/\/))[^\ ]+)", std::regex_constants::icase);
        std::regex url_regex("((((http[s]?):\/\/)|www)[^\ ]+)", std::regex_constants::icase);
        std::regex image_regex("[^\ ]+\.(gif|jpg|jpeg|png)");
        std::regex word_regex("([^\ ]+)");
        std::sregex_iterator next(text.begin(), text.end(), word_regex);
        std::sregex_iterator end;
        while (next != end) {
            std::smatch match = *next;
            Run^ run = ref new Run();
            Run^ preLink = ref new Run();
            preLink->Text = " ";
            Run^ postLink = ref new Run();
            postLink->Text = " ";
            auto str = Utils::toPlatformString(match.str());
            run->Text = str;
            if (std::regex_match(match.str(), url_regex)) {
                // it's a url so make it absolute if it's not already
                if (!std::regex_match(match.str(), abs_url_regex))
                    str = "http://" + str;
                auto uri = ref new Uri(str);
                if (std::regex_match(match.str(), image_regex)) {
                    Image^ image = ref new Image();
                    image->Source = ref new BitmapImage(uri);
                    image->MaxWidth = 300;
                    image->Margin = Windows::UI::Xaml::Thickness(4.0, 6.0, 4.0, 6.0);
                    InlineUIContainer^ iuc = ref new InlineUIContainer();
                    iuc->Child = image;
                    paragraph->Inlines->Append(iuc);
                }
                else {
                    Hyperlink^ link = ref new Hyperlink();
                    link->NavigateUri = uri;
                    link->Foreground = ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::LightCyan);
                    link->Inlines->Append(run);
                    paragraph->Inlines->Append(preLink);
                    paragraph->Inlines->Append(link);
                    paragraph->Inlines->Append(postLink);
                }
            }
            else {
                paragraph->Inlines->Append(run);
            }
            next++;
        }
        rtextBlock->Blocks->Append(paragraph);
    }
})));
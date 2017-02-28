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
using namespace Windows::UI::Core;

TextBlockExtension::TextBlockExtension()
{}

void
embeddedLoadCompleted(Object^ sender, NavigationEventArgs^ e)
{
    RingD::instance->raiseMessageDataLoaded();
}

void
imageLoadCompleted(Object^ sender, RoutedEventArgs^ e)
{
    RingD::instance->raiseMessageDataLoaded();
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
        auto regex_opts = std::regex_constants::icase;
        std::regex abs_url_regex("((((https?):\/\/))[^\ ]+)", regex_opts);
        std::regex url_regex("((((https?):\/\/)|www)[^\ ]+)", regex_opts);
        std::regex youtube_regex("(https?\:\/\/)?(www\.)?(youtube\.com|youtu\.?be)([^\=]+)=([^\ ]+)", regex_opts);
        std::regex image_regex("[^\ ]+\.(gif|jpg|jpeg|png)", regex_opts);
        std::regex word_regex("([^\ ]+)", regex_opts);
        std::sregex_iterator next(text.begin(), text.end(), word_regex);
        std::sregex_iterator end;
        unsigned elementCount = 0;
        while (next != end) {
            std::smatch match = *next;
            Run^ run = ref new Run();
            Run^ postSpace = ref new Run();
            postSpace->Text = " ";
            auto str = Utils::toPlatformString(match.str());
            run->Text = str;
            if (std::regex_match(match.str(), url_regex)) {
                // it's a url so make it absolute if it's not already
                if (!std::regex_match(match.str(), abs_url_regex))
                    str = "http://" + str;
                auto uri = ref new Uri(str);
                // youtube embed
                std::smatch groups;
                std::string sub(match.str());
                if (std::regex_search(sub, groups, youtube_regex)) {
                    WebView^ webview = ref new WebView();
                    webview->Width = 372;
                    webview->Height = 208;
                    auto videoId = Utils::toPlatformString(groups[5].str());
                    String^ bodyStr = "<style>iframe,html,body{border:0px;margin:0px;padding:0px;overflow-y:hidden;}</style>";
                    auto iframeStr = "<iframe width='372' height='210' src='https://www.youtube.com/embed/" +
                                    videoId + "' frameborder='0'></iframe>";
                    webview->Margin = Windows::UI::Xaml::Thickness(4.0, 6.0, 4.0, 4.0);
                    webview->NavigateToString(bodyStr + iframeStr);
                    webview->LoadCompleted += ref new LoadCompletedEventHandler(&embeddedLoadCompleted);
                    InlineUIContainer^ iuc = ref new InlineUIContainer();
                    iuc->Child = webview;
                    if (elementCount > 0)
                        paragraph->Inlines->Append(ref new LineBreak());
                    paragraph->Inlines->Append(iuc);
                }
                // image
                else if (std::regex_match(match.str(), image_regex)) {
                    Image^ image = ref new Image();
                    image->Source = ref new BitmapImage(uri);
                    image->MaxWidth = 300;
                    image->Margin = Windows::UI::Xaml::Thickness(4.0, 6.0, 4.0, 4.0);
                    image->Loaded += ref new RoutedEventHandler(&imageLoadCompleted);
                    InlineUIContainer^ iuc = ref new InlineUIContainer();
                    iuc->Child = image;
                    if (elementCount > 0)
                        paragraph->Inlines->Append(ref new LineBreak());
                    paragraph->Inlines->Append(iuc);
                }
                // link
                else {
                    Hyperlink^ link = ref new Hyperlink();
                    link->NavigateUri = uri;
                    link->Foreground = ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::LightCyan);
                    link->Inlines->Append(run);
                    paragraph->Inlines->Append(link);
                }
            }
            // text chunk
            else {
                paragraph->Inlines->Append(run);
            }
            // add space after everything
            paragraph->Inlines->Append(postSpace);
            elementCount++;
            next++;
        }
        rtextBlock->Blocks->Append(paragraph);
    }
})));
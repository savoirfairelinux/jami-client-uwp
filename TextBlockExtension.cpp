/**************************************************************************
* Copyright (C) 2016 by Savoir-faire Linux                                *
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
#include "TextBlockExtension.h"
#include "HttpUtils.h"

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

void
appendUrlToParagraph(Uri^ uri, Run^ run, Paragraph^ paragraph, bool& frameAdded, String^ str)
{
    Hyperlink^ link = ref new Hyperlink();
    link->NavigateUri = uri;
    link->Foreground = ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::DarkBlue);
    run->Text = str;
    link->Inlines->Append(run);
    if (frameAdded) {
        paragraph->Inlines->Append(ref new LineBreak());
        frameAdded = false;
    }
    paragraph->Inlines->Append(link);
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
        static auto regex_opts = std::regex_constants::icase;
        const static std::regex abs_url_regex("((((https?):\/\/))[^\ ]+)", regex_opts);
        const static std::regex url_regex("((((https?):\/\/)|www)[^\ ]+)", regex_opts);
        const static std::regex youtube_regex("(https?\:\/\/)?(www\.)?(youtube\.com|youtu\.?be)([^\=]+)=([^\ ]+)", regex_opts);
        const static std::regex image_regex("[^\ ]+\.(gif|jpg|jpeg|png)", regex_opts);
        const static std::regex word_regex("([^\ ]+)", regex_opts);

        // emojis
        const static std::regex emoji_smile("(^|\s):-?\\)(?!\S)|:smile", regex_opts);
        const static std::regex emoji_wink("(^|\s);-?\\)(?!\S)|:wink", regex_opts);
        const static std::regex emoji_grin("(^|\s):-?D(?!\S)|:grin", regex_opts);
        const static std::regex emoji_meh("(^|\s):-?\\)(?!\S)|:meh", regex_opts);
        const static std::regex emoji_sad("(^|\s):-?\\((?!\S)|:sad", regex_opts);
        const static std::regex emoji_crazy("(^|\s);-?P(?!\S)", regex_opts);
        const static std::regex emoji_tongue("(^|\s):-?P(?!\S)", regex_opts);
        const static std::regex emoji_hmm("(^|\s):-?\\/(?!\S)");
        const static std::regex emoji_rofl(":rofl", regex_opts);
        const static std::regex emoji_heart("(^|\s)<3(?!\S)|:heart", regex_opts);

        std::sregex_iterator next(text.begin(), text.end(), word_regex);
        std::sregex_iterator end;
        unsigned elementCount = 0;
        bool frameAdded = false;
        while (next != end) {
            Run^ run = ref new Run();
            Run^ postSpace = ref new Run();
            postSpace->Text = " ";
            std::smatch match = *next;
            auto str = Utils::toPlatformString(match.str());
            if (std::regex_match(match.str(), url_regex)) {
                // it's a url so make it absolute if it's not already
                if (!std::regex_match(match.str(), abs_url_regex))
                    str = "http://" + str;
                auto uri = ref new Uri(str);

                // youtube embed
                std::smatch groups;
                std::string sub(match.str());
                if (std::regex_search(sub, groups, youtube_regex)) {
                    if (Utils::hasInternet()) {
                        WebView^ webview = ref new WebView();
                        webview->Width = 372;
                        webview->Height = 208;
                        auto videoId = Utils::toPlatformString(groups[5].str());
                        String^ bodyStr = "<style>iframe,html,body{border:0px;margin:0px;padding:0px;overflow-y:hidden;}</style>";
                        auto iframeStr = "<iframe width='372' height='210' src='https://www.youtube.com/embed/" +
                            videoId + "' frameborder='0'></iframe>";
                        webview->Margin = Windows::UI::Xaml::Thickness(2.0, 6.0, 4.0, 4.0);
                        webview->NavigateToString(bodyStr + iframeStr);
                        webview->LoadCompleted += ref new LoadCompletedEventHandler(&embeddedLoadCompleted);
                        InlineUIContainer^ iuc = ref new InlineUIContainer();
                        iuc->Child = webview;
                        if (elementCount > 0)
                            paragraph->Inlines->Append(ref new LineBreak());
                        paragraph->Inlines->Append(iuc);
                        frameAdded = true;
                    }
                    else {
                        appendUrlToParagraph(uri, run, paragraph, frameAdded, str);
                    }
                }
                // image
                else if (std::regex_match(match.str(), image_regex)) {
                    if (Utils::hasInternet()) {
                        Image^ image = ref new Image();
                        image->Source = ref new BitmapImage(uri);
                        image->MaxWidth = 300;
                        image->Margin = Windows::UI::Xaml::Thickness(2.0, 6.0, 4.0, 4.0);
                        image->Loaded += ref new RoutedEventHandler(&imageLoadCompleted);
                        InlineUIContainer^ iuc = ref new InlineUIContainer();
                        iuc->Child = image;
                        if (elementCount > 0)
                            paragraph->Inlines->Append(ref new LineBreak());
                        paragraph->Inlines->Append(iuc);
                        frameAdded = true;
                    }
                    else {
                        appendUrlToParagraph(uri, run, paragraph, frameAdded, str);
                    }
                }
                else {
                    appendUrlToParagraph(uri, run, paragraph, frameAdded, str);
                }
            }
            // text chunk
            else {
                // convert text emoji representations to unicode
                auto ansiEmojiString = std::regex_replace(match.str(), emoji_smile, "ðŸ™‚");
                ansiEmojiString = std::regex_replace(ansiEmojiString, emoji_wink, "ðŸ˜‰");
                ansiEmojiString = std::regex_replace(ansiEmojiString, emoji_grin, "ðŸ˜");
                ansiEmojiString = std::regex_replace(ansiEmojiString, emoji_meh, "ðŸ˜");
                ansiEmojiString = std::regex_replace(ansiEmojiString, emoji_sad, "â˜¹");
                ansiEmojiString = std::regex_replace(ansiEmojiString, emoji_crazy, "ðŸ˜œ");
                ansiEmojiString = std::regex_replace(ansiEmojiString, emoji_tongue, "ðŸ˜›");
                ansiEmojiString = std::regex_replace(ansiEmojiString, emoji_hmm, "ðŸ¤”");
                ansiEmojiString = std::regex_replace(ansiEmojiString, emoji_rofl, "ðŸ¤£");
                ansiEmojiString = std::regex_replace(ansiEmojiString, emoji_heart, "â¤");
                run->Text = Utils::toPlatformString(ansiEmojiString);
                if (frameAdded) {
                    paragraph->Inlines->Append(ref new LineBreak());
                    frameAdded = false;
                }
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
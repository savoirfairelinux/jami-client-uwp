

#include "pch.h"
#include "RingMessageBubble.xaml.h"
#include "MainPage.xaml.h"
//#include "CallModel.h"

using namespace RingClientUWP::Controls;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Documents;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;


RingMessageBubble::RingMessageBubble()
{
    OutputDebugString(L"@RingMessageBubble\n@");
    InitializeComponent();

    Run^ inlineText = ref new Run();
    inlineText->Text = "Lorem ipsum  dolor sit amet, Visit our HTML tutorial</a>  consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.";


    /*
         Image^ image = ref new Image();
         image->Source = "";*/


    Paragraph^ paragraph = ref new Paragraph();
    paragraph->Inlines->Append(inlineText);
    paragraph->TextAlignment = TextAlignment::Justify;
//   paragraph->Inlines->Append(link);

    _contactMsg_->Blocks->Append(paragraph);
    //_myMsg_->Blocks->Append(paragraph);

}
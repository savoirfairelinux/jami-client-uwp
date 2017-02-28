#pragma once
#include <pch.h>

using namespace Platform;
using namespace Windows::Web::Http;
using namespace Windows::Web::Http::Filters;
using namespace Windows::Web::Http::Headers;

namespace RingClientUWP
{
namespace Utils
{

HttpClient^ httpClient = ref new HttpClient();

//bool
//isVideoContent(Uri^ uri)
//{
//    auto res = httpClient->GetBufferAsync(uri).then
//    ([uri](task<HttpResponseMessage^> responseTask) {
//        bool isVideoContentResult = false;
//        try {
//            MSG_(uri->ToString());
//            auto httpResponse = responseTask.get();
//            auto httpHeaders = httpResponse->Headers;
//            /*auto httpHeaderString = httpHeaders->ToString();
//            MSG_("header string:");
//            MSG_("\n" + httpHeaderString);
//
//            auto httpContentHeaders = httpResponse->Content->Headers;
//            auto httpContentHeaderString = httpContentHeaders->ToString();
//            MSG_("content header string:");
//            MSG_("\n" + httpContentHeaderString);*/
//
//            /*create_task(httpResponse->Content->ReadAsStringAsync()).then([=](String^ contentString) {
//            MSG_(uri->ToString() + ": ");
//            MSG_(contentString);
//            }, task_continuation_context::use_current());*/
//            return isVideoContentResult;
//        }
//        catch (Platform::Exception^ e) {
//            EXC_(e);
//        }
//        return false;
//    }, task_continuation_context::use_current());
//}

}

}
#pragma once
#include <pch.h>

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;

namespace RingClientUWP
{

public ref class UserModel sealed
{
internal:
    /* singleton */
    static property UserModel^ instance
    {
        UserModel^ get()
        {
            static UserModel^ instance_ = ref new UserModel();
            return instance_;
        }
    }

    property String^ firstName;
    property String^ lastName;

    void getUserData();

private:
    UserModel() { };
};

}
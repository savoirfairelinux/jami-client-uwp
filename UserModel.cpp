#include "pch.h"

#include "UserModel.h"

using namespace RingClientUWP;

void
UserModel::getUserData()
{
    /*create_task(User::FindAllAsync())
        .then([=](IVectorView<User^>^ users) {
        for (size_t index = 0; index < users->Size; index++) {
            auto user = users->GetAt(index);
            if (user->AuthenticationStatus == UserAuthenticationStatus::LocallyAuthenticated &&
                user->Type == UserType::LocalUser) {
                User^ currentUser = user;
                create_task(currentUser->GetPropertyAsync(KnownUserProperties::FirstName))
                    .then([=](Object^ result) {
                    firstName = safe_cast<String^>(result);
                });
                create_task(currentUser->GetPropertyAsync(KnownUserProperties::LastName))
                    .then([&](Object^ result) {
                    lastName = safe_cast<String^>(result);
                });
            }
        }
    });*/
}
/**************************************************************************
* Copyright (C) 2016 by Savoir-faire Linux                                *
* Author: Jäger Nicolas <nicolas.jager@savoirfairelinux.com>              *
* Author: Traczyk Andreas <andreas.traczyk@savoirfairelinux.com>          *
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

#include "WelcomePage.xaml.h"
#include "AboutPage.xaml.h"

#include "qrencode.h"
#include <MemoryBuffer.h>   // IMemoryBufferByteAccess

using namespace RingClientUWP;
using namespace RingClientUWP::Views;
using namespace RingClientUWP::ViewModel;

using namespace Windows::UI::ViewManagement;
using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::Storage;
using namespace Windows::UI::Xaml::Media::Imaging;
using namespace Windows::Graphics::Imaging;

WelcomePage::WelcomePage()
{
    InitializeComponent();

    RingD::instance->shareRequested += ref new ShareRequested(this, &WelcomePage::generateShareData);
};

void
WelcomePage::_welcomeAboutButton__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    auto rootFrame = dynamic_cast<Windows::UI::Xaml::Controls::Frame^>(Window::Current->Content);
    rootFrame->Navigate(Windows::UI::Xaml::Interop::TypeName(Views::AboutPage::typeid));
}

void
WelcomePage::generateShareData()
{
    auto ringId = AccountListItemsViewModel::instance->_selectedItem->_account->ringID_;
    auto ringId2 = Utils::toString(ringId);

    _ringId_->Text = ringId;
    _welcomeAboutButton_->Margin = Windows::UI::Xaml::Thickness(0.0, 24.0, 0.0, 0.0);

    auto qrcode = QRcode_encodeString(ringId2.c_str(),
        0, //Let the version be decided by libqrencode
        QR_ECLEVEL_L, // Lowest level of error correction
        QR_MODE_8, // 8-bit data mode
        1);

    if (!qrcode) {
        WNG_("Failed to generate QR code: ");
        return;
    }

    const int STRETCH_FACTOR = 4;
    const int widthQrCode = qrcode->width;
    const int widthBitmap = STRETCH_FACTOR * widthQrCode;

    unsigned char* qrdata = qrcode->data;

    auto frame = ref new SoftwareBitmap(BitmapPixelFormat::Bgra8, widthBitmap, widthBitmap, BitmapAlphaMode::Premultiplied);

    const int BYTES_PER_PIXEL = 4;


    BitmapBuffer^ buffer = frame->LockBuffer(BitmapBufferAccessMode::ReadWrite);
    IMemoryBufferReference^ reference = buffer->CreateReference();

    Microsoft::WRL::ComPtr<IMemoryBufferByteAccess> byteAccess;
    if (SUCCEEDED(reinterpret_cast<IUnknown*>(reference)->QueryInterface(IID_PPV_ARGS(&byteAccess))))
    {
        byte* data;
        unsigned capacity;
        byteAccess->GetBuffer(&data, &capacity);

        auto desc = buffer->GetPlaneDescription(0);

        unsigned char* row, *p;
        p = qrcode->data;

        for (int u = 0; u < widthBitmap; u++) {
            for (int v = 0; v < widthBitmap; v++) {
                int x = static_cast<int>((float)u / (float)widthBitmap * (float)widthQrCode);
                int y = static_cast<int>((float)v / (float)widthBitmap * (float)widthQrCode);

                auto currPixelRow = desc.StartIndex + desc.Stride * u + BYTES_PER_PIXEL * v;
                row = (p + (y * widthQrCode));

                if (*(row + x) & 0x1) {
                    data[currPixelRow + 3] = 255;
                }
            }
        }

    }
    delete reference;
    delete buffer;

    auto sbSource = ref new Media::Imaging::SoftwareBitmapSource();

    sbSource->SetBitmapAsync(frame);

    _selectedAccountQrCode_->Source = sbSource;
    _selectedAccountQrCode_->Visibility = VIS::Visible;
}
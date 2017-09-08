/**************************************************************************
* Copyright (C) 2016 by Savoir-faire Linux                                *
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

#pragma once

#include "StringUtils.h"
#include "Utils.h"

using namespace Platform;
using namespace Platform::Collections;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Security::Cryptography;
using namespace Windows::Security::Cryptography::Core;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Media::Imaging;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;

namespace RingClientUWP
{

using VIS = Windows::UI::Xaml::Visibility;

namespace Utils
{
namespace xaml
{

#undef max
#undef min

Windows::UI::Xaml::FrameworkElement^
FindVisualChildByName(DependencyObject^ obj, String^ name)
{
    FrameworkElement^ ret;
    int numChildren = VisualTreeHelper::GetChildrenCount(obj);
    for (int i = 0; i < numChildren; i++)
    {
        auto objChild = VisualTreeHelper::GetChild(obj, i);
        auto child = safe_cast<FrameworkElement^>(objChild);
        if (child != nullptr && child->Name == name)
        {
            return child;
        }

        ret = FindVisualChildByName(objChild, name);
        if (ret != nullptr)
            break;
    }
    return ret;
}

Windows::UI::Color
ColorFromString(String^ s)
{
    int a, r, g, b;
    if (sscanf_s(Utils::toString(s).c_str(), "#%2x%2x%2x%2x", &a, &r, &g, &b) == 4)
        return Windows::UI::ColorHelper::FromArgb(a, r, g, b);
    else
        return Windows::UI::ColorHelper::FromArgb(255, 0, 0, 0);
}

// 16 Web ready colors
// https://material.io/guidelines/style/color.html#color-color-palette
auto colorStrings = ref new Vector<String^>({
    "#fff44336", // { 0.956862,     0.262745, 0.211764 }    #fff44336   // red 244, green 67,   blue 54     (red)
    "#ffe91e63", // { 0.913725,     0.117647, 0.388235 }    #ffe91e63   // red 233, green 30,   blue 99     (pink)
    "#ff9c27b0", // { 0.611764,     0.152941, 0.690196 }    #ff9c27b0   // red 156, green 39,   blue 176    (purple)
    "#ff673ab7", // { 0.956862,     0.262745, 0.211764 }    #ff673ab7   // red 244, green 67,   blue 54     (deep purple)
    "#ff3f51b5", // { 0.403921,     0.227450, 0.717647 }    #ff3f51b5   // red 103, green 58,   blue 183    (indigo)
    "#ff2196f3", // { 0.247058,     0.317647, 0.211764 }    #ff2196f3   // red 63,  green 81,   blue 54     (blue)
    "#ff00bcd4", // { 0, 0.737254,  0.831372, 1.0 }         #ff00bcd4   // red 0,   green 188,  blue 212    (cyan)
    "#ff009688", // { 0, 0.588235,  0.533333, 1.0 }         #ff009688   // red 0,   green 150,  blue 136    (teal)
    "#ff4caf50", // { 0.298039,     0.682745, 0.313725 }    #ff4caf50   // red 76,  green 175,  blue 80     (green)
    "#ff8bc34a", // { 0.545098,     0.764705, 0.290196 }    #ff8bc34a   // red 138, green 194,  blue 73     (light green)
    "#ff9e9e9e", // { 0.619607,     0.619607, 0.619607 }    #ff9e9e9e   // red 157, green 157,  blue 157    (grey)
    "#ffcddc39", // { 0.803921,     0.862745, 0.223529 }    #ffcddc39   // red 204, green 219,  blue 56     (lime)
    "#ffffc107", // { 1, 0.756862,  0.027450, 1.0 }         #ffffc107   // red 255, green 192,  blue 6      (amber)
    "#ffff5722", // { 1, 0.341176,  0.133333, 1.0 }         #ffff5722   // red 255, green 86,   blue 33     (deep orange)
    "#ff795548", // { 0.474509,     0.333333, 0.282352 }    #ff795548   // red 120, green 84,   blue 71     (brown)
    "#ff607d8b"  // { 0.376470,     0.490196, 0.545098 }    #ff607d8b   // red 95,  green 124,  blue 138,   (blue grey)
});

unsigned
mapToRange(unsigned value, unsigned from_low, unsigned from_high, unsigned to_low, unsigned to_high)
{
    unsigned from_range = from_high - from_low;
    unsigned to_range = to_high - to_low;
    auto factor = static_cast<double>(to_range) / static_cast<double>(from_range);
    return static_cast<unsigned>(factor * static_cast<double>(value));
}

unsigned
hashToRange(std::string hash, unsigned to_low, unsigned to_high)
{
    unsigned x;
    std::stringstream ss;
    ss << std::hex << hash.substr(0, 2);
    ss >> x;
    return mapToRange(static_cast<unsigned>(x), 0, 255, to_low, to_high);
}

unsigned int
generateRandomNumberInRange(uint32_t min, uint32_t max)
{
    auto rnd = static_cast<float>(CryptographicBuffer::GenerateRandomNumber());
    auto normalized = rnd / std::numeric_limits<unsigned int>::max();
    auto amp = static_cast<float>(max - min);
    return static_cast<unsigned int>(min + normalized * amp);
}

String^
getRandomColorString()
{
    auto index = generateRandomNumberInRange(0, colorStrings->Size - 1);
    return colorStrings->GetAt(index);
}

String^
getAvatarColorStringFromString(String^ str)
{
    auto index = hashToRange(toString(computeMD5(str)), 0, colorStrings->Size - 1);
    return colorStrings->GetAt(index);
}

SolidColorBrush^
solidColorBrushFromString(String^ colorString)
{
    return ref new SolidColorBrush(ColorFromString(colorString));
}

} /*namespace xaml*/

} /*namespace Utils*/

} /*namespace RingClientUWP*/

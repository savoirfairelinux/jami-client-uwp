/**************************************************************************
* Copyright (C) 2016 by Savoir-faire Linux                                *
* Author: Jäger Nicolas <nicolas.jager@savoirfairelinux.com>              *
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

#include "RingConsolePanel.xaml.h"

using namespace RingClientUWP;
using namespace RingClientUWP::Views;
using namespace RingClientUWP::ViewModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml::Documents;

RingConsolePanel::RingConsolePanel()
{
    InitializeComponent();

    RingDebug::instance->messageToScreen += ref new debugMessageToScreen([this](Platform::String^ message) {
        CoreApplication::MainView->CoreWindow->Dispatcher->RunAsync(CoreDispatcherPriority::High,
            ref new DispatchedHandler([=]()
        {
            output(message);
        }));
    });
}

void
RingConsolePanel::output(Platform::String^ message)
{
    try {
        Run^ inlineText = ref new Run();
        inlineText->Text = message;
        Paragraph^ paragraph = ref new Paragraph();
        paragraph->Inlines->Append(inlineText);
        _debugWindowOutput_->Blocks->Append(paragraph);
        _scrollView_->UpdateLayout();
        _scrollView_->ScrollToVerticalOffset(_scrollView_->ScrollableHeight);
    }
    catch (Platform::Exception^ e) {
        return;
    }
}

void RingConsolePanel::_btnSendDbgCmd__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    sendCommand();
}


void RingConsolePanel::_sendDbgCmd__KeyDown(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e)
{
    if (e->Key == Windows::System::VirtualKey::Enter && _tBoxDbg_->Text != "") {
        sendCommand();
    }
    else if (e->Key == Windows::System::VirtualKey::PageUp) {

        if (historyLevel < 1)
            return;
        if (historyLevel == historyCmds.Size)
            currentCmd = _tBoxDbg_->Text;
        historyLevel--;
        _tBoxDbg_->Text = historyCmds.GetAt(historyLevel);

    }
    else if (e->Key == Windows::System::VirtualKey::PageDown) {
        if (historyLevel < historyCmds.Size) {
            _tBoxDbg_->Text = historyCmds.GetAt(historyLevel);
            historyLevel++;

        }
        else {
            _tBoxDbg_->Text = currentCmd;
        }
        return;
    }
}

/*\ ADD EACH NEW COMMAND TO THE HELP LIST \*/
void RingConsolePanel::sendCommand()
{
    auto inputConst_str = Utils::toString(_tBoxDbg_->Text);
    if (inputConst_str.empty())
        return;

    auto inputConst_cstr = inputConst_str.c_str();
    char* input_cstr = _strdup(inputConst_cstr); // full string
    char* input_cstr_nextToken; // tokenized string
    char* cmd_cstr = strtok_s(input_cstr, " ", &input_cstr_nextToken);
    char* parameter1_cstr = strtok_s(input_cstr_nextToken, " ", &input_cstr_nextToken);
    // parameter2...

    if (!cmd_cstr)
        return;

    std::string input(cmd_cstr);
    std::string parameter1;

    if (parameter1_cstr)
        parameter1 = parameter1_cstr;

    free(input_cstr);
    free(input_cstr_nextToken);
    //free((char*)inputConst_cstr);

    addCommandToHistory();
    historyLevel++;
    _tBoxDbg_->Text = "";
    currentCmd = "";
    historyLevel = historyCmds.Size;

    if (input == "help") {
        MSG_(">> Help :");
        MSG_("use PgUp/PgDown for crawling commands history.");
        MSG_("getCallsList, switchDebug, killCall [callId, -all], getAccountInfo, getContactsList, placeCall [contact name]");
        return;
    }
    else if (input == "getCallsList") {
        RingD::instance->getCallsList();
        return;
    }
    else if (input == "switchDebug") {
        MSG_(">> switching dameon debug output");
        RingD::instance->switchDebug();
        return;
    }
    else if (input == "killCall") {
        if (parameter1.empty()) {
            MSG_("callId missing");
            return;
        }
        RingD::instance->killCall(Utils::toPlatformString(parameter1));
        return;
    }
    else if (input == "getAccountInfo") {
        auto id = AccountListItemsViewModel::instance->_selectedItem->_account->accountID_;
        MSG_("id : "+Utils::toString(id));
        return;
    }

    MSG_(">> error, command \'" + input + "\' not found");
}

void RingConsolePanel::addCommandToHistory()
{
    historyCmds.Append(_tBoxDbg_->Text);
}

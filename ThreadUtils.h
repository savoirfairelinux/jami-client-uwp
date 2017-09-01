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

#include <functional>
#include <mutex>
#include <queue>

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::System;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::System::Threading;

namespace RingClientUWP
{
namespace Utils
{
namespace Threading
{

void
runOnWorkerThread(std::function<void()> const& f,
    WorkItemPriority priority = WorkItemPriority::Normal)
{
    ThreadPool::RunAsync(ref new WorkItemHandler([=](IAsyncAction^ spAction)
    {
        f();
    }, Platform::CallbackContext::Any), priority);
}

void
runOnWorkerThreadDelayed(int delayInMilliSeconds, std::function<void()> const& f,
    WorkItemPriority priority = WorkItemPriority::Normal)
{
    // duration is measured in 100-nanosecond units
    TimeSpan delay;
    delay.Duration = 10000 * delayInMilliSeconds;
    ThreadPoolTimer^ delayTimer = ThreadPoolTimer::CreateTimer(
        ref new TimerElapsedHandler([=](ThreadPoolTimer^ source)
    {
        f();
    }), delay);
}

void
runOnUIThread(std::function<void()> const& f,
    CoreDispatcherPriority priority = CoreDispatcherPriority::High)
{
    CoreApplication::MainView->CoreWindow->Dispatcher->RunAsync(priority,
        ref new DispatchedHandler([=]()
    {
        f();
    }));
}

void
runOnUIThreadDelayed(int delayInMilliSeconds, std::function<void()> const& f)
{
    // duration is measured in 100-nanosecond units
    TimeSpan delay;
    delay.Duration = 10000 * delayInMilliSeconds;
    ThreadPoolTimer^ delayTimer = ThreadPoolTimer::CreateTimer(
        ref new TimerElapsedHandler([=](ThreadPoolTimer^ source)
    {
        runOnUIThread(f);
    }), delay);
}

class task_queue {
public:
    task_queue() {}

    task_queue(task_queue const& other) {}

    void add_task(std::function<void()> const& task) {
        runOnWorkerThread([this, task]() {
            std::lock_guard<std::mutex> lk(taskMutex_);
            tasks_.push(task);
        });
    }

    void dequeue_tasks() {
        runOnWorkerThread([this]() {
            std::lock_guard<std::mutex> lk(taskMutex_);
            while (!tasks_.empty()) {
                auto f = tasks_.front();
                f();
                tasks_.pop();
            }
        });
    }

    void clear() {
        runOnWorkerThread([this]() {
            std::lock_guard<std::mutex> lk(taskMutex_);
            std::queue<std::function<void()>> empty;
            std::swap(tasks_, empty);
        });
    }

private:
    std::mutex taskMutex_;
    mutable std::queue<std::function<void()>> tasks_;
};

} /*namespace Tasks*/

} /*namespace Utils*/

} /*namespace RingClientUWP*/

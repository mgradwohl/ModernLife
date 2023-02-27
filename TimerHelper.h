#pragma once

#include <winrt/Windows.Foundation.h>
#include <winrt/Microsoft.UI.Dispatching.h>

#include "Log.h"

// https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.dispatching.dispatcherqueuetimer.tick?view=windows-app-sdk-1.2
class TimerHelper
{
public:
    // construct
    TimerHelper() = default;

    TimerHelper(int fps, bool repeating)
    {
        //std::scoped_lock lock { _locktimer };
        //_controller = winrt::Microsoft::UI::Dispatching::DispatcherQueueController::CreateOnCurrentThread();
        _queue = Microsoft::UI::Dispatching::DispatcherQueue::GetForCurrentThread();  //_controller.DispatcherQueue();
        _timer = _queue.CreateTimer();
        _timer.Stop();
        _timer.IsRepeating(repeating);
        FPS(fps);
    }

    TimerHelper(TimerHelper&) = delete;
    TimerHelper(TimerHelper&&) = delete;

    // copy/move
    //TimerHelper& operator=(TimerHelper&) = delete;
    //TimerHelper& operator=(TimerHelper&&) = delete;

    // destruct
    ~TimerHelper()
    {
        ML_METHOD;

        Revoke();
    }

    const void Revoke()
    {
		ML_METHOD;
        std::scoped_lock lock { _locktimer };
        if (!_needsRevoke)
        {
            return;
        }
        _timer.Stop();
		_timer.Tick(_eventtoken);
        _needsRevoke = false;
	}

    void Tick(winrt::Windows::Foundation::TypedEventHandler<winrt::Microsoft::UI::Dispatching::DispatcherQueueTimer, winrt::Windows::Foundation::IInspectable> const& handler)
    {
        std::scoped_lock lock { _locktimer };
        _eventtoken = _timer.Tick(handler);
    }

    void Stop()
    {
        std::scoped_lock lock { _locktimer };
        _timer.Stop();
    }

    void Start()
    {
        std::scoped_lock lock { _locktimer };
        _timer.Start();
    }

    [[nodiscard]] bool IsRunning()
    {
        std::scoped_lock lock { _locktimer };
        return _timer.IsRunning();
    }

    void Repeating(bool repeating)
    {
        std::scoped_lock lock { _locktimer };
        _timer.IsRepeating(repeating);
    }

    [[nodiscard]] bool Repeating()
    {
        std::scoped_lock lock { _locktimer };
        return _timer.IsRepeating();
    }

    [[nodiscard]] int FPS() const noexcept
    {
		return _fps;
    }
    
    [[nodiscard]] void FPS(int fps)
    {
        std::scoped_lock lock { _locktimer };
        using namespace  std::literals::chrono_literals;
        if (fps > 0 && fps <= 240)
        {
			_fps = fps;
            _timer.Interval(std::chrono::milliseconds(1000 / _fps));
        }
    }

private:
    std::mutex _locktimer;

    winrt::Microsoft::UI::Dispatching::DispatcherQueueController _controller{ nullptr };
    winrt::Microsoft::UI::Dispatching::DispatcherQueue _queue{ nullptr };
    winrt::Microsoft::UI::Dispatching::DispatcherQueueTimer _timer{ nullptr };
    winrt::event_token _eventtoken{ 0 };
	int _fps{ 30 };
    bool _needsRevoke = true;
};
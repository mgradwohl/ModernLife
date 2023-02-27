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
        if (!_needsRevoke)
        {
            return;
        }
        Stop();
		_timer.Tick(_eventtoken);
        _needsRevoke = false;
	}

    TimerHelper(int fps, bool repeating)
    {
        if (_eventtoken)
        {
            _timer.Tick(_eventtoken);
        }
        
        _controller = winrt::Microsoft::UI::Dispatching::DispatcherQueueController::CreateOnDedicatedThread();
        _queue = _controller.DispatcherQueue();
        _timer = _queue.CreateTimer();
        _timer.Stop();
        Repeating(repeating);
        FPS(fps);
    }
    
    void Tick(winrt::Windows::Foundation::TypedEventHandler<winrt::Microsoft::UI::Dispatching::DispatcherQueueTimer, winrt::Windows::Foundation::IInspectable> const& handler)
    {
        _eventtoken = _timer.Tick(handler);
    }

    void Stop()
    {
        _timer.Stop();
    }

    void Start()
    {
        _timer.Start();
    }

    [[nodiscard]] bool IsRunning() const
    {
        return _timer.IsRunning();
    }

    void Repeating(bool repeating)
    {
        _timer.IsRepeating(repeating);
    }

    [[nodiscard]] bool Repeating() const
    {
        return _timer.IsRepeating();
    }

    [[nodiscard]] int FPS() const noexcept
    {
		return _fps;
    }
    
    [[nodiscard]] void FPS(int fps)
    {
        using namespace  std::literals::chrono_literals;
        if (fps > 0 && fps <= 240)
        {
			_fps = fps;
            _timer.Interval(std::chrono::milliseconds(1000 / _fps));
        }
    }

private:
    winrt::Microsoft::UI::Dispatching::DispatcherQueueController _controller{ nullptr };
    winrt::Microsoft::UI::Dispatching::DispatcherQueue _queue{ nullptr };
    winrt::Microsoft::UI::Dispatching::DispatcherQueueTimer _timer{ nullptr };
    winrt::event_token _eventtoken{ 0 };
	int _fps{ 30 };
    bool _needsRevoke = true;
};
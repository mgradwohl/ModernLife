#pragma once
#include <winrt/Windows.Foundation.h>
#include <winrt/Microsoft.UI.Dispatching.h>
#include <winrt/Windows.UI.Xaml.h>
#include <winrt/Windows.Foundation.Collections.h>

class TimerHelper
{
public:
    explicit TimerHelper(std::nullptr_t) {};

    // https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.dispatching.dispatcherqueuetimer.tick?view=windows-app-sdk-1.2

    TimerHelper(int fps, bool repeating)
    {
        if (!_initialized)
        {
            _controller = winrt::Microsoft::UI::Dispatching::DispatcherQueueController::CreateOnDedicatedThread();
            _queue = _controller.DispatcherQueue();
            _timer = _queue.CreateTimer();

            using namespace  std::literals::chrono_literals;
            _timer.Interval(std::chrono::milliseconds(1000 / fps));
            _timer.IsRepeating(repeating);
            _initialized = true;
        }
    }
    
    void Tick(winrt::Windows::Foundation::TypedEventHandler<winrt::Microsoft::UI::Dispatching::DispatcherQueueTimer, winrt::Windows::Foundation::IInspectable> const& handler)
    {
        _eventtoken = _timer.Tick(handler);
    }

    // does not work
    TimerHelper(winrt::Windows::Foundation::TypedEventHandler<winrt::Microsoft::UI::Dispatching::DispatcherQueueTimer, winrt::Windows::Foundation::IInspectable> const& handler, int fps, bool repeating)
    {
        if (!_initialized)
        {
            _controller = winrt::Microsoft::UI::Dispatching::DispatcherQueueController::CreateOnDedicatedThread();
            _queue = _controller.DispatcherQueue();
            _timer = _queue.CreateTimer();
            _eventtoken = _timer.Tick(handler);

            using namespace  std::literals::chrono_literals;
            _timer.Interval(std::chrono::milliseconds(1000/fps));
            _timer.IsRepeating(repeating);
            _initialized = true;
        }
    }

    ~TimerHelper()
    {
        // release anything that needs to be released
        _timer.Stop();
        _initialized = false;
        _timer.Tick(_eventtoken);
    }

	winrt::Microsoft::UI::Dispatching::DispatcherQueueTimer& GetTimer()
	{
		return _timer;
	}
    
    void Stop()
    {
        _timer.Stop();
    }


    void Start()
    {
        _timer.Start();
    }

    bool IsRunning()
    {
        return _timer.IsRunning();
    }

    void Repeating(bool repeating)
    {
        _timer.IsRepeating(repeating);
    }

    bool Repeating()
    {
        return _timer.IsRepeating();
    }

    void FPS(int fps)
    {
        using namespace  std::literals::chrono_literals;
        if (fps > 0)
        {
            _timer.Interval(std::chrono::milliseconds(1000 / fps));
        }
    }

private:
    winrt::Microsoft::UI::Dispatching::DispatcherQueueController _controller{ nullptr };
    winrt::Microsoft::UI::Dispatching::DispatcherQueue _queue{ nullptr };
    winrt::Microsoft::UI::Dispatching::DispatcherQueueTimer _timer{ nullptr };
    winrt::event_token _eventtoken = 0;
    bool _initialized{ false };
};
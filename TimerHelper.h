#pragma once

#include <winrt/Microsoft.UI.Dispatching.h>

class TimerHelper
{
public:
    TimerHelper() = delete;
    explicit TimerHelper(std::nullptr_t) noexcept {};

    // https://learn.microsoft.com/en-us/windows/windows-app-sdk/api/winrt/microsoft.ui.dispatching.dispatcherqueuetimer.tick?view=windows-app-sdk-1.2

    TimerHelper(int fps, bool repeating)
    {
        if (_eventtoken)
        {
            _timer.Tick(_eventtoken);
        }
        
        _controller = winrt::Microsoft::UI::Dispatching::DispatcherQueueController::CreateOnDedicatedThread();
        _queue = _controller.DispatcherQueue();
        _timer = _queue.CreateTimer();
        Repeating(repeating);
        FPS(fps);
    }
    
    // does not work
    //TimerHelper(winrt::Windows::Foundation::TypedEventHandler<winrt::Microsoft::UI::Dispatching::DispatcherQueueTimer, winrt::Windows::Foundation::IInspectable> const& handler, int fps, bool repeating)
    //{
    //    if (!_initialized)
    //    {
    //        _controller = winrt::Microsoft::UI::Dispatching::DispatcherQueueController::CreateOnDedicatedThread();
    //        _queue = _controller.DispatcherQueue();
    //        _timer = _queue.CreateTimer();
    //        _eventtoken = _timer.Tick(handler);

    //        using namespace  std::literals::chrono_literals;
    //        _timer.Interval(std::chrono::milliseconds(1000/fps));
    //        _timer.IsRepeating(repeating);
    //        _initialized = true;
    //    }
    //}

    ~TimerHelper()
    {
        // release anything that needs to be released
        //_timer.Stop();
        _timer.Tick(_eventtoken);
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

    int FPS() noexcept
    {
		return _fps;
    }
    
    void FPS(int fps)
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
};
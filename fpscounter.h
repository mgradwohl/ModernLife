#pragma once

#include <chrono>

class FPScounter
{
public:
	// construct
	FPScounter() = default;
	FPScounter(FPScounter&& b) = delete;
	FPScounter(FPScounter& b) = delete;

	// copy/move
	FPScounter& operator=(FPScounter&& b) = delete;
	FPScounter& operator=(FPScounter& b) = delete;

	// destruct
	~FPScounter() = default;

	inline void Start() noexcept
	{
		_start = std::chrono::high_resolution_clock::now();
		_end = std::chrono::high_resolution_clock::now();
	}

	inline double FPS() const noexcept
	{
		return _fps;
	}

	void AddFrame() noexcept
	{
		_frames++;
		
		if (_frames - _framebaseline >= 5)
		{
			_end = std::chrono::high_resolution_clock::now();
			const auto start = std::chrono::time_point_cast<std::chrono::milliseconds>(_start).time_since_epoch().count();
			const auto end = std::chrono::time_point_cast<std::chrono::milliseconds>(_end).time_since_epoch().count();
			const auto span = end - start;
			const double dspan = span * 0.001;

			if (span != 0)
			{
				_fps = (_frames - _framebaseline) / dspan;
			}
			_framebaseline = _frames;
			_start = std::chrono::high_resolution_clock::now();
		}
	}
	
private:
	std::chrono::time_point<std::chrono::high_resolution_clock> _start;
	std::chrono::time_point<std::chrono::high_resolution_clock> _end;
	uint32_t _frames{ 0 };
	uint32_t _framebaseline{ 0 };
	double _fps{ 0 };
};
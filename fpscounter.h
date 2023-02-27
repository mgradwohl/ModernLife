#pragma once

#include <chrono>

class FPScounter
{
public:
	// construct
	FPScounter() = default;

	// copy/move constructor
	FPScounter(FPScounter&& b) = delete;
	FPScounter(FPScounter& b) = delete;

	// copy/move not needed
	FPScounter& operator=(FPScounter&& b) = delete;
	FPScounter& operator=(FPScounter& b) = delete;

	// destruct
	~FPScounter() = default;

	void Start() noexcept
	{
		_start = std::chrono::high_resolution_clock::now();
		_end = std::chrono::high_resolution_clock::now();
	}

	[[nodiscard]] double FPS() const noexcept
	{
		return _fps;
	}

	void AddFrame() noexcept
	{
		_frames++;

		if (_frames - _framebaseline >= 5)
		{
			_end = std::chrono::high_resolution_clock::now();
			const auto span = std::chrono::duration_cast<std::chrono::milliseconds>(_end - _start).count();

			if (span != 0)
			{
				_fps = (_frames - _framebaseline) / (span * 0.001);
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
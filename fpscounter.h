#pragma once
#include <chrono>

class FPScounter
{
public:
	FPScounter(FPScounter& b) = delete;

	~FPScounter() = default;
	
	FPScounter()
	{
		Start();
	}
	
	void Start()
	{
		_start = std::chrono::high_resolution_clock::now();
		_end = std::chrono::high_resolution_clock::now();

		_frames = 0;
	}

	uint64_t FPS()
	{
		auto start = std::chrono::time_point_cast<std::chrono::seconds>(_start).time_since_epoch().count();
		auto end = std::chrono::time_point_cast<std::chrono::seconds>(_end).time_since_epoch().count();

		auto span = end - start;
		
		if (span != 0)
		{
			return _frames / span;
		}
		else
			return 0;
	}

	void AddFrame()
	{
		_end = std::chrono::high_resolution_clock::now();
		_frames++;
	}
	
private:
	std::chrono::time_point<std::chrono::high_resolution_clock> _start;
	std::chrono::time_point<std::chrono::high_resolution_clock> _end;
	uint32_t _frames = 0;
};

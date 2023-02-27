#pragma once

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "Cell.h"

class Shape
{
public:
	Shape(std::filesystem::path& path)
	{
		_path = path;
	}

	Shape(std::string& path)
	{
		_path = std::filesystem::path(path);
	}

	~Shape() = default;

	bool Load();
	void Dump();

	[[nodiscard]] uint16_t Width() noexcept
	{
		return _width;
	}

	[[nodiscard]] uint16_t Height() noexcept
	{
		return _height;
	}

	[[nodiscard]] uint16_t MaxDimension() noexcept
	{
		return _maxdim;
	}

	[[nodiscard]] std::string Name() noexcept
	{
		return _name;
	}

	[[nodiscard]] std::vector<std::string>& GetNotes() noexcept
	{
		return _notes;
	}

	[[nodiscard]] std::vector<Cell>& GetCells() noexcept
	{
		return _cells;
	}

	[[nodiscard]] bool IsAlive(uint16_t x, uint16_t y)
	{
		return gsl::at(_cells, x + y * _width).IsAlive();
	}

private:
	void Parse();

	uint16_t _width{ 0 };
	uint16_t _height{ 0 };
	uint16_t _maxdim{ 0 };

	std::filesystem::path _path;
	std::ifstream _stream;

	std::string _name{ 0 };
	std::vector<std::string> _notes;
	std::vector<std::string> _textcells;

	std::vector<Cell> _cells;
};
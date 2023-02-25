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

	[[nodiscard]] size_t Width()
	{
		return _width;
	}

	[[nodiscard]] size_t Height()
	{
		return _height;
	}

	[[nodiscard]] size_t MaxDimension()
	{
		return _maxdim;
	}

	[[nodiscard]] std::string Name()
	{
		return _name;
	}

	[[nodiscard]] std::vector<std::string>& GetNotes()
	{
		return _notes;
	}

	[[nodiscard]] std::vector<Cell>& GetCells()
	{
		return _cells;
	}

	[[nodiscard]] bool IsAlive(size_t x, size_t y)
	{
		return _cells[x + y * _width].IsAlive();
	}

private:
	void Parse();

	size_t _width{ 0 };
	size_t _height{ 0 };
	size_t _maxdim{ 0 };

	std::filesystem::path _path;
	std::ifstream _stream;

	std::string _name{ 0 };
	std::vector<std::string> _notes;
	std::vector<std::string> _textcells;

	std::vector<Cell> _cells;
};
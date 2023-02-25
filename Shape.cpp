#include "pch.h"
#include "Shape.h"

#include <fstream>
#include <string>

#include "Cell.h"
#include"Log.h"

bool Shape::Load()
{
	ML_METHOD;

	_stream = std::ifstream(_path, std::ifstream::in);

    if (!_stream.is_open())
    {
		ML_TRACE("Failed to open file: {}", _path.string());
		return false;
	}

	std::string line{0};
	while (std::getline(_stream, line))
	{
		if (line.empty())
			continue;

		if (line[0] == '!')
		{
			_notes.push_back(line);
			continue;
		}

		_textcells.push_back(line);
	}

	_name = _notes[0];
	_notes.erase(_notes.begin());

	Dump();

	Parse();

	return true;
}

void Shape::Dump()
{
	ML_TRACE(_name);
	for (const auto& note : _notes)
	{
		ML_TRACE(note);
	}

	for (const auto& cell : _textcells)
	{
		ML_TRACE(cell);
	}
}

void Shape::Parse()
{
	ML_METHOD;

	_height = _textcells.size();
	_width = 0;
	for (const auto& row : _textcells)
	{
		if (row.size() > _width)
			_width = row.size();
	}

	if (_width > _height)
	{
		_maxdim = _width;
	}
	else
	{
		_maxdim = _height;
	}

	_cells.resize(_width * _height);

	int x = 0;
	int y = 0;
	for (const auto& row : _textcells)
	{
		for (const auto& cell : row)
		{
			if (cell == 'O')
			{
				_cells[y * _width + x].SetState(Cell::State::Live);
			}
			x++;
		}
		x = 0;
		y++;
	}
}

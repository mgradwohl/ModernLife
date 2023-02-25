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
	_stream.close();

	_name = _notes[0];
	_notes.erase(_notes.begin());

	Dump();

	Parse();

	return true;
}

void Shape::Dump()
{
	#ifdef ML_LOGGING
	ML_TRACE(_name);
	for (const auto& note : _notes)
	{
		ML_TRACE(note);
	}

	for (const auto& cell : _textcells)
	{
		ML_TRACE(cell);
	}
	#endif
}

void Shape::Parse()
{
	ML_METHOD;

	// get the width and height of the shape
	// since the file will not always contain full rows
	_height = gsl::narrow_cast<uint16_t>(_textcells.size());
	_width = 0;
	for (const auto& row : _textcells)
	{
		if (row.size() > _width)
			_width = gsl::narrow_cast<uint16_t>(row.size());
	}

	// set the max dimension (width or height)
	if (_width > _height)
	{
		_maxdim = _width;
	}
	else
	{
		_maxdim = _height;
	}

	// create a vector of Cells that is the right size, initialize to all dead Cells
	_cells.resize(_width * _height);

	int x = 0;
	int y = 0;
	for (const auto& row : _textcells)
	{
		for (const auto& cell : row)
		{
			if (cell == 'O')
			{
				// only turn on Cells if they are alive in the file
				// otherwise do nothing (leave them dead)
				gsl::at(_cells, y * _width + x).SetState(Cell::State::Live);
			}
			x++;
		}
		x = 0;
		y++;
	}
}

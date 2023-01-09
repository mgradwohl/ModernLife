#include "pch.h"
#include "Board.h"
#include <future>

// for visualization purposes (0,0) is the top left.
// as x increases move right, as y increases move down

// optimized to never use std::endl until the full board is done printing
std::wostream& operator<<(std::wostream& stream, Board& board)
{
	static std::wstring str(((board.Width() + 2) * board.Height()) + 1, ' ');
	// clear the static string of any leftover goo
	str.clear();

	for (uint16_t y = 0; y < board.Height(); y++)
	{
		for (uint16_t x = 0; x < board.Width(); x++)
		{
			const Cell& cell = board.GetCell(x, y);
			str += cell.GetEmojiStateString();
		}
		str += L"\r\n";
	}

	wprintf(str.c_str());
	return stream;
}

Board::Board(uint16_t width, uint16_t height)
	: _width(width), _height(height), _size(width * height)
{
	_board.resize(_size);
}

void Board::PrintBoard()
{
	std::wcout << (*this) << std::endl;
}

void Board::SetCell(Cell& cell, Cell::State state) noexcept
{
	cell.SetState(state);

	switch (state)
	{
		case Cell::State::Dead:
		{
			_numDead++;
			break;
		}
		case Cell::State::Live:
		{
			_numLive++;
			break;
		}
		case Cell::State::Born:
		{
			_numBorn++;
			cell.SetAge(0);
			break;
		}
		case Cell::State::Old:
		{
			_numOld++;
			break;
		}
		case Cell::State::Dying:
		{
			_numDying++;
			break;
		}
		default:
			// do nothing
			break;
	}
}

uint8_t Board::CountLiveAndDyingNeighbors(uint16_t x, uint16_t y) noexcept
{
	// calculate offsets that wrap
	const uint16_t xoleft = (x == 0) ? _width - 1 : -1;
	const uint16_t xoright = (x == (_width - 1)) ? -(_width - 1) : 1;
	const uint16_t yoabove = (y == 0) ? _height - 1 : -1;
	const uint16_t yobelow = (y == (_height - 1)) ? -(_height - 1) : 1;

	uint8_t count = 0;

	if (GetCell(x + xoleft, y + yobelow).IsAlive()) count++;
	if (GetCell(x, y + yobelow).IsAlive()) count++;
	if (GetCell(x + xoright, y + yobelow).IsAlive()) count++;

	if (GetCell(x + xoleft, y + yoabove).IsAlive()) count++;
	if (GetCell(x, y + yoabove).IsAlive()) count++;
	if (GetCell(x + xoright, y + yoabove).IsAlive()) count++;

	if (GetCell(x + xoleft, y).IsAlive()) count++;
	if (GetCell(x + xoright, y).IsAlive()) count++;

	GetCell(x,y).SetNeighbors(count);

	return count;
}

uint8_t Board::CountLiveNotDyingNeighbors(uint16_t x, uint16_t y) noexcept
{
	// calculate offsets that wrap
	const uint16_t xoleft = (x == 0) ? _width - 1 : -1;
	const uint16_t xoright = (x == (_width - 1)) ? -(_width - 1) : 1;
	const uint16_t yoabove = (y == 0) ? _height - 1 : -1;
	const uint16_t yobelow = (y == (_height - 1)) ? -(_height - 1) : 1;

	uint8_t count = 0;

	if (GetCell(x + xoleft, y + yobelow).IsAliveNotDying()) count++;
	if (GetCell(x, y + yobelow).IsAliveNotDying()) count++;
	if (GetCell(x + xoright, y + yobelow).IsAliveNotDying()) count++;

	if (GetCell(x + xoleft, y + yoabove).IsAliveNotDying()) count++;
	if (GetCell(x, y + yoabove).IsAliveNotDying()) count++;
	if (GetCell(x + xoright, y + yoabove).IsAliveNotDying()) count++;

	if (GetCell(x + xoleft, y).IsAliveNotDying()) count++;
	if (GetCell(x + xoright, y).IsAliveNotDying()) count++;

	GetCell(x,y).SetNeighbors(count);
	return count;
}

void Board::ApplyNextStateToBoard() noexcept
{
	_generation++;
	ResetCounts();
	// TODO check size of board before iterating over board
	for (auto& cell : _board)
	{
		if (cell.GetState() == Cell::State::Born)
		{
			SetCell(cell, Cell::State::Live);
			cell.SetAge(0);
			_dirty++;
			continue;
		}

		if (cell.GetState() == Cell::State::Dying)
		{
			SetCell(cell, Cell::State::Dead);
			_dirty++;
			continue;
		}

		SetCell(cell, cell.GetState());
		cell.SetAge(cell.Age() + 1);
	}
}

void Board::RandomizeBoard(float alivepct)
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<> pdis(0.0, 1.0);
	std::uniform_int_distribution<unsigned short> adis(0, 1000);

	// TODO check size of board before iterating over board
	for (auto& cell : _board)
	{
		static int ra;
		static double rp;
		ra = adis(gen);
		rp = pdis(gen);

		if (rp <= alivepct)
		{
			SetCell(cell, Cell::State::Live);
			cell.SetAge(ra);
		}
	}
	_dirty = 1; // must be dirty, we just randomized it
}

void Board::ConwayUpdateRowsWithNextState(uint16_t startRow, uint16_t endRow) noexcept
{
	for (uint16_t y = startRow; y < endRow; y++)
	{
		for (uint16_t x = 0; x < Width(); x++)
		{
			Cell& cell = GetCell(x, y);
			CountLiveAndDyingNeighbors(x, y);
			ConwayRules(cell);
		}
	}

}
void Board::ConwayUpdateBoardWithNextState()
{
	auto update1 = std::async(&Board::ConwayUpdateRowsWithNextState, this, static_cast<uint16_t>(0), static_cast<uint16_t>(Height()));
	update1.wait();
}

void Board::ConwayRules(Cell& cell) const noexcept
{
	// Any live cell with two or three live neighbours survives.
	// Any dead cell with three live neighbours becomes a live cell.
	// All other live cells die in the next generation. Similarly, all other dead cells stay dead.

	static uint16_t count = 0;
	count = cell.Neighbors();

	if (cell.IsAlive() && count >= 2 && count <= 3)
	{
		cell.SetState(Cell::State::Live);
	}
	else if (cell.IsDead() && count == 3)
	{
		cell.SetState(Cell::State::Born);
	}
	else if (cell.IsAlive())
	{
		cell.SetState(Cell::State::Dying);
	}
}

void Board::DayAndNightRules(Cell& cell) const noexcept
{
	// https://en.wikipedia.org/wiki/Day_and_Night_(cellular_automaton)
	// rule notation B3678/S34678, meaning that a dead cell becomes live (is born)
	// if it has 3, 6, 7, or 8 live neighbors, and a live cell remains alive (survives)
	// if it has 3, 4, 6, 7, or 8 live neighbors,

	static int count = 0;
	count = cell.Neighbors();

	if (cell.IsAlive() && ((count >= 3) && (count != 5)))
	{
		cell.SetState(Cell::State::Live);
	}
	else if (cell.IsDead() && (count == 3 || count >= 6))
	{
		cell.SetState(Cell::State::Born);
	}
	else if (cell.IsAlive())
	{
		cell.SetState(Cell::State::Dying);
	}
}

void Board::LifeWithoutDeathRules(Cell& cell) const noexcept
{
	// https://en.wikipedia.org/wiki/Life_without_Death
	// every cell that was alive in the previous pattern remains alive,
	// every dead cell that has exactly 3 live neighbors becomes alive itself
	// and every other dead cell remains dead. B3/S012345678

	static uint16_t count = 0;
	count = cell.Neighbors();

	if (cell.IsAlive())
	{
		cell.SetState(Cell::State::Live);
	}
	else
	if (cell.IsDead() && count == 3)
	{
		cell.SetState(Cell::State::Born);
	}
}

void Board::HighlifeRules(Cell& cell) const noexcept
{
	// https://en.wikipedia.org/wiki/Highlife_(cellular_automaton)
	// the rule B36 / S23; that is, a cell is born if it has 3 or 6 neighbors
	//and survives if it has 2 or 3 neighbors.

	static uint16_t count = 0;
	count = cell.Neighbors();

	if (cell.IsAlive() && count >= 2 && count <= 3)
	{
		cell.SetState(Cell::State::Live);
	}
	else
	if (cell.IsDead() && ((count == 3) || (count == 6)))
	{
		cell.SetState(Cell::State::Born);
	}
	else
	if (cell.IsAlive())
	{
		cell.SetState(Cell::State::Dying);
	}
}

void Board::SeedsRules(Cell& cell) const noexcept
{
	// https://en.wikipedia.org/wiki/Seeds_(cellular_automaton)
	// In each time step, a cell turns on or is "born" if it was off or "dead"
	// but had exactly two neighbors that were on
	// all other cells turn off. It is described by the rule B2 / S

	static uint16_t count = 0;
	count = cell.Neighbors();

	if (cell.IsDead() && count == 2)
	{
		cell.SetState(Cell::State::Born);
	}
	else
	{
		cell.SetState(Cell::State::Dying);
	}
}

void Board::BriansBrainRules(Cell& cell) const noexcept
{
	// https://en.wikipedia.org/wiki/Brian%27s_Brain
	// In each time step, a cell turns on if it was off but had exactly two neighbors that were on,
	// just like the birth rule for Seeds. All cells that were "on" go into the "dying" state,
	// which is not counted as an "on" cell in the neighbor count, and prevents any cell from
	// being born there. Cells that were in the dying state go into the off state.

	static uint16_t count = 0;
	count = cell.Neighbors();

	if (cell.IsDead() && count == 2)
	{
		cell.SetState(Cell::State::Born);
	}
	else
	if (cell.GetState() == Cell::State::Live)
	{
		cell.SetState(Cell::State::Dying);
	}
	else
	if (cell.GetState() == Cell::State::Dying)
	{
		cell.SetState(Cell::State::Dead);
	}
}

#include "pch.h"
#include "Board.h"

#include <future>
#include <algorithm>
#include <random>
#include <iostream>

#include "gsl/include/gsl"

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

void Board::Resize(uint16_t width, uint16_t height, uint16_t maxage)
{
	std::scoped_lock lock { _lockboard };
	_height = height;
	_width = width;
	_maxage = maxage;
	_board.resize(gsl::narrow_cast<size_t>(_height * _width));

	SetThreadCount();
}

void Board::SetThreadCount()
{
	_threadcount = gsl::narrow_cast<int>(std::thread::hardware_concurrency() / 2);
	_threadcount = std::clamp(_threadcount, 1, 8);
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

void Board::FastCountLiveAndDyingNeighbors(uint16_t x, uint16_t y)
{
	static const std::vector<int16_t> dx { -1,  0,  1, -1, 1, -1, 0, 1 };
	static const std::vector<int16_t> dy { -1, -1, -1,  0, 0,  1, 1, 1 };

	uint8_t count{ 0 };

	for (size_t i = 0; i < dx.size(); ++i)
	{
		const uint16_t xx = (x + gsl::at(dx,i) + _width) % _width;
		const uint16_t yy = (y + gsl::at(dy,i) + _height) % _height;
		if (GetCell(xx, yy).IsAlive())
		{
			count++;
		}
	}

	GetCell(x, y).SetNeighbors(count);
}

void Board::CountLiveAndDyingNeighbors(uint16_t x, uint16_t y)
{
	// calculate offsets that wrap
	const uint16_t xoleft = (x == 0) ? _width - 1 : -1;
	const uint16_t xoright = (x == (_width - 1)) ? -(_width - 1) : 1;
	const uint16_t yoabove = (y == 0) ? _height - 1 : -1;
	const uint16_t yobelow = (y == (_height - 1)) ? -(_height - 1) : 1;

	uint8_t count{ 0 };

	if (GetCell(x + xoleft, y + yobelow).IsAlive()) count++;
	if (GetCell(x, y + yobelow).IsAlive()) count++;
	if (GetCell(x + xoright, y + yobelow).IsAlive()) count++;

	if (GetCell(x + xoleft, y + yoabove).IsAlive()) count++;
	if (GetCell(x, y + yoabove).IsAlive()) count++;
	if (GetCell(x + xoright, y + yoabove).IsAlive()) count++;

	if (GetCell(x + xoleft, y).IsAlive()) count++;
	if (GetCell(x + xoright, y).IsAlive()) count++;

	GetCell(x,y).SetNeighbors(count);
}

void Board::Update(int32_t ruleset)
{
	std::scoped_lock lock { _lockboard };
	FastUpdateBoardWithNextState(ruleset);
	ApplyNextStateToBoard();
}

void Board::ApplyNextStateToBoard() noexcept
{
	_generation++;
	ResetCounts();
	for (Cell& cell : _board)
	{
		if (cell.GetState() == Cell::State::Born)
		{
			SetCell(cell, Cell::State::Live);
			cell.SetAge(0);
			continue;
		}

		if (cell.GetState() == Cell::State::Dying)
		{
			SetCell(cell, Cell::State::Dead);
			continue;
		}

		SetCell(cell, cell.GetState());
		cell.SetAge(cell.Age() + 1);
	}
}

void Board::RandomizeBoard(float alivepct, uint16_t maxage)
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution pdis(0.0, 1.0);
	std::uniform_int_distribution<int> adis(0, maxage);

	_maxage = maxage;
	{
		std::scoped_lock lock { _lockboard };
		for (Cell& cell : _board)
		{
			static int ra;
			static double rp;
			ra = adis(gen);
			rp = pdis(gen);

			if (rp <= alivepct)
			{
				SetCell(cell, Cell::State::Live);
				cell.SetAge(gsl::narrow_cast<uint16_t>(ra));
			}
			else
			{
				SetCell(cell, Cell::State::Dead);
			}
		}
	}
}

void Board::UpdateRowsWithNextState(uint16_t startRow, uint16_t endRow, int32_t ruleset)
{
	using RuleMethod = void (Board::*)(Cell&) const noexcept;
	RuleMethod f_rules = &Board::FastConwayRules;

	switch (ruleset)
	{
		case 1:	f_rules = &Board::FastConwayRules; break;
		case 2:	f_rules = &Board::DayAndNightRules; break;
		case 3:	f_rules = &Board::LifeWithoutDeathRules; break;
		case 4:	f_rules = &Board::BriansBrainRules; break;
		case 5:	f_rules = &Board::SeedsRules; break;
		case 6:	f_rules = &Board::HighlifeRules; break;
		default: f_rules = &Board::ConwayRules; break;
	}

	for (uint16_t y = startRow; y < endRow; y++)
	{
		for (uint16_t x = 0; x < Width(); x++)
		{
			Cell& cell = GetCell(x, y);
			CountLiveAndDyingNeighbors(x, y);
			std::invoke(f_rules, this, cell);
		}
	}
}

void Board::FastUpdateBoardWithNextState(int32_t ruleset)
{
	uint16_t rowStart = 0;
	const uint16_t rowsPerThread = gsl::narrow_cast<uint16_t>(Height() / _threadcount);
	const uint16_t remainingRows = gsl::narrow_cast<uint16_t>(Height() % _threadcount);

	{
		// create a scope block so the vector dtor will be called and auto join the threads
		std::vector<std::jthread> threads;
		for (int t = 0; t < _threadcount - 1; t++)
		{
			threads.push_back(std::jthread{ &Board::UpdateRowsWithNextState,this, rowStart, gsl::narrow_cast<uint16_t>(rowStart + rowsPerThread), ruleset });
			rowStart += rowsPerThread;

		}
		threads.push_back(std::jthread{ &Board::UpdateRowsWithNextState,this, rowStart, gsl::narrow_cast<uint16_t>(rowStart + rowsPerThread + remainingRows), ruleset });
	}
}

void Board::ConwayRules(Cell& cell) const noexcept
{
	// Any live cell with two or three live neighbours survives.
	// Any dead cell with three live neighbours becomes a live cell.
	// All other live cells die in the next generation. Similarly, all other dead cells stay dead.

	const uint16_t count = cell.Neighbors();

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

inline void Board::FastConwayRules(Cell& cell) const noexcept
{
	const uint16_t count = cell.Neighbors();

	cell.SetState(
		cell.IsAlive() && count >= 2 && count <= 3 ? Cell::State::Live
		: cell.IsDead() && count == 3 ? Cell::State::Born
		: cell.IsAlive() ? Cell::State::Dying : Cell::State::Dead
	);
}

void Board::DayAndNightRules(Cell& cell) const noexcept
{
	// https://en.wikipedia.org/wiki/Day_and_Night_(cellular_automaton)
	// rule notation B3678/S34678, meaning that
	// a dead cell becomes live (is born) if it has 3, 6, 7, or 8 live neighbors
	// live cell remains alive (survives) if it has 3, 4, 6, 7, or 8 live neighbors,

	const uint16_t count = cell.Neighbors();

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

	if (cell.IsDead() && cell.Neighbors() == 3)
	{
		cell.SetState(Cell::State::Born);
	}
	if (cell.IsDying())
	{
		// should never happen
		cell.SetState(Cell::State::Live);
	}
}

void Board::HighlifeRules(Cell& cell) const noexcept
{
	// https://en.wikipedia.org/wiki/Highlife_(cellular_automaton)
	// the rule B36 / S23; that is,
	// a cell is born if it has 3 or 6 neighbors
	// and survives if it has 2 or 3 neighbors.

	const uint16_t count = cell.Neighbors();

	if (cell.IsAlive() && ((count == 2) || (count == 3)))
	{
		cell.SetState(Cell::State::Live);
	}
	else
	if (cell.IsDead() && ((count == 3) || (count == 6)))
	{
		cell.SetState(Cell::State::Born);
	}
	else
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

	if (cell.IsDead() && cell.Neighbors() == 2)
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

	// Cell::State::BrianDying is a special case for this ruleset
	// so that Dying cells draw as well as Live cells
	if (cell.GetState() == Cell::State::BrianDying)
	{
		cell.SetState(Cell::State::Dying);
	}
	else
	if (cell.GetState() == Cell::State::Live)
	{
		cell.SetAge(_maxage +1);
		cell.SetState(Cell::State::BrianDying);
	}
	else
	if (cell.IsDead() && cell.Neighbors() == 2)
	{
		cell.SetState(Cell::State::Born);
	}
}

#include "pch.h"
#include "cell.h"

void Cell::SetState(State state)
{
	_state = state;

	switch (_state)
	{
		case Cell::State::Dead:
		{
			numDead++;
			_age = 0;
			break;
		}
		case Cell::State::Live: numLive++;
			break;
		case Cell::State::Born:
		{
			numBorn++;
			_age = 0;
			break;
		}
		case Cell::State::Old: numOld++;
			break;
		case Cell::State::Dying: numDying++;
			break;
		default:
			// do nothing
			break;
	}
}

bool Cell::IsAlive() const
{
	if (_state == Cell::State::Live || _state == Cell::State::Dying || _state == Cell::State::Old)
	{
		return true;
	}
	return false;
}

bool Cell::IsAliveNotDying() const
{
	if (_state == Cell::State::Live)
	{
		return true;
	}
	return false;
}

bool Cell::IsDead() const
{
	if (_state == Cell::State::Dead || _state == Cell::State::Born)
	{
		return true;
	}
	return false;
}

const char* Cell::GetStateString() const
{
	switch (_state)
	{
		case State::Dead: return " ";
			break;
		case State::Born: return "o";
			break;
		case State::Live: return "O";
			break;
		case State::Old: return "x";
			break;
		case State::Dying: return ".";
			break;
		default:
			return "?";
	}
}

const std::u8string& Cell::GetEmojiStateString() const
{
	static std::u8string sDead(u8"🖤");
	static std::u8string sLive(u8"😀");
	static std::u8string sBorn(u8"💕");
	static std::u8string sOld(u8"🤡");
	static std::u8string sDying(u8"🤢");
	static std::u8string sUnknown(u8"⁉️");

	switch (_state)
	{
		case State::Dead: return sDead;
			break;
		case State::Live: return sLive;
			break;
		case State::Born: return sBorn;
			break;
		case State::Old: return sOld;
			break;
		case State::Dying: return sDying;
			break;
		default:
			return sUnknown;
	}
}

void Cell::NextGeneration()
{
	if (_state == Cell::State::Dead)
	{
		// no birthday for you
		return;
	}

	if (_state == Cell::State::Born)
	{
		SetState(Cell::State::Live);
		_age = 0;
		return;
	}

	if (_state == Cell::State::Dying)
	{
		SetState(Cell::State::Dead);
		_age++;
		return;
	}
	_age++;
}

void Cell::KillOldCell()
{
	// we only enforce this rule if age > 0
	if (Cell::OldAge > 0)
	{
		if (_age == Cell::OldAge - 2)
		{
			// mark it as old, about to die
			SetState(Cell::State::Old);
		}
		else if (_age == Cell::OldAge - 1)
		{
			// mark it as old, about to die
			SetState(Cell::State::Dying);
		}
		else if (_age >= Cell::OldAge)
		{
			SetState(Cell::State::Dead);
		}
	}
}

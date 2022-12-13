#include "pch.h"
#include "cell.h"

void Cell::SetState(State state)
{
	_state = state;
	if (state == Cell::State::Born)
	{
		_age = 0;
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

const std::string& Cell::GetEmojiStateString() const
{
	static std::string sDead("🖤");
	static std::string sLive("😀");
	static std::string sBorn("💕");
	static std::string sOld("🤡");
	static std::string sDying("🤢");
	static std::string sUnknown("⁉️");

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

//void Cell::KillOldCell()
//{
//	// we only enforce this rule if age > 0
//	if (Cell::OldAge > 0)
//	{
//		if (_age == Cell::OldAge - 2)
//		{
//			// mark it as old, about to die
//			SetState(Cell::State::Old);
//		}
//		else if (_age == Cell::OldAge - 1)
//		{
//			// mark it as old, about to die
//			SetState(Cell::State::Dying);
//		}
//		else if (_age >= Cell::OldAge)
//		{
//			SetState(Cell::State::Dead);
//		}
//	}
//}

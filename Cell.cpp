#include "pch.h"

#include "Cell.h"

#include <string>

const char* Cell::GetStateString() const noexcept
{
	switch (_state)
	{
		case CellState::Dead: return " ";
			break;
		case CellState::Born: return "o";
			break;
		case CellState::Live: return "O";
			break;
		case CellState::Old: return "x";
			break;
		case CellState::Dying: return ".";
			break;
		default:
			return "?";
	}
}

const std::wstring& Cell::GetEmojiStateString() const
{
	static std::wstring sDead(L"🖤");
	static std::wstring sLive(L"😀");
	static std::wstring sBorn(L"💕");
	static std::wstring sOld(L"🤡");
	static std::wstring sDying(L"🤢");
	static std::wstring sUnknown(L"⁉️");

	switch (_state)
	{
		case CellState::Dead: return sDead;
			break;
		case CellState::Live: return sLive;
			break;
		case CellState::Born: return sBorn;
			break;
		case CellState::Old: return sOld;
			break;
		case CellState::Dying: return sDying;
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

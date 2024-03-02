#include "pch.h"

#include "Cell.h"

#include <string>

const char* Cell::GetStateString() const noexcept
{
	switch (_state)
	{
		case Cell::State::Dead: return " ";
			break;
		case Cell::State::Born: return "o";
			break;
		case Cell::State::Live: return "O";
			break;
		case Cell::State::Old: return "x";
			break;
		case Cell::State::Dying: return ".";
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
		case Cell::State::Dead: return sDead;
			break;
		case Cell::State::Live: return sLive;
			break;
		case Cell::State::Born: return sBorn;
			break;
		case Cell::State::Old: return sOld;
			break;
		case Cell::State::Dying: return sDying;
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

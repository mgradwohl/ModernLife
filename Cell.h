#pragma once

#include <string>
//enum class CellState   { Dead, Born, Live, Old, Dying, BrianDying } ;
//enum class CellState : unsigned char { Dead, Born, Live, Old, Dying, BrianDying };

class Cell
{
public:
    enum class State : uint8_t { Dead, Born, Live, Old, Dying, BrianDying };

private:
    State _state{ State::Dead };
    uint8_t _neighbors{ 0 };
    uint16_t _age{ 0 };

public:
    Cell() = default;
    ~Cell() = default;

    // move/copy constuct
    Cell(Cell&& b) = default;
    Cell(Cell& b) = default;

    // no need to assign one cell to another cell
    // TODO future, compare by age or Live/Dead
    Cell& operator=(Cell&& b) = delete;
    Cell& operator=(Cell& b) = delete;

    [[nodiscard]] uint8_t Neighbors() const noexcept
    {
        return _neighbors;
    }

    void Neighbors(uint8_t n) noexcept
    {
        _neighbors = n;
    }

    void Age(uint16_t age) noexcept
    {
        _age = age;
    }

    void GetOlder() noexcept
    {
        _age++;
    }

    [[nodiscard]] uint16_t Age() const noexcept
    {
        return _age;
    }

    [[nodiscard]] State GetState() const noexcept
    {
        return _state;
    }

    void SetState(State state) noexcept
    {
        // if the state didn't change, do nothing
        if (_state == state)
        {
			return;
		}

        _state = state;
        if (state == State::Born)
        {
            _age = 0;
        }
    }

    [[nodiscard]] bool ShouldDraw() const noexcept
    {
        if (_state == State::Live || _state == State::BrianDying)
        {
            return true;
        }
        return false;
    }
    
    [[nodiscard]] bool IsAlive() const noexcept
    {
        if (_state == State::Live || _state == State::Dying || _state == State::Old)
        {
            return true;
        }
        return false;
    }

    [[nodiscard]] bool IsAliveNotDying() const noexcept
    {
        if (_state == State::Live)
        {
            return true;
        }
        return false;
    }

    [[nodiscard]] bool IsDying() const noexcept
    {
        if (_state == State::Dying)
        {
            return true;
        }
        return false;
    }

    [[nodiscard]] bool IsDead() const noexcept
    {
        if (_state == State::Dead || _state == State::Born)
        {
            return true;
        }
        return false;
    }

    [[nodiscard]] const char* GetStateString() const noexcept;

    [[nodiscard]] const std::wstring& GetEmojiStateString() const;

    //void KillOldCell();
};

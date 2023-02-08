#pragma once

#include <string>

class Cell
{
public:
    enum class State { Dead, Born, Live, Old, Dying, BrianDying };

private:
    State _state{ State::Dead };
    uint16_t _age{ 0 };
    uint8_t _neighbors{ 0 };

public:
    Cell() noexcept : _state(State::Dead), _age(0), _neighbors(0)
    {
    }

    ~Cell() = default;

    inline uint8_t Neighbors() const noexcept
    {
        return _neighbors;
    }

    inline void SetNeighbors(uint8_t n) noexcept
    {
        _neighbors = n;
    }

    inline void SetAge(uint16_t age) noexcept
    {
        _age = age;
    }

    inline uint16_t Age() const noexcept
    {
        return _age;
    }


    inline State GetState() const noexcept
    {
        return _state;
    }

    inline void SetState(State state) noexcept
    {
        _state = state;
        if (state == Cell::State::Born)
        {
            _age = 0;
        }
    }

    inline bool ShouldDraw() const noexcept
    {
        if (_state == Cell::State::Live || _state == Cell::State::BrianDying)
        {
            return true;
        }
        return false;
    }
    
    inline bool IsAlive() const noexcept
    {
        if (_state == Cell::State::Live || _state == Cell::State::Dying || _state == Cell::State::Old)
        {
            return true;
        }
        return false;
    }

    inline bool IsAliveNotDying() const noexcept
    {
        if (_state == Cell::State::Live)
        {
            return true;
        }
        return false;
    }

    inline bool IsDying() const noexcept
    {
        if (_state == Cell::State::Dying)
        {
            return true;
        }
        return false;
    }

    inline bool IsBrianDying() const noexcept
    {
        if (_state == Cell::State::BrianDying)
        {
            return true;
        }
        return false;
    }

    inline bool IsDead() const noexcept
    {
        if (_state == Cell::State::Dead || _state == Cell::State::Born)
        {
            return true;
        }
        return false;
    }

    const char* GetStateString() const noexcept;

    const std::wstring& GetEmojiStateString() const;

    //void KillOldCell();
};

﻿#pragma once

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
    Cell() = default;
    ~Cell() = default;

    [[nodiscard]] inline uint8_t Neighbors() const noexcept
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

    [[nodiscard]] inline uint16_t Age() const noexcept
    {
        return _age;
    }


    [[nodiscard]] inline State GetState() const noexcept
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

    [[nodiscard]] inline bool ShouldDraw() const noexcept
    {
        if (_state == Cell::State::Live || _state == Cell::State::BrianDying)
        {
            return true;
        }
        return false;
    }
    
    [[nodiscard]] inline bool IsAlive() const noexcept
    {
        if (_state == Cell::State::Live || _state == Cell::State::Dying || _state == Cell::State::Old)
        {
            return true;
        }
        return false;
    }

    [[nodiscard]] inline bool IsAliveNotDying() const noexcept
    {
        if (_state == Cell::State::Live)
        {
            return true;
        }
        return false;
    }

    [[nodiscard]] inline bool IsDying() const noexcept
    {
        if (_state == Cell::State::Dying)
        {
            return true;
        }
        return false;
    }

    [[nodiscard]] inline bool IsDead() const noexcept
    {
        if (_state == Cell::State::Dead || _state == Cell::State::Born)
        {
            return true;
        }
        return false;
    }

    [[nodiscard]] const char* GetStateString() const noexcept;

    [[nodiscard]] const std::wstring& GetEmojiStateString() const;

    //void KillOldCell();
};

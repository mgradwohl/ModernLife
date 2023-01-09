#pragma once
#include <string>
class Cell
{
public:
    enum class State { Dead, Born, Live, Old, Dying };

private:
    State _state;
    uint16_t _age;
    uint8_t _neighbors;

public:
    Cell() noexcept : _state(State::Dead), _age(0), _neighbors(0)
    {
    }

    ~Cell() = default;

    uint8_t Neighbors() const noexcept
    {
        return _neighbors;
    }

    void SetNeighbors(uint8_t n) noexcept
    {
        _neighbors = n;
    }

    void SetAge(uint16_t age) noexcept
    {
        _age = age;
    }

    uint16_t Age() const noexcept
    {
        return _age;
    }

    void SetState(State state) noexcept;

    State GetState() const noexcept
    {
        return _state;
    }

    bool IsAlive() const noexcept;

    bool IsAliveNotDying() const noexcept;

    bool IsDead() const noexcept;

    const char* GetStateString() const noexcept;

    const std::wstring& GetEmojiStateString() const;

    //void KillOldCell();
};

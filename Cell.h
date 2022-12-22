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
    Cell() : _state(State::Dead), _age(0), _neighbors(0)
    {
    }

    ~Cell() = default;

    uint8_t Neighbors() const
    {
        return _neighbors;
    }

    void SetNeighbors(uint8_t n)
    {
        _neighbors = n;
    }

    void SetAge(uint16_t age)
    {
        _age = age;
    }

    uint16_t Age() const
    {
        return _age;
    }

    void SetState(State state);

    State GetState() const
    {
        return _state;
    }

    bool IsAlive() const;

    bool IsAliveNotDying() const;

    bool IsDead() const;

    const char* GetStateString() const;

    const std::string& GetEmojiStateString() const;

    //void KillOldCell();
};

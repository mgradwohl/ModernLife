#pragma once
class Cell
{
public:
    enum class State { Dead, Born, Live, Old, Dying };

private:
    State _state;
    uint32_t _age;
    uint8_t _neighbors;

public:
    Cell() : _state(State::Dead), _age(0), _neighbors(0)
    {
    }

    //Cell operator=(Cell& cell) = delete;

    ~Cell() = default;

    int Neighbors() const
    {
        return _neighbors;
    }

    void SetNeighbors(uint8_t n)
    {
        _neighbors = n;
    }

    void SetAge(int age)
    {
        _age = age;
    }

    int Age() const
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

    const std::u8string& GetEmojiStateString() const;

    //void KillOldCell();
};

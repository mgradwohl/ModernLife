#pragma once
#include "Cell.h"

// for visualization purposes (0,0) is the top left.
// as x increases move right, as y increases move down
class Board
{
private:
    // if I allocated this on the heap, I could get the size right with resize
    std::vector<Cell> _board;
    uint16_t _width = 0;
    uint16_t _height = 0;
    uint16_t _size = 0;
    uint16_t _generation = 0;

    uint16_t _numDead = 0;
    uint16_t _numLive = 0;
    uint16_t _numBorn = 0;
    uint16_t _numOld = 0;
    uint16_t _numDying = 0;
    uint16_t _OldAge = -1;
    uint16_t _dirty = 0;

public:
    explicit Board(std::nullptr_t) {};
    Board(Board& b) = delete;

    ~Board() = default;

    Board(uint16_t width, uint16_t height);

    void SetOldAge(uint16_t age)
    {
        _OldAge = age;
    }

    int GetOldAge() const
    {
        return _OldAge;
    }

    int GetSize() const
    {
        return _size;
    }

    int GetDeadCount() const
    {
        return _numDead;
    }

    int GetLiveCount() const
    {
        return _numLive;
    }

    int GetBornCount() const
    {
        return _numBorn;
    }

    int GetOldCount() const
    {
        return _numOld;
    }

    int GetDyingCount() const
    {
        return _numDying;
    }

    void ResetCounts()
    {
        _numDead = 0;
        _numLive = 0;
        _numBorn = 0;
        _numDying = 0;
        _numOld = 0;
        _dirty = 0;
    }

    bool IsDirty() const
    {
        return _dirty;
    }

    int Generation() const
    {
        return _generation;
    }

    int Width() const
    {
        return _width;
    }

    int Height() const
    {
        return _height;
    }

    void SetCell(Cell& cell, Cell::State state);

    const Cell& GetCell(uint16_t x, uint16_t y) const
    {
        if (x * y > _size)
        {
            exit(-1);
        }

        return _board[x + (y * _width)];
    }

    Cell& GetCell(uint16_t x, uint16_t y)
    {
        if (x * y > _size)
        {
            exit(-1);
        }
        return _board[x + (y * _width)];
    }

    uint16_t CountLiveAndDyingNeighbors(uint16_t x, uint16_t y);

    uint16_t CountLiveNotDyingNeighbors(uint16_t x, uint16_t y);

    void ApplyNextStateToBoard();

    void RandomizeBoard(float alivepct);

    // This form does not work: void UpdateBoard(std::function<void(Cell& cell)>& F)
    // but using auto is magic
    void UpdateBoardWithNextState(auto F)
    {
        for (uint16_t y = 0; y < Height(); y++)
        {
            for (uint16_t x = 0; x < Width(); x++)
            {
                Cell& cc = GetCell(x, y);
                CountLiveAndDyingNeighbors(x, y);
                F(cc);
                //cc.KillOldCell();
            }
        }
    }

    void ConwayUpdateBoardWithNextState();

    void ConwayRules(Cell& cell);

    void DayAndNightRules(Cell& cell) const;

    void LifeWithoutDeathRules(Cell& cell) const;

    void HighlifeRules(Cell& cell) const;

    void SeedsRules(Cell& cell) const;

    void BriansBrainRules(Cell& cell) const;
 
    void PrintBoard();
};
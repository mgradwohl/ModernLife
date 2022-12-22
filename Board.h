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
    uint32_t _size = 0;
    uint32_t _generation = 0;

    uint32_t _numDead = 0;
    uint32_t _numLive = 0;
    uint32_t _numBorn = 0;
    uint32_t _numOld = 0;
    uint32_t _numDying = 0;
    uint32_t _OldAge = static_cast<uint16_t>(- 1);
    uint32_t _dirty = 0;

public:
    explicit Board(std::nullptr_t) {};
    Board(Board& b) = delete;

    ~Board() = default;

    Board(uint16_t width, uint16_t height);

    void SetOldAge(uint32_t age)
    {
        _OldAge = age;
    }

    uint32_t GetOldAge() const
    {
        return _OldAge;
    }

    uint32_t GetSize() const
    {
        return _size;
    }

    uint32_t GetDeadCount() const
    {
        return _numDead;
    }

    uint32_t GetLiveCount() const
    {
        return _numLive;
    }

    uint32_t GetBornCount() const
    {
        return _numBorn;
    }

    uint32_t GetOldCount() const
    {
        return _numOld;
    }

    uint32_t GetDyingCount() const
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

    uint32_t Generation() const
    {
        return _generation;
    }

    uint32_t Width() const
    {
        return _width;
    }

    uint32_t Height() const
    {
        return _height;
    }

    void SetCell(Cell& cell, Cell::State state);

    const Cell& GetCell(uint16_t x, uint16_t y) const
    {
        uint32_t check = x * y;
        if (check > _size)
        {
            exit(-1);
        }

        return _board[x + (y * _width)];
    }

    Cell& GetCell(uint16_t x, uint16_t y)
    {
        uint32_t check = x * y;
        if (check > _size)
        {
            exit(-1);
        }
        return _board[x + (y * _width)];
    }

    uint8_t CountLiveAndDyingNeighbors(uint16_t x, uint16_t y);

    uint8_t CountLiveNotDyingNeighbors(uint16_t x, uint16_t y);

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

    void ConwayUpdateRowsWithNextState(uint16_t startRow, uint16_t endRow);
    void ConwayUpdateBoardWithNextState();

    void ConwayRules(Cell& cell);

    void DayAndNightRules(Cell& cell) const;

    void LifeWithoutDeathRules(Cell& cell) const;

    void HighlifeRules(Cell& cell) const;

    void SeedsRules(Cell& cell) const;

    void BriansBrainRules(Cell& cell) const;
 
    void PrintBoard();
};
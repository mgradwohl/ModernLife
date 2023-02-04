#pragma once
#include "Cell.h"
#include <vector>
#include <stdexcept>

// for visualization purposes (0,0) is the top left.
// as x increases move right, as y increases move down
class Board
{
private:
    std::vector<Cell> _board;
    uint16_t _width{ 0 };
    uint16_t _height{ 0 };
    uint32_t _size{ 0 };
    uint32_t _generation{ 0 };

    uint32_t _numDead{ 0 };
    uint32_t _numLive{ 0 };
    uint32_t _numBorn{ 0 };
    uint32_t _numOld{ 0 };
    uint32_t _numDying{ 0 };
    uint32_t _OldAge{ 0xFFFFFFFF };

    int _threadcount{ 1 };

public:
    explicit Board(std::nullptr_t) noexcept {};
    Board(Board& b) = delete;
    Board(uint16_t width, uint16_t height);
    ~Board() = default;

    void SetThreadCount();

    void SetOldAge(uint32_t age) noexcept
    {
        _OldAge = age;
    }

    uint32_t GetOldAge() const noexcept
    {
        return _OldAge;
    }

    uint32_t GetSize() const noexcept
    {
        return _size;
    }

    uint32_t GetDeadCount() const noexcept
    {
        return _numDead;
    }

    uint32_t GetLiveCount() const noexcept
    {
        return _numLive;
    }

    uint32_t GetBornCount() const noexcept
    {
        return _numBorn;
    }

    uint32_t GetOldCount() const noexcept
    {
        return _numOld;
    }

    uint32_t GetDyingCount() const noexcept
    {
        return _numDying;
    }

    void ResetCounts() noexcept
    {
        _numDead = 0;
        _numLive = 0;
        _numBorn = 0;
        _numDying = 0;
        _numOld = 0;
    }

    uint32_t Generation() const noexcept
    {
        return _generation;
    }

    uint32_t Width() const noexcept
    {
        return _width;
    }

    uint32_t Height() const noexcept
    {
        return _height;
    }

    void SetCell(Cell& cell, Cell::State state) noexcept;

    inline const Cell& GetCell(uint16_t x, uint16_t y) const
    {
        return gsl::at(_board, x + (y * _width));
    }

    inline Cell& GetCell(uint16_t x, uint16_t y)
    {
        return gsl::at(_board, x + (y * _width));
    }

    uint8_t FastCountLiveAndDyingNeighbors(uint16_t x, uint16_t y);
    
    uint8_t CountLiveAndDyingNeighbors(uint16_t x, uint16_t y);

    uint8_t CountLiveNotDyingNeighbors(uint16_t x, uint16_t y);

    void ApplyNextStateToBoard() noexcept;

    void RandomizeBoard(float alivepct, int maxage);

    // This form does not work: void UpdateBoard(std::function<void(Cell& cell)>& F)
    // but using auto is magic
    //void UpdateBoardWithNextState(auto F)
    //{
    //    for (uint16_t y = 0; y < Height(); y++)
    //    {
    //        for (uint16_t x = 0; x < Width(); x++)
    //        {
    //            Cell& cell = GetCell(x, y);
    //            CountLiveAndDyingNeighbors(x, y);
    //            F(cell);
    //            //cc.KillOldCell();
    //        }
    //    }
    //}

    void UpdateRowsWithNextState(uint16_t startRow, uint16_t endRow, int32_t ruleset);
	void FastUpdateBoardWithNextState(int32_t ruleset);

    void ConwayRules(Cell& cell) const noexcept;

    void FastConwayRules(Cell& cell) const noexcept;

    void DayAndNightRules(Cell& cell) const noexcept;

    void LifeWithoutDeathRules(Cell& cell) const noexcept;

    void HighlifeRules(Cell& cell) const noexcept;

    void SeedsRules(Cell& cell) const noexcept;

    void BriansBrainRules(Cell& cell) const noexcept;
 
    void PrintBoard();
};
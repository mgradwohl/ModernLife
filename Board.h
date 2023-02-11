﻿#pragma once

#include <vector>
#include <mutex>

#include "gsl/include/gsl"

#include "Cell.h"

// for visualization purposes (0,0) is the top left.
// as x increases move right, as y increases move down
class Board
{
public:
    Board() = default;
    ~Board() = default;

    void SetThreadCount();
    void Resize(uint16_t width, uint16_t height, uint16_t maxage);

    void SetCell(Cell& cell, Cell::State state) noexcept;

    inline const Cell& GetCell(uint16_t x, uint16_t y) const
    {
        return gsl::at(_board, gsl::narrow_cast<uint16_t>(x + (y * _width)));
    }

    inline Cell& GetCell(uint16_t x, uint16_t y)
    {
        return gsl::at(_board, gsl::narrow_cast<uint16_t>(x + (y * _width)));
    }

    void RandomizeBoard(float alivepct, uint16_t maxage);

    // board updating
	// Update calls UpdateRowsWithNextState and FastUpdateBoardWithNextState
	// if you drew the board in between those calls, you'd see the intermediate states e.g. cells born or that will die in the next generation
	// many of these are split up to support multithreading
    void Update(int32_t ruleset);
    void UpdateRowsWithNextState(uint16_t startRow, uint16_t endRow, int32_t ruleset);
	void FastUpdateBoardWithNextState(int32_t ruleset);
    uint8_t FastCountLiveAndDyingNeighbors(uint16_t x, uint16_t y);
    uint8_t CountLiveAndDyingNeighbors(uint16_t x, uint16_t y);
    uint8_t CountLiveNotDyingNeighbors(uint16_t x, uint16_t y);
    void ApplyNextStateToBoard() noexcept;

    // rulesets
    void ConwayRules(Cell& cell) const noexcept;
    void FastConwayRules(Cell& cell) const noexcept;
    void DayAndNightRules(Cell& cell) const noexcept;
    void LifeWithoutDeathRules(Cell& cell) const noexcept;
    void HighlifeRules(Cell& cell) const noexcept;
    void SeedsRules(Cell& cell) const noexcept;
    void BriansBrainRules(Cell& cell) const noexcept;
 
    void PrintBoard();

	// getters
    inline void MaxAge(uint16_t maxage) noexcept
    {
        _maxage = maxage;
    }

    const inline uint16_t MaxAge() const noexcept
    {
        return _maxage;
    }

    inline void OldAge(uint32_t age) noexcept
    {
        _OldAge = age;
    }

    const inline uint32_t GetOldAge() const noexcept
    {
        return _OldAge;
    }

    const inline uint32_t GetDeadCount() const noexcept
    {
        return _numDead;
    }

    const inline uint32_t GetLiveCount() const noexcept
    {
        return _numLive;
    }

    const inline uint32_t GetBornCount() const noexcept
    {
        return _numBorn;
    }

    const inline uint32_t GetOldCount() const noexcept
    {
        return _numOld;
    }

    const inline uint32_t GetDyingCount() const noexcept
    {
        return _numDying;
    }

    inline void ResetCounts() noexcept
    {
        _numDead = 0;
        _numLive = 0;
        _numBorn = 0;
        _numDying = 0;
        _numOld = 0;
    }

    const inline uint32_t Generation() const noexcept
    {
        return _generation;
    }

    const inline uint16_t Width() const noexcept
    {
        return _width;
    }

    const inline uint16_t Height() const noexcept
    {
        return _height;
    }

    const inline uint32_t GetSize() const noexcept
    {
        return _height * _width;
    }

private:
    std::vector<Cell> _board;
    std::mutex _lockboard;

    uint16_t _width{ 0 };
    uint16_t _height{ 0 };
    uint32_t _generation{ 0 };

    uint32_t _numDead{ 0 };
    uint32_t _numLive{ 0 };
    uint32_t _numBorn{ 0 };
    uint32_t _numOld{ 0 };
    uint32_t _numDying{ 0 };
    uint32_t _OldAge{ 0xFFFFFFFF };
    uint16_t _maxage{ 100 };

    int _threadcount{ 1 };
};
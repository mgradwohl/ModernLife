#pragma once

#include <mutex>
#include <vector>

#include <deps/gsl/include/gsl/gsl>

#include "Shape.h"
#include "Cell.h"

struct GridPoint
{
    uint16_t x;
    uint16_t y;
};

// for visualization purposes (0,0) is the top left.
// as x increases move right, as y increases move down
class Board
{
  public:

    Board() noexcept;
    ~Board() = default;

    // move/copy constuct
    Board(Board&& b) = delete;
    Board(Board& b) = delete;

    // no need to assign one board to another board
    Board& operator=(Board&& b) = delete;
    Board& operator=(Board& b) = delete;

    [[nodiscard]] const Cell& GetCell(uint16_t x, uint16_t y) const
    {
        return gsl::at(_cells, x + (y * _width));
    }

    [[nodiscard]] Cell& GetCell(uint16_t x, uint16_t y)
    {
        return gsl::at(_cells, x + (y * _width));
    }

    void Reserve(size_t max);
    void Resize(uint16_t width, uint16_t height, uint16_t maxage);
    void RandomizeBoard(float alivepct, uint16_t maxage);
    void TurnCellOn(GridPoint g, bool on);
    void Update(int32_t ruleset);
    bool CopyShape(Shape& shape, uint16_t startX, uint16_t startY);
    void PrintBoard();

    // getters
    const void MaxAge(uint16_t maxage) noexcept
    {
        _maxage = maxage;
    }

    [[nodiscard]] const uint16_t MaxAge() const noexcept
    {
        return _maxage;
    }

    const void OldAge(uint32_t age) noexcept
    {
        _OldAge = age;
    }

    [[nodiscard]] const uint32_t GetOldAge() const noexcept
    {
        return _OldAge;
    }

    [[nodiscard]] const uint32_t GetDeadCount() const noexcept
    {
        return _numDead;
    }

    [[nodiscard]] const uint32_t GetLiveCount() const noexcept
    {
        return _numLive;
    }

    [[nodiscard]] const uint32_t GetBornCount() const noexcept
    {
        return _numBorn;
    }

    [[nodiscard]] const uint32_t GetOldCount() const noexcept
    {
        return _numOld;
    }

    [[nodiscard]] const uint32_t GetDyingCount() const noexcept
    {
        return _numDying;
    }

    [[nodiscard]] const uint32_t Generation() const noexcept
    {
        return _generation;
    }

    [[nodiscard]] const uint16_t Width() const noexcept
    {
        return _width;
    }

    [[nodiscard]] const uint16_t Height() const noexcept
    {
        return _height;
    }

    [[nodiscard]] const uint32_t Size() const noexcept
    {
        return _height * _width;
    }

private:
    // board updating
    // Update calls UpdateRowsWithNextState and FastDetermineNextState
    // if you drew the board in between those calls, you'd see the intermediate states e.g. cells born or that will die
    // in the next generation many of these are split up to support multithreading
    void SetCell(Cell& cell, Cell::State state) noexcept;
    void UpdateRowsWithNextState(uint16_t startRow, uint16_t endRow, int32_t ruleset);
    void FastDetermineNextState(int32_t ruleset);
    void CountLiveAndDyingNeighbors(uint16_t x, uint16_t y);
    [[nodiscard]] const uint8_t CountLiveNotDyingNeighbors(uint16_t x, uint16_t y);
    void ApplyNextState() noexcept;

    // rulesets
    void ConwayRules(Cell& cell) const noexcept;
    void FastConwayRules(Cell& cell) const noexcept;
    void DayAndNightRules(Cell& cell) const noexcept;
    void LifeWithoutDeathRules(Cell& cell) const noexcept;
    void HighlifeRules(Cell& cell) const noexcept;
    void SeedsRules(Cell& cell) const noexcept;
    void BriansBrainRules(Cell& cell) const noexcept;

    void ResetCounts() noexcept
    {
        _numDead = 0;
        _numLive = 0;
        _numBorn = 0;
        _numDying = 0;
        _numOld = 0;
    }

  private:
	  int _threadcount{1};
	  std::mutex _lockboard;
	  std::vector<Cell> _cells;

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
};

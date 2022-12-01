#pragma once
#include "Cell.h"

// for visualization purposes (0,0) is the top left.
// as x increases move right, as y increases move down
class Board
{
private:
    // if I allocated this on the heap, I could get the size right with resize
    std::vector<Cell> _board;
    int _width = 0;
    int _height = 0;
    int _size = 0;
    int _generation = 0;

    int _numDead = 0;
    int _numLive = 0;
    int _numBorn = 0;
    int _numOld = 0;
    int _numDying = 0;
    int _OldAge = -1;

public:
    Board(std::nullptr_t) {};
    Board(const Board& b) = delete;
    Board(Board& b) = delete;
    //Board operator=(Board& b) = delete;
    ~Board() = default;

    Board(int width, int height);

    void SetOldAge(int age)
    {
        _OldAge = age;
    }

    int GetOldAge()
    {
        return _OldAge;
    }

    int GetDeadCount()
    {
        return _numDead;
    }

    int GetLiveCount()
    {
        return _numLive;
    }

    int GetBornCount()
    {
        return _numBorn;
    }

    int GetOldCount()
    {
        return _numOld;
    }

    int GetDyingCount()
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

    const Cell& GetCell(int x, int y) const
    {
        // no bounds checking
        return _board[x + (y * _width)];
    }

    Cell& GetCell(int x, int y)
    {
        // no bounds checking
        return _board[x + (y * _width)];
    }

    int CountLiveAndDyingNeighbors(int x, int y);

    int CountLiveNotDyingNeighbors(int x, int y);

    void ApplyNextStateToBoard();

    void RandomizeBoard(float alivepct);

    // This form does not work: void UpdateBoard(std::function<void(Cell& cell)>& F)
    // but using auto is magic
    void UpdateBoardWithNextState(auto F)
    {
        for (int y = 0; y < Height(); y++)
        {
            for (int x = 0; x < Width(); x++)
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
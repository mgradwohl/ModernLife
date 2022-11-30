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
    int _x = 0;
    int _y = 0;
    inline static int numDead = 0;
    inline static int numLive = 0;
    inline static int numBorn = 0;
    inline static int numOld = 0;
    inline static int numDying = 0;
    inline static int OldAge = -1;

public:
    Board(std::nullptr_t) {};
    Board(const Board& b) = delete;
    Board(Board& b) = delete;
    //Board operator=(Board& b) = delete;
    ~Board() = default;

    Board(int width, int height);
    static void SetOldAge(int age)
    {
        OldAge = age;
    }

    static int GetOldAge()
    {
        return OldAge;
    }

    static int GetDeadCount()
    {
        return numDead;
    }

    static int GetLiveCount()
    {
        return numLive;
    }

    static int GetBornCount()
    {
        return numBorn;
    }

    static int GetOldCount()
    {
        return numOld;
    }

    static int GetDyingCount()
    {
        return numDying;
    }

    static void ResetCounts()
    {
        numDead = 0;
        numLive = 0;
        numBorn = 0;
        numDying = 0;
        numOld = 0;
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

    void SetCell(int x, int y, Cell::State state)
    {
        // no bounds checking
        Cell& cell = GetCell(x, y);
        cell.SetState(state);

        switch (state)
        {
            case Cell::State::Dead:
            {
                numDead++;
                break;
            }
            case Cell::State::Live:
            {
                numLive++;
                break;
            }
            case Cell::State::Born:
            {
                numBorn++;
                break;
            }
            case Cell::State::Old:
            {
                numOld++;
                break;
            }
            case Cell::State::Dying:
            {
                numDying++;
                break;
            }
            default:
                // do nothing
                break;
        }
    }

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

    void NextGeneration();

    void RandomizeBoard(int n);

    // This form does not work: void UpdateBoard(std::function<void(Cell& cell)>& F)
    // but using auto is magic
    void UpdateBoard(auto F)
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
    void ConwayRules(Cell& cell) const;

    void DayAndNightRules(Cell& cell) const;

    void LifeWithoutDeathRules(Cell& cell) const;

    void HighlifeRules(Cell& cell) const;

    void SeedsRules(Cell& cell) const;

    void BriansBrainRules(Cell& cell) const;
    void PrintBoard();
};
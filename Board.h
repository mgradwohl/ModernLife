#pragma once
#include "Cell.h"

// for visualization purposes (0,0) is the top left.
// as x increases move right, as y increases move down
class Board
{
private:
    // if I allocated this on the heap, I could get the size right with resize
    std::vector<Cell> _board;
    int _width;
    int _height;
    int _size;
    int _generation;
    int _x;
    int _y;

public:
    Board(std::nullptr_t) {};
    Board(const Board& b) = delete;
    Board(Board& b) = delete;
    //Board operator=(Board& b) = delete;
    ~Board() = default;

    Board(int width, int height);

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

    Cell& CurrentCell()
    {
        return _board[_x + (_y * _width)];
    }

    int CountLiveAndDyingNeighbors(Cell& cell);

    int CountLiveNotDyingNeighbors(Cell& cell);

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
                CountLiveAndDyingNeighbors(cc);
                F(cc);
                cc.KillOldCell();
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


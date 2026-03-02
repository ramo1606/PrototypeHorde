#pragma once

#include "Knight.h"
#include "Shuffler.h"

class MazeGenerator
{
	const int dirCol[4] = { -1, 0, 1, 0 };
	const int dirRow[4] = { 0, 1, 0, -1 };

public:
	enum MazeCellFlag
	{
		FlagWall = 0,
		FlagEmpty = 1,
		FlagEntry = 2,
		FlagExit = 3
	};
private:
	enum EDirection
	{
		UP = 0,
		DOWN,
		RIGHT,
		LEFT
	};

	int _cols, _rows;
	int _entry[2], _exit[2];
	int** _maze;

	/* Function: DFS 
	*		Use Deep First Search method to visit the next cell
	*  Parameters:
	*		Rows: Count of rows of the maze
	*		Columns: Count of columns of the maze
	*/
	void DFS(int row, int col)
	{
		int directions[] = { UP, DOWN, RIGHT, LEFT };
		Shuffler::FisherYateShuffle(directions, 4);

		for (int dir = 0; dir < 4; ++dir)
		{
			int direction = directions[dir];
			int newRow = row + 2 * dirRow[direction];
			int newCol = col + 2 * dirCol[direction];

			//Check if the new cell is valid 
			if (newRow >= 0 && newRow < _rows &&
				newCol >= 0 && newCol < _cols &&
				_maze[newRow][newCol] == FlagWall)
			{

				//Cave the path by setting the cells connecting the two cells to be empty
				_maze[row + dirRow[direction]][col + dirCol[direction]] = FlagEmpty;
				_maze[newRow][newCol] = FlagEmpty;

				//Recursively find the next way point on the path to the exit point
				DFS(newRow, newCol);
			}
		}
	}

public:

	/* Constructor: MazeGenerator
	*  Parameters:
	*		Rows: Count of rows of the maze
	*		Columns: Count of columns of the maze
	*/
	MazeGenerator(int Rows, int Columns)
		: _rows(Rows)
		, _cols(Columns)
	{
		_entry[0] = 0;
		_entry[1] = 2;
		_exit[0] = _rows - 1;
		_exit[1] = _cols - 3;

		//Allocate memory for the maze
		_maze = new int* [_rows];
		for (int row = 0; row < _rows; ++row)
		{
			_maze[row] = new int[_cols];
		}

		GenerateMaze();
	}

	/* Destructor: ~MazeGenerator
	 *  Relases the memory allocated for the maze. 
	*/
	~MazeGenerator()
	{
		for (int row = 0; row < _rows; ++row)
		{
			delete[] _maze[row];
		}
		delete[] _maze;
	}
	/* Function: GenerateMaze()
	 *  Generate the maze with the defalt entry and exit points
	*/
	void GenerateMaze()
	{
		//Initialize the maze with wall blocks
		for (int row = 0; row < _rows; ++row)
		{
			for (int col = 0; col < _cols; ++col)
			{
				_maze[row][col] = FlagWall;
			}
		}

		//Set maze entry and exit flags
		_maze[_entry[1]][_entry[0]] = FlagEntry;
		_maze[_exit[1]][_exit[0]] = FlagExit;

		//Find the path that starts at the entry point and leads to the exit.
		DFS(_entry[0], _entry[1]);
	}

	/* Function: GenerateMaze()
	 *  Generate the maze with the given entry and exit points  
	 *  Parameters:
	 *		Entry: the entry point position (row, col)
	 *		Exit: The exit point position (row, col)
	 */
	void GenerateMaze(int Entry[2], int Exit[2])
	{
		_entry[0] = Entry[0];
		_entry[0] = Entry[1];
		_exit[0] = Exit[0];
		_exit[1] = Exit[1];
		GenerateMaze();
	}

	int** GetMaze()
	{
		return const_cast<int**>(_maze);
	}

	int GetRowCount()
	{
		return _rows;
	}

	int GetColumnCount()
	{
		return _cols;
	}
};
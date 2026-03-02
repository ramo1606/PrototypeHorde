#pragma once

#include "Knight.h"

#define GridsSize 64	   //8x8 grid cells

class Maze
{
public:
	static int MazeGrids[GridsSize];
	static int CellSize;
	static int Rows, Cols;

public:
	
	/* Function GenerateMaze
	*  Description: generates the maze by randomlly setting each grid cell's flag.
	*				0-is accessible   1-has wall
	*/
	static void GenerateMaze()
	{
		for (int i = 0; i < GridsSize; ++i)
		{
			if (i == 0 || i == 63)
			{
				MazeGrids[i] = 0;
			}
			else
			{
				MazeGrids[i] = (GetRandomValue(0, 4) == 4) ? 1 : 0;
			}
		}
	}

	/* Function GetMazeCellIndex
	*  Description: gets the grid cell index on the matrix array base on its (row, col) 
	*  Parameters:
	*		row: the row index
	*		col: the col index
	*  Returns: the cell's index on the matrix array
	*/
	static int GetMazeCellIndex(int row, int col)
	{
		return row * Cols + col;
	}

	/* Function GetCellPosition
	*  Description: gets the 3D position of a cell based on its (row, col)
	*  Parameters:
	*		row: the row index
	*		col: the col index
	*  Returns: The Vector3 type 3D position
	*/
	static Vector3 GetCellPosition(int row, int col)
	{
		return Vector3
		{
			(float)(col - Cols / 2) * CellSize + CellSize / 2,
			0.0f,
			(float)(row - Rows / 2) * CellSize + CellSize / 2,
		};
	}

	/* Function GetMazeCellRowCol
	*  Description: gets the row and col indices from the array index
	*  Parameters:
	*		cellIndex: the cell's index on the matrix array
	*		row: output the row index
	*		col: output the col index
	*  Returns: true-succeeded; false-failed
	*/
	static bool GetMazeCellRowCol(int cellIndex, int& row, int& col)
	{
		if (cellIndex < 0 || cellIndex >= GridsSize)
		{
			return false;
		}
		row = cellIndex / Cols;
		col = cellIndex % Cols;
		return true;
	}

	/* Function GetMazeCellRowCol
	*  Description: gets the grid cell position from a 3D position
	*  Parameters: 
	*		position: A 3D position
	*		row: output the row index
	*		col: output the col index
	*  Returns: true-succeeded; false-failed
	*/
	static bool GetMazeCellRowCol(Vector3& position, int& row, int& col)
	{
		row = ((int)position.z + CellSize * 3 + CellSize / 2) / CellSize;
		col = ((int)position.x + CellSize * 3 + CellSize / 2) / CellSize;
		return true;
	}

	/* Function IsValidNode
	*  Description: checks if a node's (row, col) is within the grids map 
	*  Parameters:
	*		row: row index of the node
	*		col: col index of the node
	*  Returns: true-is valid; false-is invalid
	*/
	static bool IsValidNode(const int row, const int col)
	{
		int cellIndex = GetMazeCellIndex(row, col);
		return (row >= 0 && row < Rows &&
			col >= 0 && col < Cols &&
			MazeGrids[cellIndex] == 0);
	}
};


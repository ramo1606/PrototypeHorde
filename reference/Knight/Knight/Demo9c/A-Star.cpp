#include "A-Star.h"

vector<Vector3> AStarPathFinder::FindPath(Vector3& Start, Vector3& Target)
{
	priority_queue<Node*, vector<Node*>, NodeComparer> OpenList;
		//A priority queue contraining all travelled nodes
	vector<vector<bool>> ClosedList(GridsSize, vector<bool>(GridsSize, false));
		//A matrix indicating whether a grid node has been visited 
	Node* startNode = new Node(Start);
	Node* targetNode = new Node(Target);
	Maze::GetMazeCellRowCol(Start, startNode->GridPos[0], startNode->GridPos[1]);
	Maze::GetMazeCellRowCol(Target, targetNode->GridPos[0], targetNode->GridPos[1]);
	startNode->GCost = 0;			//Set the start node's G value to be 0
	startNode->HCost = ManhattanDistance(startNode, targetNode);
									//Calculate the H value
	OpenList.push(startNode);		//Enqueue the start node

	int directions[4][2] = { {-1, 0}, {1, 0}, {0, -1}, {0, 1} };
									//These four vectors help lead to the left, 
									//right, down, and up adjacent neighbours 
	while (!OpenList.empty())
	{
		//Get the shortest found neighbour node
		Node* currentNode = OpenList.top();
		OpenList.pop();

		int row = currentNode->GridPos[0];
		int col = currentNode->GridPos[1];
		ClosedList[row][col] = true;	//Mark as a visited node

		//If it reaches the target node, return the found path
		if (currentNode->GridPos[0] == targetNode->GridPos[0] &&
			currentNode->GridPos[1] == targetNode->GridPos[1])
		{
			return ReconstructPath(currentNode);
		}

		//Visit the four neighboure nodes 
		for (const auto& dir : directions)
		{
			int row = currentNode->GridPos[0] + dir[0];
			int col = currentNode->GridPos[1] + dir[1];

			//If the neighbour node is valid and has not been visited
			if (Maze::IsValidNode(row, col) && !ClosedList[row][col])
			{
				float newGCost = currentNode->GCost + 1;
				Vector3 pos = Maze::GetCellPosition(row, col);
				Node* neighborNode = new Node(pos, currentNode);
				neighborNode->GridPos[0] = row;
				neighborNode->GridPos[1] = col;
				neighborNode->GCost = newGCost;
				neighborNode->HCost = ManhattanDistance(neighborNode, targetNode);
				
				//Add the new visited node to the priority queue
				OpenList.push(neighborNode);
			}
		}
	}
	return {};
}



#pragma once

#include "Random.h"

class Shuffler
{
private:
	static Random _random;

public:
	/*	Function: FisherYateShuffle
		Parameters:
			Items: array which contains the items to be shuffled
			ArraySize: size of the items array
			PickedItems: array which stores the picked items when completion
			PickedSize: size of the pickedItems array
		Returns: true-succeeded, false-failed
	*/
	template<typename T>
	static bool FisherYateShuffle(T Items[], int Size, T PickedItems[], int PickedSize)
	{
		if (Size <= 0 || PickedSize > Size)
		{
			return false;
		}

		//Shuffle the items
		for (int i = Size - 1; i >= 0; --i)
		{
			int j = _random.NextInt(0, i);
			T temp = Items[j];
			Items[j] = Items[i];
			Items[i] = temp;
		}

		//Fill up the output array with the selected number of items
		for (int k = 0; k < PickedSize; ++k)
		{
			PickedItems[k] = Items[k];
		}

		return true;
	}

};

// create the random instance for generating random numbers
Random Shuffler::_random = (unsigned long)std::time(nullptr);

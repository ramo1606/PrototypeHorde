#pragma once

#include "Random.h"

class Selector
{
private:
	static Random _random;

public:
	/*	Function: RandomSelect
		Parameters:
			Options: array which contains the optional items
			ArraySize: size of the Options array
		Returns the selected item.
	*/
	template<typename T>
	static T RandomSelect(T Options[], int ArraySize)
	{
		int selectedIndex = _random.NextInt(0, ArraySize);
		return Options[selectedIndex];
	}

	/*	Function: WeightedRandomSelect
		Parameters:
			Options: array which contains the optional items
			Weigths: array which contains weights for each item
			ArraySize: size of the Options array
		Returns the selected item.
	*/
	template<typename T>
	static T WeightedRandomSelect(T Options[], float Weights[], int ArraySize)
	{
		int i;
		float totalWeight = 0.0f;

		for (i = 0; i < ArraySize; ++i)
		{
			totalWeight += Weights[i];
		}

		float randomValue = _random.NextFloat() * totalWeight;

		for (i = 0; i < ArraySize - 1; ++i)
		{
			if (randomValue < Weights[i])
			{
				break;
			}
			randomValue -= Weights[i];
		}

		return Options[i];
	}
};

// create the random instance for generating random numbers
Random Selector::_random = (unsigned long)std::time(nullptr);

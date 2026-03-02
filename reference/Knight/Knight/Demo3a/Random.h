#pragma once
#include <ctime>

class Random
{
private:

	const unsigned long _a = 1664525;		//Multiplayer
	const unsigned long _c = 1013904223;	//Increment
	unsigned long long _m = 0xffffffff;
	unsigned long _seed;

public:

	Random(long Seed = -1) : _seed(Seed) 
	{ 
		if (Seed < 0)
		{
			_seed = (unsigned long)std::time(nullptr);
		}
		else
		{
			_seed = Seed;
		}
	}

	/*	Function: Next()
	Calculate the next random number within [0, m).
	Returns: The next random unsigned long number.
	*/
	unsigned long Next()
	{
		_seed = (unsigned long)((_a * _seed + _c) % _m);
		return _seed;
	}

	/*	Function: NextInt()
		Calculate the next random number within [Min, Max).
		Returns: The next random integer number.
	*/
	unsigned int NextInt(unsigned int Min, unsigned int Max)
	{
		return Min + Next() % (Max - Min);
	}

	/*	Function: NextFloat()
		Calculate the next random float-point number within [0.0f, 1.0f].
		Returns: The next random float-point number.
	*/
	float NextFloat()
	{
		return static_cast<float>(Next()) / _m;
	}

	/*	Function: NextFLoat()
		Calculate the next random float-point number within [Min, Max].
		Returns: The next random float-point number.
	*/
	float NextFloat(float Min, float Max)
	{
		return Min + NextFloat() * (Max - Min);
	}
};
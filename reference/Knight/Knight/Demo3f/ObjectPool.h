#pragma once

#include "Knight.h"

#define MIN_POOL_SIZE 3

//A base class which serves as a base for actual poolable objects
class PoolableObject
{
protected:
    bool _isActive;
    float _lifespan;
public:
    PoolableObject()
        : _isActive(false)
        , _lifespan(0.0f)
    {
    }

    virtual void Activate()
    {
        _isActive = true;
    }

    virtual void Deactivate()
    {
        _isActive = false;
    }

    void SetLifespan(float Seconds)
    {
        _lifespan = Seconds;
    }

    float GetLifespan()
    {
        return _lifespan;
    }

    float DecreaseLifeSpan(float Seconds)
    {
        _lifespan -= Seconds;
    }

    bool IsActive()
    {
        return _isActive;
    }

    virtual void Update(float ElapsedSeconds) = 0;
    virtual void Draw() = 0;
};


//A template class which manages and contains the pool of objects
template<class T>
class ObjectPool
{
public :
    static_assert(std::is_base_of<PoolableObject, T>::value, "T must be a subclass of PoolableObject");

private:
    T** _pool;
    int _poolSize;
    int _activeCount;

    /* Function: ExpandPool
    *  Exponentially increase the pool size
    */
    void ExpandPool()
    {
        int newPoolSize = _poolSize * 2;    //Double the pools size
        T** newPool = new T*[newPoolSize];  //Allocate memory to create a new pool
        for (int i = 0; i < newPoolSize; ++i)
        {
            if (i < _poolSize)
            {
                //move the object from the old pool to the new pool
                newPool[i] = _pool[i];      
            }
            else
            {
                //Create a new object for the expanded part of the new pool
                newPool[i] = new T();       
            }
        }

        _poolSize = newPoolSize;

        //Release the memory allocated for the old pool
        delete[] _pool;
        _pool = newPool;
    }

  public:

      ObjectPool()
        : _poolSize(MIN_POOL_SIZE)
        , _activeCount(0)
    {
        //Initialize the pool
        _pool = new T*[MIN_POOL_SIZE];
        for (int i = 0; i < _poolSize; ++i)
        {
            _pool[i] = new T();
        }
    }

    ~ObjectPool()
    {
        //Release the memory for all the pool objects
        for (int i = 0; i < _poolSize; ++i)
        {
            delete _pool[i];
        }
        //Release the memory allocated to the pool
        delete[] _pool;
    }

    T* GetObject()
    {
        //Find the first inactive object and return it
        for (int i = 0; i < _poolSize; ++i)
        {
            if (!_pool[i]->IsActive())
            {
                _pool[i]->Activate();
                ++_activeCount;
                return _pool[i];
            }
        }

        //If no inactive object is found, expand the pool
        ExpandPool();

        //Get the object from the expanded pool and return it
        return GetObject();
    }

    void ReturnObject(T* objToReturn)
    {
        objToReturn->Deactivate();
        --_activeCount;
    }

    void Update(float ElapsedSeconds)
    {
        //Update all the active objects in the pool
        for (int i = 0; i < _poolSize; ++i)
        {
            if (_pool[i]->IsActive())
            {
                _pool[i]->Update(ElapsedSeconds);

                //If the object's lifespan reaches 0, return it.  
                if (_pool[i]->GetLifespan() <= 0.0f)
                {
                    ReturnObject(_pool[i]);
                }
            }
        }
    }

    void Draw()
    {
        //Draw all the active objects
        for (int i = 0; i < _poolSize; ++i)
        {
            if (_pool[i]->IsActive())
            {
                _pool[i]->Draw();
            }
        }
    }
};
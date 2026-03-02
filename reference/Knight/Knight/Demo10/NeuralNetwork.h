#pragma once

#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>

using namespace std;

class Neuron
{
private:
    vector<float> _Weights;     //Input weights
    float _Error;               //Error
    float _Activation;          //Activation
    float _Output;              //Output
    float _Bias;                //Bias
public:
    Neuron(size_t InputSize);
    void Activate(vector<float> inputs); 
public:
    vector<float>& GetWeights() { return _Weights; }
    float GetActivation() { return _Activation; }
    float GetOutput() { return _Output; }
    float GetError() { return _Error; }
    void SetError(float Error) { _Error = Error; }
    float GetBias() { return _Bias; }
    void SetBias(float Bias) { _Bias = Bias; }
};

class Layer
{
private:
    vector<Neuron> _Neurons;    //Neurons on this layer
public:
    Layer(size_t NeuronSize, size_t InputSize);
    vector<Neuron>& GetNeurons() { return _Neurons; }
};

class NeuralNetwork 
{
public:
    static float Sigmoid(float x)
    {
        return (float)(1.0 / (1.0 + exp(-x)));
    }

    static float SigmoidDerivative(float x)
    {
        return (float)(x * (1.0 - x));
    }

    static float TanH(float x)
    {
        return tanh(x);
    }

    static float TanHDerivative(float x)
    {
        return 1.0f - x * x;
    }

    static bool EnableMinusActivation;
private:
    vector<Layer> _Layers;      //The hidden and outpout layers
    float _LearningRate = 0.5f;  //Range from 0 to 1. 
    vector<float> ForwardPropagate(vector<float>& Inputs);
    void BackPropagate(vector<float>& Targets);
    void UpdateWeights(vector<float>& Inputs);
public:
    NeuralNetwork(vector<size_t>& LayerSizes, 
        float LearningRate = 0.5f, 
        bool MinusActivation = false);
    
    void Train(vector<float>& Inputs, vector<float>& Targets);
    
    vector<float> Predict(vector<float>& Inputs) { return ForwardPropagate(Inputs); }
};


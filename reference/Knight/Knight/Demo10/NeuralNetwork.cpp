#include <cassert>
#include "NeuralNetwork.h"

bool NeuralNetwork::EnableMinusActivation = false;

Neuron::Neuron(size_t InputSize)
	: _Error (0.0f)
	, _Activation(0.0f)
	, _Output(0.0f)
{
	//Using random values to initialize the weights and bias.
	for (int i = 0; i < InputSize; ++i)	
	{
		_Weights.push_back((float)((double)rand()) / RAND_MAX);
	}
	_Bias = (float)((double)rand() / RAND_MAX);
}

void Neuron::Activate(vector<float> Inputs)
{
	//The number of inputs must match the weights' size
	assert(Inputs.size() == _Weights.size());
	//Calculate the activation value 
	_Activation = _Bias;
	for (int i = 0; i < _Weights.size(); ++i)
	{
		_Activation += _Weights[i] * Inputs[i];
	}
	//Apply the activation function
	if (NeuralNetwork::EnableMinusActivation)
	{
		_Output = NeuralNetwork::TanH(_Activation);
	}
	else
	{
		_Output = NeuralNetwork::Sigmoid(_Activation);
	}
}

Layer::Layer(size_t NeuronSize, size_t InputSize)
{
	for (int i = 0; i < NeuronSize; ++i)
	{
		_Neurons.push_back(Neuron(InputSize));
	}
}

NeuralNetwork::NeuralNetwork(vector<size_t>& LayerSizes, float LearningRate, bool MinusActivation)
	: _LearningRate(LearningRate)
{
	EnableMinusActivation = MinusActivation;
	//Add all the hidden and output layers
	for (size_t i = 1; i < LayerSizes.size(); ++i)	
	{
		_Layers.push_back(Layer(LayerSizes[i], LayerSizes[i - 1]));
	}
}

vector<float> NeuralNetwork::ForwardPropagate(vector<float>& Inputs)
{
	vector<float> outputs;
	vector<float> inputs = Inputs;
	for (size_t layer = 0; layer < _Layers.size(); ++layer)
	{
		outputs.clear();
		vector<Neuron>& neurons = _Layers[layer].GetNeurons();
		for (size_t i = 0; i < neurons.size(); ++i)
		{
			neurons[i].Activate(inputs);
			outputs.push_back(neurons[i].GetOutput());
		}
		inputs.clear();
		inputs = outputs;
	}
	return outputs;
}

void NeuralNetwork::BackPropagate(vector<float>& Targets)
{
	size_t i;
	float error, output;
	vector<Neuron>& outputs = _Layers.back().GetNeurons();
	assert(Targets.size() == outputs.size());
	//Calculate errors for the outputs
	for (i = 0; i < outputs.size(); ++i)
	{
		output = outputs[i].GetOutput();
		float derivatived = (NeuralNetwork::EnableMinusActivation) ? 
				NeuralNetwork::TanHDerivative(output) :
				NeuralNetwork::SigmoidDerivative(output);
		error = Targets[i] - output;
		outputs[i].SetError(error * derivatived);
	}
	
	//Calculate errors for hidden layers’ neurons
	for (int layer = (int)_Layers.size() - 2; layer >= 0; --layer)
	{
		vector<Neuron>& neurons = _Layers[layer].GetNeurons();
		for (size_t i = 0; i < neurons.size(); ++i)
		{
			error = 0.0f;
			vector<Neuron>& nextLayerNeurons = _Layers[layer + 1].GetNeurons();
			for (size_t j = 0; j < nextLayerNeurons.size(); ++j)
			{
				error += nextLayerNeurons[j].GetWeights()[i] * nextLayerNeurons[j].GetError();
			}
			output = neurons[i].GetOutput();
			float derivative = (NeuralNetwork::EnableMinusActivation) ?
				NeuralNetwork::TanHDerivative(output) :
				NeuralNetwork::SigmoidDerivative(output);
			neurons[i].SetError(error * derivative);
		}
	}
}

void NeuralNetwork::UpdateWeights(vector<float>& Inputs)
{
	vector<float> inputs;
	for (size_t layer = 0; layer < _Layers.size(); ++layer)
	{
		if (layer == 0)
		{
			inputs = Inputs;
		}
		else
		{
			inputs.clear();
			vector<Neuron>& prevLayerNeurons = _Layers[layer - 1].GetNeurons();
			for (size_t i = 0; i < prevLayerNeurons.size(); ++i)
			{
				inputs.push_back(prevLayerNeurons[i].GetOutput());
			}
		}

		vector<Neuron>& neurons = _Layers[layer].GetNeurons();
		for (size_t i = 0; i < neurons.size(); ++i)
		{
			vector<float>& weights = neurons[i].GetWeights();
			for (size_t j = 0; j < inputs.size(); ++j)
			{
				weights[j] += _LearningRate * neurons[i].GetError() * inputs[j];
			}
			neurons[i].SetBias(neurons[i].GetBias() +_LearningRate * neurons[i].GetError());
		}
	}
}

void NeuralNetwork::Train(vector<float>& Inputs, vector<float>& Targets)
{
	ForwardPropagate(Inputs);
	BackPropagate(Targets);
	UpdateWeights(Inputs);
}

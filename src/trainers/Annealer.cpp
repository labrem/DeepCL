// Copyright Hugh Perkins 2015 hughperkins at gmail
//
// This Source Code Form is subject to the terms of the Mozilla Public License, 
// v. 2.0. If a copy of the MPL was not distributed with this file, You can 
// obtain one at http://mozilla.org/MPL/2.0/.

#include <iostream>

#include "trainers/Annealer.h"
#include "trainers/Trainer.h"
#include "EasyCL.h"
#include "util/stringhelper.h"
#include "net/NeuralNet.h"
#include "layer/Layer.h"
#include "clmath/CopyBuffer.h"
#include "clmath/GpuAdd.h"
#include "clmath/MultiplyInPlace.h"
#include "loss/LossLayer.h"
#include "loss/IAcceptsLabels.h"

using namespace std;

#undef STATIC
#undef VIRTUAL
#define STATIC
#define VIRTUAL

STATIC Annealer *Annealer::instance( EasyCL *cl, float learningRate, float anneal ) {
    Annealer *annealer = new Annealer( cl );
    annealer->setLearningRate( learningRate );
    annealer->setAnneal( anneal );
    return annealer;
}
Annealer::Annealer( EasyCL *cl ) :
    Trainer( cl ) {
    anneal = 1.0f;
//    epoch = -1;
    copyBuffer = new CopyBuffer( cl );
    gpuAdd = new GpuAdd( cl );
    multiplyInPlace = new MultiplyInPlace( cl );
}
VIRTUAL Annealer::~Annealer() {
    delete copyBuffer;
    delete gpuAdd;
    delete multiplyInPlace;
}
VIRTUAL std::string Annealer::asString() {
    return "Annealer{ learningRate=" + toString( learningRate ) + ", anneal=" + 
        toString( anneal ) + " }";
}
VIRTUAL void Annealer::setAnneal( float anneal ) {
    this->anneal = anneal;
}
VIRTUAL void Annealer::updateWeights( float annealedLearningRate, CLWrapper *weightsWrapper, CLWrapper *gradWeightsWrapper ) {
    // hmmmm, so all we need to do is calculate:
    // annealedLearningRate = learningRate * pow( anneal, epoch )
    // weightsWrapper = weightsWrapper - annealedLearningRate * gradWeightsWrapper

    int numWeights = weightsWrapper->size();

    float *gradWeightsCopy = new float[ numWeights ];
    CLWrapper *gradWeightsCopyWrapper = cl->wrap( numWeights, gradWeightsCopy );
    gradWeightsCopyWrapper->createOnDevice();

    copyBuffer->copy( numWeights, gradWeightsWrapper, gradWeightsCopyWrapper );
    multiplyInPlace->multiply( numWeights, - annealedLearningRate, gradWeightsCopyWrapper );
    gpuAdd->add( numWeights, weightsWrapper, gradWeightsCopyWrapper );

    delete gradWeightsCopyWrapper;
    delete[] gradWeightsCopy;
}
VIRTUAL BatchResult Annealer::train( NeuralNet *net, TrainingContext *context,
        float const*input, float const*expectedOutput ) {
    // hmmmm, so all we need to do is calculate:
    // annealedLearningRate = learningRate * pow( anneal, epoch )
    // weightsWrapper = weightsWrapper - annealedLearningRate * gradWeightsWrapper
//    cout << " epoch=" << epoch << " learningrate=" << learningRate << " anneal=" << anneal << endl;
    float annealedLearningRate = learningRate * pow( anneal, context->epoch );
    if( context->batch == 0 ) {
        cout << "Annealer annealedLearningRate=" << annealedLearningRate << endl;
    }

    bindState( net );
    net->forward( input );
    int numLayers = net->getNumLayers();
    LossLayer *lossLayer = dynamic_cast< LossLayer * >( net->getLastLayer() );
    if( lossLayer == 0 ) {
        throw runtime_error( "last layer of net should be a LossLayer class" );
    }
    lossLayer->calcGradInput( expectedOutput );
    for( int layerIdx = numLayers - 2; layerIdx > 0; layerIdx-- ) {
        Layer *layer = net->getLayer( layerIdx );
        if( !layer->needsBackProp() ) {
            break;
        }
        layer->backward();
        if( layer->needsTrainerState() ) {
            updateWeights( annealedLearningRate, layer->getWeightsWrapper(), layer->getGradWeightsWrapper() );
            if( layer->biased() ) {
                updateWeights( annealedLearningRate, layer->getBiasWrapper(), layer->getGradBiasWrapper() );
            }
        }
    }
    return BatchResult(0,0);
}
VIRTUAL BatchResult Annealer::trainFromLabels( NeuralNet *net, TrainingContext *context,
        float const*input, int const*labels ) {
    float annealedLearningRate = learningRate * pow( anneal, context->epoch );
    if( context->batch == 0 ) {
        cout << "Annealer annealedLearningRate=" << annealedLearningRate << endl;
    }

    bindState( net );
    net->forward( input );
    int numLayers = net->getNumLayers();
    IAcceptsLabels *lossLayer = dynamic_cast< IAcceptsLabels * >( net->getLastLayer() );
    if( lossLayer == 0 ) {
        throw runtime_error( "last layer of net should be a LossLayer class" );
    }
    lossLayer->calcGradInputFromLabels( labels );
    for( int layerIdx = numLayers - 2; layerIdx > 0; layerIdx-- ) {
        Layer *layer = net->getLayer( layerIdx );
        if( !layer->needsBackProp() ) {
            break;
        }
        layer->backward();
        if( layer->needsTrainerState() ) {
            updateWeights( annealedLearningRate, layer->getWeightsWrapper(), layer->getGradWeightsWrapper() );
            if( layer->biased() ) {
                updateWeights( annealedLearningRate, layer->getBiasWrapper(), layer->getGradBiasWrapper() );
            }
        }
    }
    return BatchResult(0,0);
}
VIRTUAL void Annealer::bindState( NeuralNet *net ) {
    // since we have no state, all we will do is strip any existing state,
    // so that if another trainer trains the net, it wont come across
    // some stale state
    for( int layerIdx = 0; layerIdx < net->getNumLayers(); layerIdx++ ) {
        Layer *layer = net->getLayer( layerIdx );
        if( layer->needsTrainerState() ) {
            TrainerState *state = layer->getTrainerState();
            if( state != 0 ) {
                layer->setTrainerState( 0 );
            }
        }
    }
}


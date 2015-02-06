// Copyright Hugh Perkins 2014 hughperkins at gmail
//
// This Source Code Form is subject to the terms of the Mozilla Public License, 
// v. 2.0. If a copy of the MPL was not distributed with this file, You can 
// obtain one at http://mozilla.org/MPL/2.0/.

#include <iostream>

#include "NeuralNet.h"
#include "Layer.h"
#include "RandomPatches.h"
#include "MyRandom.h"
#include "PatchExtractor.h"

using namespace std;

#undef VIRTUAL
#define VIRTUAL 
#undef STATIC
#define STATIC

RandomPatches::RandomPatches( Layer *previousLayer, RandomPatchesMaker *maker ) :
        Layer( previousLayer, maker ),
        patchSize( maker->_patchSize ),
        numPlanes ( previousLayer->getOutputPlanes() ),
        inputBoardSize( previousLayer->getOutputBoardSize() ),
        outputBoardSize( maker->_patchSize ),
        results(0),
        batchSize(0),
        allocatedSize(0) {
    if( inputBoardSize == 0 ) {
//        maker->net->print();
        throw runtime_error("Error: Pooling layer " + toString( layerIndex ) + ": input board size is 0" );
    }
    if( outputBoardSize == 0 ) {
//        maker->net->print();
        throw runtime_error("Error: Pooling layer " + toString( layerIndex ) + ": output board size is 0" );
    }
    if( previousLayer->needsBackProp() ) {
        throw runtime_error("Error: RandomPatches layer does not provide backprop currently, so you cannot put it after a layer that needs backprop");
    }
}
VIRTUAL RandomPatches::~RandomPatches() {
    if( results != 0 ) {
        delete[] results;
    }
}
VIRTUAL void RandomPatches::setBatchSize( int batchSize ) {
    if( batchSize <= allocatedSize ) {
        this->batchSize = batchSize;
        return;
    }
    if( results != 0 ) {
        delete[] results;
    }
    this->batchSize = batchSize;
    this->allocatedSize = batchSize;
    results = new float[ getResultsSize() ];
}
VIRTUAL int RandomPatches::getResultsSize() {
    return batchSize * numPlanes * outputBoardSize * outputBoardSize;
}
VIRTUAL float *RandomPatches::getResults() {
    return results;
}
VIRTUAL bool RandomPatches::needsBackProp() {
    return false;
}
VIRTUAL int RandomPatches::getResultsSize() const {
    return batchSize * numPlanes * outputBoardSize * outputBoardSize;
}
VIRTUAL int RandomPatches::getOutputBoardSize() const {
    return outputBoardSize;
}
VIRTUAL int RandomPatches::getOutputPlanes() const {
    return numPlanes;
}
VIRTUAL int RandomPatches::getPersistSize() const {
    return 0;
}
VIRTUAL bool RandomPatches::providesErrorsForUpstreamWrapper() const {
    return false;
}
VIRTUAL bool RandomPatches::hasResultsWrapper() const {
    return false;
}
VIRTUAL void RandomPatches::propagate() {
    float *upstreamResults = previousLayer->getResults();
    for( int n = 0; n < batchSize; n++ ) {
        int patchMargin = inputBoardSize - outputBoardSize;
        int patchRow = patchMargin / 2;
        int patchCol = patchMargin / 2;
        if( training ) {
            patchRow = MyRandom::instance()->uniformInt( 0, patchMargin );
            patchCol = MyRandom::instance()->uniformInt( 0, patchMargin );
        }
        PatchExtractor::extractPatch( n, numPlanes, inputBoardSize, patchSize, patchRow, patchCol, upstreamResults, results );
    }
}
VIRTUAL std::string RandomPatches::asString() const {
    return "RandomPatches{ inputPlanes=" + toString(numPlanes) + " inputBoardSize=" + toString(inputBoardSize) + " patchSize=" + toString( patchSize ) + " }";
}


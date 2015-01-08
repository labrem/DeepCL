#pragma once

#include "Propagate.h"

class Propagate2 : public Propagate {
public:
    CLKernel *kernel;

    // [[[cog
    // import cog_addheaders
    // cog_addheaders.add()
    // ]]]
    // classname: Propagate2
    // cppfile: Propagate2.cpp

    Propagate2( OpenCLHelper *cl, LayerDimensions dim, ActivationFunction *fn );
    VIRTUAL ~Propagate2();
    VIRTUAL void propagate( int batchSize, CLWrapper *dataWrapper, CLWrapper *weightsWrapper, CLWrapper *biasWeightsWrapper,
    CLWrapper *resultsWrapper );

    // [[[end]]]

};

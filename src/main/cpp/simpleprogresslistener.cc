//
// Created by Ruben Taelman on 22/08/16.
//

#include "simpleprogresslistener.h"
#include <iostream>

using namespace std;

void SimpleProgressListener::notifyProgress(float level, const char *section) {
    cerr << "\r" << section << ": " << level << flush;
}

void SimpleProgressListener::notifyProgress(float task, float level, const char *section) {
    cerr << "\r" << section << ": " << level << " / " << task << flush;
}

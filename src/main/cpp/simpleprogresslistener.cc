//
// Created by Ruben Taelman on 22/08/16.
//

#include "simpleprogresslistener.h"
#include <iostream>

void SimpleProgressListener::notifyProgress(float level, const char *section) {
    std::cerr << "\r" << section << ": " << level << std::flush;
}

void SimpleProgressListener::notifyProgress(float task, float level, const char *section) {
    std::cerr << "\r" << section << ": " << level << " / " << task << std::flush;
}

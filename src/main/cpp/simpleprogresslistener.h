//
// Created by Ruben Taelman on 22/08/16.
//

#ifndef TPFPATCH_STORE_SIMPLEPROGRESSLISTENER_H
#define TPFPATCH_STORE_SIMPLEPROGRESSLISTENER_H

#include <HDTListener.hpp>

class SimpleProgressListener : public hdt::ProgressListener {
    void notifyProgress(float level, const char *section);
    void notifyProgress(float task, float level, const char *section);
};

#define NOTIFYLVL(listener, message, number) \
    if((listener)!=NULL) (listener)->notifyProgress((number), (message));
#define NOTIFYMSG(listener, message) \
    if((listener)!=NULL) (listener)->notifyProgress(0, message);

#endif //TPFPATCH_STORE_SIMPLEPROGRESSLISTENER_H

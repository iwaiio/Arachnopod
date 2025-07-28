#include "cvs.hpp"

int main() {
    RenderManager rndr;
    rndr.version();
    
    CVS cvs;
    cvs.initialize();
    cvs.mainLoop();
    return 0;
}
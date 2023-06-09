
#include "dom.hpp"

Component   TopBarComponent()
{
    return (Renderer([](){
        return text("Top Bar");
    }));
}


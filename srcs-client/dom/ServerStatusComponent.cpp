#include "dom.hpp"

Component   ServerStatusComponent()
{
    return (Renderer([](){
        return text("ServerStatus");
    }));
}
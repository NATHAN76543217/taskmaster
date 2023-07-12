#include "dom.hpp"
\
Component   OutputLogComponent()
{
    return (Renderer([](){
        return text("output log");
    }));
}

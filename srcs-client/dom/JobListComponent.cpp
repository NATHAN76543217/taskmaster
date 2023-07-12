#include "dom.hpp"

Component   JobsListComponent()
{
    return (Renderer([](){
        return text("Jobs list");
    }));
}

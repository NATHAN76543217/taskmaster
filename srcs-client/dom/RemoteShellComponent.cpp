#include "dom.hpp"

Component   RemoteShellComponent()
{
    return (Renderer([](){
        return text("remote shell");
    }));
}

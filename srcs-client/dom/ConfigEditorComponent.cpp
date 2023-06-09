#include "dom.hpp"

Component   ConfigEditorComponent()
{
    return (Renderer([](){
        return text("config editor");
    }));
}
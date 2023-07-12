#pragma once

#include "ftxui/dom/elements.hpp"
#include "ftxui/screen/screen.hpp"
#include "ftxui/screen/string.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/component/component_base.hpp"
#include "ftxui/component/component_options.hpp"

using namespace ftxui;

// get size relative to the size of the terminal, for x and y.
#define RELATIVE_X(percent) ((percent * Terminal::Size().dimx) / 100)
#define RELATIVE_Y(percent) ((percent * Terminal::Size().dimy) / 100)

struct SplitLayoutState
{
    // left bar components size
    // relative to serverStatus component
    int serverStatusSize;
    int jobListSize;

    // right bar components size
    // relative to configEditor component
    int configEditorSize;

    // content size relative between left/right
    int contentSize;

    // relative between topBar and contentSize
    int mainContentSize;
};

// main info bar
Component   TopBarComponent();


// left component content
Component   ServerStatusComponent();
Component   JobsListComponent();
Component   RemoteShellComponent();
// left component
Component   leftTab(SplitLayoutState&);



// right component content
Component   ConfigEditorComponent();
Component   OutputLogComponent();
// right component
Component   rightTab(SplitLayoutState&);

#include "dom.hpp"


Component   leftTab(SplitLayoutState& state)
{
    auto container = RemoteShellComponent();
    container = ResizableSplitTop(JobsListComponent(), container, &state.jobListSize);
    container = ResizableSplitTop(ServerStatusComponent(), container, &state.serverStatusSize);
    return (container);
}

Component   rightTab(SplitLayoutState& state)
{
    auto container = OutputLogComponent();
    container = ResizableSplitTop(ConfigEditorComponent(), container, &state.configEditorSize);
    return (container);
}



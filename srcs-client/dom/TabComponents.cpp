#include "dom.hpp"


Component   leftTab(SplitLayoutState& state)
{
    auto container = ServerStatusComponent();
    container = ResizableSplitBottom(JobsListComponent(), container, &state.jobListSize);
    container = ResizableSplitBottom(RemoteShellComponent(), container, &state.remoteShellSize);
    return (container);
}

Component   rightTab(SplitLayoutState& state)
{
    auto container = ConfigEditorComponent();
    container = ResizableSplitBottom(OutputLogComponent(), container, &state.outputLogSize);
    return (container);
}



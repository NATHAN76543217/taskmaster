#include <iostream>
#include "cpp_argparse.hpp"
#include "net/client.hpp"
#include "dto.hpp"

#include "ftxui/dom/elements.hpp"
#include "ftxui/screen/screen.hpp"
#include "ftxui/screen/string.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/component/component_base.hpp"
#include "ftxui/component/component_options.hpp"

# define TM_CLIENT_VERSION "0.0.1"
// #include "dom.hpp"

//TODO log disconnetion in taskmasterctl
class ClientData
{
    // data storage for client to use in handlers
};




class TaskmasterClientHandler : public ClientHandler<TaskmasterClientHandler, ClientData>
{
    public:
        TaskmasterClientHandler(client_type& Client)
        : handler_type(Client)
        {}

        void onConnected()
        {
            StatusDTO dto;
            dto.value = 101;
            this->client.emit("status", dto);
        }

        void declareMessages()
        {
            this->client.onMessage("status_response", 
                client_type::make_handler<StatusDTO>([](client_type& client, DTO* dto)
                {
                    StatusDTO *status = reinterpret_cast<StatusDTO*>(dto);
                    LOG_INFO(LOG_CATEGORY_NETWORK, "Received status response from server with value: " << status->value);
                    client.disconnect();
                }
            ));
        }
};


int main(int ac, char **av)
{
    bool        help = false;

    std::string serverIp;
    int         serverPort;

	ARG_INIT(
		ARG_GROUP("server", "Daemonized server running taskmaster",
			ARG<std::string>("-i", "--ip", "ip address of taskmaster server", &serverIp, ARG_NOFLAGS ),
			ARG<int>("-p", "--port", "port of taskmaster server", &serverPort, ARG_NOFLAGS )
		),
		ARG_NOVALUE("-h", "--help", "shows this usage", &help)
	);


	int parsing_failed = ARG_PARSE(ac, av);
	if (parsing_failed)
	{
		ARG_USAGE("  === Taskmaster client === ");
		return (EXIT_FAILURE);
	}
	if (help)
	{
		ARG_USAGE("  === Taskmaster client ===");
		return (EXIT_SUCCESS);
	}


    #if LOG_CATEGORY_AUTO == false
        int err = 0;
        err += Tintin_reporter::getLogManager("./client.log").addCategory(LOG_CATEGORY_DEFAULT);
        err += Tintin_reporter::getLogManager().addCategory(LOG_CATEGORY_INIT);
        err += Tintin_reporter::getLogManager().addCategory(LOG_CATEGORY_NETWORK);
        err += Tintin_reporter::getLogManager().addCategory(LOG_CATEGORY_SIGNAL, "./signal.log");
        err += Tintin_reporter::getLogManager().addCategory(LOG_CATEGORY_CONFIG);
        std::cout << "init err: " << err << std::endl;
    #endif


    // Client<TaskmasterClientHandler> *client = new Client<TaskmasterClientHandler>(serverIp, serverPort);
    // client->connect();

    // do
    // {
    // } while (client->wait_update());
    
    // delete client;
	// auto screen = ScreenInteractive::Fullscreen();

	// SplitLayoutState splitLayoutState;
	// splitLayoutState.serverStatusSize = RELATIVE_Y(30);
	// splitLayoutState.jobListSize = RELATIVE_Y(48);
	// splitLayoutState.configEditorSize = RELATIVE_Y(70);
	// splitLayoutState.contentSize = RELATIVE_X(50);
	// splitLayoutState.mainContentSize = RELATIVE_Y(90);

	// auto contentContainer = leftTab(splitLayoutState);
	// contentContainer = ResizableSplitRight(rightTab(splitLayoutState), contentContainer, &splitLayoutState.contentSize);

	// auto mainContainer = TopBarComponent();
	// mainContainer = ResizableSplitBottom(contentContainer, mainContainer, &splitLayoutState.mainContentSize);

	// using namespace ftxui;


using namespace ftxui;

    int main_menu_index = 0;

    std::vector<std::string> main_menu = {
        "connect", "client config", "client info"
    };

    auto tab_commands =
        Menu(&main_menu, &main_menu_index, MenuOption::VerticalAnimated()) | center;


    auto screen = ScreenInteractive::Fullscreen();

    // top
    auto topBar = Renderer([] { return text("Taskmaster Client (v " TM_CLIENT_VERSION ")") | center; });

	# define TEXT_COLOR Color(200, 200, 200)
	# define BORDER_COLOR Color::White
	
    // left
    auto jobList = Renderer([] { return window(text("Jobs"),
			text("Jobs content") | center | color(TEXT_COLOR) | bgcolor(Color(0, 0, 0)))
		| color(Color(200,0,0)); });

	auto serverTitle = Renderer([] { return text("Server Status content") | center;  });
    Component serverContainer = Container::Vertical({
		serverTitle,
		serverTitle,
	});
    auto serverWindow = Renderer([] { return window(text("Server Status"), 
			paragraphAlignLeft("Server Status content") | color(TEXT_COLOR) | bgcolor(Color(0, 0, 0)) )
		| color(Color(200,200,200)); });
// | borderStyled(LIGHT, Color::White)

    auto remoteShell = Renderer([] { return 
		window(
			text("Remote Shell") | color(TEXT_COLOR),
			text("TODO Shell component...") | color(TEXT_COLOR)
		) | color(BORDER_COLOR);
	});

    // right
    auto configEditor = Renderer([] { return
    	window(
			text("Config Editor") | color(TEXT_COLOR) | center,
			paragraphAlignLeft("TODO Config component...") | color(TEXT_COLOR) | bgcolor(Color(0, 0, 0))
		) | color(BORDER_COLOR);
	});

	auto outputLog = Renderer([] { return
	window(
		text("Output Log") | color(TEXT_COLOR),
		text("TODO Output component...") | color(TEXT_COLOR) | bgcolor(Color::Black)
	) | color(BORDER_COLOR); });


    int serverStatusSize = 10;
    int outputLogSize = 10;
    int remoteShellSize = 10;


    auto leftContainer = jobList;
    leftContainer = ResizableSplitBottom(serverWindow, leftContainer, &serverStatusSize) | color(Color::Black) | borderEmpty;// borderStyled(LIGHT, Color::Red);
    leftContainer = ResizableSplitBottom(remoteShell, leftContainer, &remoteShellSize) | color(Color::Black) | borderEmpty;

    auto rightContainer = configEditor;
    rightContainer = ResizableSplitBottom(outputLog, rightContainer, &outputLogSize) | color(Color::Black)  | borderEmpty;

    int rightContainerSize = 10;
    auto contentContainer = leftContainer;
    contentContainer = ResizableSplitRight(rightContainer, contentContainer, &rightContainerSize) | color(Color::Black)  | borderEmpty;
    

    int contentContainerSize = 1;
    auto mainContainer = topBar;
    mainContainer = ResizableSplitBottom(contentContainer, mainContainer, &contentContainerSize) | borderEmpty;

    auto renderer =
        Renderer(mainContainer, [&] { return mainContainer->Render() | borderStyled(ROUNDED, Color::White) | vscroll_indicator | bgcolor(Color::Black); });

    //   screen.Print(renderer);
    screen.Loop(renderer);
	// auto renderer = Renderer(mainContainer, [&] { return mainContainer->Render() | border; });

	// screen.Loop(renderer);

    return (EXIT_SUCCESS);

}



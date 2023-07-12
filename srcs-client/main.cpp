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

{


	using namespace ftxui;

	// auto summary = [&] {
	// 	auto content = vbox({
	// 		hbox({text(L"- done:   "), text(L"3") | bold}) | color(Color::Green),
	// 		hbox({text(L"- done:   "), text(L"3") | bold}) | color(Color::Green),
	// 		hbox({text(L"- active: "), text(L"2") | bold}) | color(Color::RedLight),
	// 		hbox({text(L"- queue:  "), text(L"9") | bold}) | color(Color::Red),
	// 	});
	// 	return window(text(L" Summary "), content);
	// };

	// auto document =  //
	// 	vbox({
	// 		hbox({
	// 			summary(),
	// 			summary(),
	// 			summary() | flex,
	// 		}),
	// 		summary(),
	// 		summary(),
	// 	});

	// // Limit the size of the document to 80 char.
	// document = document | size(WIDTH, LESS_THAN, 80);

	// auto screen = Screen::Create(Dimension::Full(), Dimension::Fit(document));
	// Render(screen, document);

// 	auto screen = ScreenInteractive::Fullscreen();

//   int tab_index = 0;
//   int main_menu_index = 0;
//   std::vector<std::string> tab_entries = {
// //       "htop", "color", "spinner", "gauge", "compiler", "paragraph",
// //   };
//   std::vector<std::string> main_menu = {
//       "connect", "client config", "client info"
//   };

//     Elements line1;
// 	line1.push_back(text("line1") | bold | hcenter);
//   auto tab_selection =
//       Menu(&tab_entries, &tab_index, MenuOption::VerticalAnimated());
//   auto tab_commands =
//       Menu(&main_menu, &main_menu_index, MenuOption::VerticalAnimated()) | center;
// 	auto shell_placeholder = vbox({
// 		line1
// 	});
// //   auto main_container = Container::Vertical({
// //       tab_selection,
// //     //   tab_content,
// //   });
//     Elements line2;
// 	line2.push_back(text("line2") | bold | hcenter);
//     Elements line3;
// 	line3.push_back(text("line3") | bold | hcenter);

//     Elements description_box;
// 	description_box.push_back(text("Description") | bold | hcenter);

//     auto main_container = Container::Horizontal({
//       Container::Vertical({
//           Container::Horizontal({
// 			tab_selection,
// 	        // line1
//           })| border,
// 		  tab_commands,
//       }),
// 	//   shell_placeholder
// 	        // line3
//   });

//  auto main_renderer = Renderer(main_container, [&] {
//     return vbox({
//         text("Taskmasterctl") | bold | hcenter,
//         hbox({
// 		// description_box | border,
// 		tab_selection->Render() | border,
// 		tab_commands->Render() | border,
            //   spinner(i, index) | bold,

// 		}),
//         // tab_content->Render() | flex,
// 		shell_placeholder
//     });
//   });

// 	std::cout << screen.ToString() << '\0' << std::endl;
	
// 	screen.Loop(main_renderer);
	
// 	  auto screen = ScreenInteractive::Fullscreen();
 
//   auto leftcontainer = Renderer([] { return text("left container") | size(HEIGHT, EQUAL, 20) | center ; });
//   auto left = Renderer([] { return text("Left") | center; });
//   auto right = Renderer([] { return text("right") | center; });
//   auto top2 = Renderer([] { return text("top2") | center; });
//   auto top3 = Renderer([] { return text("top3") | center; });
//   auto top = Renderer([] { return text("top") | center; });
//   auto middle = Renderer([] { return text("middle") | size(HEIGHT, EQUAL, 20) | center ; });
//   auto bottom = Renderer([] { return text("bottom") | center; });
 
// //   int left_size = 20;
//   int right_size = 20;
//   int top_size = 10;
// //   int bottom_size = 10;
 
//   auto container_top = top;
//   auto container_middle = middle;
//   auto container_bottom = bottom;
// //   container = ResizableSplitLeft(tab_commands, container, &left_size);
//   container_bottom = ResizableSplitRight(right, container_bottom, &right_size);
//     container_middle = ResizableSplitBottom(bottom, container_middle, &top_size);
//     container_top = ResizableSplitBottom(middle, container_top, &top_size);
//     // container = ResizableSplitTop(top3, container, &top_size);
// //   container = ResizableSplitBottom(bottom, container, &bottom_size);
 
//   auto renderer =
//       Container::Vertical({ container_top->Render() | border, container_bottom->Render()});
 
// //   screen.Print(renderer);
//   screen.Loop(renderer);

// }
//     return (EXIT_SUCCESS);
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

    return (EXIT_SUCCESS);
}
}



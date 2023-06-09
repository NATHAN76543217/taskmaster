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
			ARG<std::string>("-i", "--ip", "ip address of taskmaster server", &serverIp, ARG_REQUIRED),
			ARG<int>("-p", "--port", "port of taskmaster server", &serverPort, ARG_REQUIRED)
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


    Client<TaskmasterClientHandler> *client = new Client<TaskmasterClientHandler>(serverIp, serverPort);
    client->connect();

    do
    {
    } while (client->wait_update());
    
    delete client;

{


	using namespace ftxui;

	auto summary = [&] {
		auto content = vbox({
			hbox({text(L"- done:   "), text(L"3") | bold}) | color(Color::Green),
			hbox({text(L"- done:   "), text(L"3") | bold}) | color(Color::Green),
			hbox({text(L"- active: "), text(L"2") | bold}) | color(Color::RedLight),
			hbox({text(L"- queue:  "), text(L"9") | bold}) | color(Color::Red),
		});
		return window(text(L" Summary "), content);
	};

	auto document =  //
		vbox({
			hbox({
				summary(),
				summary(),
				summary() | flex,
			}),
			summary(),
			summary(),
		});

	// Limit the size of the document to 80 char.
	document = document | size(WIDTH, LESS_THAN, 80);

	// auto screen = Screen::Create(Dimension::Full(), Dimension::Fit(document));
	// Render(screen, document);

// 	auto screen = ScreenInteractive::Fullscreen();

//   int tab_index = 0;
  int main_menu_index = 0;
//   std::vector<std::string> tab_entries = {
//       "htop", "color", "spinner", "gauge", "compiler", "paragraph",
//   };
  std::vector<std::string> main_menu = {
      "connect", "client config", "client info"
  };

//     Elements line1;
// 	line1.push_back(text("line1") | bold | hcenter);
//   auto tab_selection =
//       Menu(&tab_entries, &tab_index, MenuOption::VerticalAnimated());
  auto tab_commands =
      Menu(&main_menu, &main_menu_index, MenuOption::VerticalAnimated()) | center;
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
	
	  auto screen = ScreenInteractive::Fullscreen();
 
  auto middle = Renderer([] { return text("middle") | size(HEIGHT, EQUAL, 20) | center ; });
  auto left = Renderer([] { return text("Left") | center; });
  auto right = Renderer([] { return text("right") | center; });
  auto top = Renderer([] { return text("top") | center; });
  auto bottom = Renderer([] { return text("bottom") | center; });
 
  int left_size = 20;
  int right_size = 20;
  int top_size = 10;
  int bottom_size = 10;
 
  auto container = middle;
  container = ResizableSplitLeft(tab_commands, container, &left_size);
  container = ResizableSplitRight(right, container, &right_size);
  container = ResizableSplitTop(top, container, &top_size);
  container = ResizableSplitBottom(bottom, container, &bottom_size);
 
  auto renderer =
      Renderer(container, [&] { return container->Render() | border; });
 
//   screen.Print(renderer);
  screen.Loop(renderer);

}
    return (EXIT_SUCCESS);
}



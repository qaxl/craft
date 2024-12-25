#include "app.hpp"

#include <iostream>

#include <SDL3/SDL.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>
#include <steam/steam_api.h>
#include <volk.h>

#include "graphics/vulkan/renderer.hpp"
#include "graphics/widgets/background_settings_widget.hpp"
#include "graphics/widgets/render_time_widget.hpp"
#include "graphics/widgets/util_widget.hpp"
#include "graphics/widgets/widget.hpp"
#include "steam/isteamapps.h"
#include "steam/isteamfriends.h"
#include "steam/isteammatchmaking.h"
#include "steam/isteamnetworkingsockets.h"
#include "steam/isteamnetworkingutils.h"
#include "steam/steam_api_common.h"
#include "util/error.hpp"

namespace craft {
App::~App() {
  SDL_Quit();
  SteamAPI_Shutdown();
}

// Callback Registration
class SteamCallbackHandler {
public:
  // Constructor to hook the callback
  SteamCallbackHandler() : m_SteamJoinGameCallback(this, &SteamCallbackHandler::OnJoinGame) {}

private:
  STEAM_CALLBACK(SteamCallbackHandler, OnJoinGame, GameRichPresenceJoinRequested_t);
  CCallback<SteamCallbackHandler, GameRichPresenceJoinRequested_t> m_SteamJoinGameCallback;
};

void SteamCallbackHandler::OnJoinGame(GameRichPresenceJoinRequested_t *pCallback) {
  // pCallback->m_rgchConnect contains the connection string
  std::cout << "Join request received from: " << pCallback->m_rgchConnect << std::endl;

  // Process the join request, e.g., connect to the game session
  std::string connectionString = pCallback->m_rgchConnect;
  // Call your game's logic to handle joining, like:
  // ConnectToGame(connectionString);
}

App::App() {
  if (!SteamAPI_Init()) {
    // TODO: not make this forced.
    RuntimeError::Throw("Steamworks API couldn't initialize.");
  }

  if (SDL_Init(SDL_INIT_VIDEO) == false) {
    RuntimeError::Throw("SDL Initialization Failure", EF_AppendSDLErrors);
  }

  if (volkInitialize() != VK_SUCCESS) {
    RuntimeError::Throw(
        "Vulkan Initialization Failure: Couldn't load Vulkan library functions. Does this computer support rendering "
        "with Vulkan? Currently, we do not support graphics APIs other than Vulkan.\nOpenGL support may be added in "
        "the "
        "future for better compatibility with older hardware.\nIf you have hardware that is newer than, let's say, the "
        "last 5 to 10 years, then it most likely does support Vulkan 1.3. If that's the case, consider updating your "
        "graphics drivers.\n\nRead more for AMD graphics cards: "
        "https://www.amd.com/en/support/download/drivers.html\nRead more for NVIDIA graphics cards: "
        "https://www.nvidia.com/en-us/drivers/\n\nIf upgrading your drivers doesn't fix this issue, you most likely do "
        "not have support for Vulkan 1.3, which is the minimum required version for this application to run.\nAnd if "
        "you "
        "have the ability to program in OpenGL/DirectX 11, proficiently, you're welcome contributing into the "
        "framework that the current application is running from https://github.com/qaxl/craft :)");
  }

  m_window = std::make_shared<Window>(1024, 768, "test");
  m_renderer = std::make_shared<vk::Renderer>(m_window);

  m_widget_manager = std::make_shared<WidgetManager>();
  m_widget_manager->AddWidget(std::make_unique<UtilWidget>());
  m_widget_manager->AddWidget(std::make_unique<RenderTimingsWidget>());
  m_widget_manager->AddWidget(std::make_unique<BackgroundSettingsWidget>(m_renderer));

  std::cout << "Hi, " << SteamFriends()->GetPersonaName() << "!" << std::endl;

  // char server_or_client;
  // std::cin >> server_or_client;

  // SteamNetworkingUtils()->InitRelayNetworkAccess();
  // SteamNetworkingSockets()->InitAuthentication();
  // SteamFriends()->SetRichPresence("status", "Deliberating the existence...");
  // // SteamFriends()->SetRichPresence("steam_player_group", "fadaba");
  // // SteamFriends()->SetRichPresence("steam_player_group_size", "4");
  // SteamFriends()->SetRichPresence("connect", "4");

  // if (server_or_client == 's') {
  //   HSteamListenSocket socket = SteamNetworkingSockets()->CreateListenSocketP2P(0, 0, nullptr);
  //   if (socket == k_HSteamListenSocket_Invalid) {
  //     RuntimeError::Throw("Couldn't host a P2P server.");
  //   }

  //   ISteamNetworkingMessage *pIncomingMsg = nullptr;
  //   while (socket != k_HSteamListenSocket_Invalid) {
  //     int numMsgs = SteamNetworkingSockets()->ReceiveMessagesOnConnection(socket, &pIncomingMsg, 1);

  //     if (numMsgs < 1)
  //       continue;

  //     std::cout << "Received message on socket: " << pIncomingMsg->GetData() << std::endl;
  //     pIncomingMsg->Release();

  //     SteamNetworkingSockets()->SendMessageToConnection(pIncomingMsg->GetConnection(), "WE GOT IT!", 11,
  //                                                       k_nSteamNetworkingSend_Reliable, nullptr);
  //   }
  // } else {
  //   // SteamNetworkingSockets()->ConnectP2P()
  // }
}

bool App::Run() {
  uint64_t start_tick = SDL_GetTicksNS();
  uint64_t end_tick = SDL_GetTicksNS();

  bool render_stats = true;
  bool bg_settings = true;

  SteamCallbackHandler handler;

  while (m_window->IsOpen()) {
    if (RuntimeError::HasAnError()) {
      // The main function
      return false;
    }

    start_tick = SDL_GetTicksNS();
    float tick_difference = start_tick - end_tick;
    end_tick = start_tick;

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    m_widget_manager->RenderWidgets();

    ImGui::Render();

    m_renderer->Draw();
    m_window->PollEvents();

    SteamAPI_RunCallbacks();
  }

  return true;
}
} // namespace craft

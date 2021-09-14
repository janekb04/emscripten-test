#ifndef __EMSCRIPTEN__
#include <GL/glew.h>
#else
#include <emscripten.h>
#endif

#ifdef _WIN32

#include "windows_swca.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include <glfwpp/native.h>

#endif

#include <array>
#include <filesystem>
#include <glfwpp/glfwpp.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_internal.h>
#include <iostream>

#include "ImGuiFileDialog.h"

void cleanupImgui() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

template <typename Func> void renderImgui(Func &&guiRenderFunc_) {
  ImGui_ImplGlfw_NewFrame();
  ImGui_ImplOpenGL3_NewFrame();
  ImGui::NewFrame();

  guiRenderFunc_();

  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

  if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    glfw::Window &backupCurrentContext = glfw::getCurrentContext();
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
    glfw::makeContextCurrent(backupCurrentContext);
  }
}

void initImgui(const glfw::Window &wnd) {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
#ifndef __EMSCRIPTEN__
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
#endif

  ImGuiStyle &style = ImGui::GetStyle();

  ImGui_ImplGlfw_InitForOpenGL(wnd, true);
  ImGui_ImplOpenGL3_Init();
}

class WindowResizer {
  const int OP_RESIZE_LEFT = 1, OP_RESIZE_TOP = 2, OP_RESIZE_RIGHT = 4,
            OP_RESIZE_BOTTOM = 8, OP_MOVE = 16;
  int state;
  glfw::Window &wnd;
  const ImGuiWindow *menuBarWindow;

  const int TITLE_BAR_SIZE = 15;
  const int BORDER_WIDTH = 5;
  const int MIN_HEIGHT = 20;
  const int MIN_WIDTH = 20;

public:
  WindowResizer(glfw::Window &wnd) : state{0}, wnd{wnd} {}

  void next() {
    // Stop any op if no longer holding mouse
    if (!wnd.getMouseButton(glfw::MouseButton::Left)) {
      if (state == OP_MOVE) {
        state = 0;
      } else if (state != 0) { // resizing in some way
        state = 0;
        // acrylic_swca(glfw::native::getWin32Window(wnd));
      }
    }

    const auto [mouse_x, mouse_y] = wnd.getCursorPos();
    const auto [mouse_dx, mouse_dy] =
        ImGui::GetMouseDragDelta(ImGuiMouseButton_Left, 0);
    const auto [wnd_width, wnd_height] = wnd.getSize();

    // Potentially pick new current state
    if ((!ImGui::IsAnyItemActive() ||
         ImGui::GetCurrentContext()->ActiveIdWindow == menuBarWindow) &&
        !wnd.getAttribMaximized()) {
      const int dst_to_left_edge = abs(mouse_x);
      const int dst_to_right_edge = abs(wnd_width - dst_to_left_edge);
      const int dst_to_top_edge = abs(mouse_y);
      const int dst_to_bottom_edge = abs(wnd_height - dst_to_top_edge);

      // When Hovering...
      if (state == 0) {
        if (dst_to_left_edge < BORDER_WIDTH) {
          ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        }
        if (dst_to_right_edge < BORDER_WIDTH) {
          ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        }
        if (dst_to_bottom_edge < BORDER_WIDTH) {
          ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
        }
        if (dst_to_top_edge < BORDER_WIDTH) {
          ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
        }
      }

      // When dragging...
      if (state == 0 && wnd.getMouseButton(glfw::MouseButton::Left)) {
        const bool on_title_bar =
            ImGui::GetCurrentContext()->ActiveIdWindow == menuBarWindow;
        const bool in_drag_location =
            !ImGui::GetIO().WantCaptureMouse || on_title_bar;
        if (in_drag_location && ImGui::IsMouseDragging(0)) {
          state = OP_MOVE;
        } else {
          if (dst_to_left_edge < BORDER_WIDTH) {
            state |= OP_RESIZE_LEFT;
          }
          if (dst_to_right_edge < BORDER_WIDTH) {
            state |= OP_RESIZE_RIGHT;
          }
          if (dst_to_bottom_edge < BORDER_WIDTH) {
            state |= OP_RESIZE_BOTTOM;
          }
          if (dst_to_top_edge < BORDER_WIDTH) {
            state |= OP_RESIZE_TOP;
          }

          if (state != 0) {
            // acrylic_swca_disable(glfw::native::getWin32Window(wnd));
          }
        }
      }
    } else if (ImGui::GetCurrentContext()->ActiveIdWindow == menuBarWindow &&
               wnd.getAttribMaximized() && ImGui::IsMouseDragging(0)) {
      wnd.restore();
      auto [wnd_width, wnd_height] = wnd.getSize();
      wnd.setPos(mouse_x - wnd_width / 2, mouse_y - 10);
      state = OP_MOVE;
    }

    const auto [wnd_x, wnd_y] = wnd.getPos();

    // Perform current op
    {
      if (state == OP_MOVE) {
        auto [x, y] = wnd.getPos();
        wnd.setPos(x + mouse_dx, y + mouse_dy);
      } else {
        if (state & OP_RESIZE_BOTTOM) {
          ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
          const auto new_height = wnd_height + mouse_dy;
          if (new_height >= MIN_HEIGHT)
            wnd.setSize(wnd_width, new_height);
        }
        if (state & OP_RESIZE_RIGHT) {
          ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
          const auto new_width = wnd_width + mouse_dx;
          if (new_width >= MIN_WIDTH)
            wnd.setSize(new_width, wnd_height);
        }
        if (state & OP_RESIZE_TOP) {
          ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
          const auto new_height = wnd_height - mouse_dy;
          if (new_height >= MIN_HEIGHT)
            wnd.setSize(wnd_width, new_height);
          wnd.setPos(wnd_x, wnd_y + mouse_dy);
        }
        if (state & OP_RESIZE_LEFT) {
          ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
          const auto new_width = wnd_width - mouse_dx;
          if (new_width >= MIN_WIDTH)
            wnd.setSize(new_width, wnd_height);
          wnd.setPos(wnd_x + mouse_dx, wnd_y);
        }
      }
    }

    if (state != 0)
      ImGui::ResetMouseDragDelta();
  }

  void setMenuBarWindow(const ImGuiWindow *wnd) { menuBarWindow = wnd; }
};

WindowResizer *resizer;
glfw::Window *mainWindow;

void render() {
  glClear(GL_COLOR_BUFFER_BIT);
  renderImgui([]() {
    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(),
                                 ImGuiDockNodeFlags_PassthruCentralNode);

    if (ImGui::BeginMainMenuBar()) {
      if (ImGui::BeginMenu("File")) {
        ImGui::MenuItem("New", "Ctrl + N");
        ImGui::MenuItem("Open", "Ctrl + O");
        ImGui::MenuItem("Save", "Ctrl + S");
        ImGui::MenuItem("Save As", "Ctrl + Shift + S");
        ImGui::Separator();
        ImGui::MenuItem("Exit", "Ctrl + Q");
        ImGui::EndMenu();
      }
      if (ImGui::BeginMenu("Edit")) {
        ImGui::MenuItem("Undo", "Ctrl + Z");
        ImGui::MenuItem("Redo", "Ctrl + Y");
        ImGui::Separator();
        ImGui::MenuItem("Cut", "Ctrl + X");
        ImGui::MenuItem("Copy", "Ctrl + C");
        ImGui::MenuItem("Paste", "Ctrl + V");
        ImGui::EndMenu();
      }

      {
        struct options_t {
          int select_idx = 0;
          std::array<const char *, 4> optionList = {"Hello", "World", "Test",
                                                    "Hi"};
        };
        static options_t options;
        static char currentText[255];

        auto inputCallback = [](ImGuiInputTextCallbackData *data) {
          options_t &options = *reinterpret_cast<options_t *>(data->UserData);
          if (data->EventFlag == ImGuiInputTextFlags_CallbackCompletion) {
            data->InsertChars(
                data->CursorPos,
                std::data(options.optionList)[options.select_idx]);
            data->InsertChars(data->CursorPos, " ");
          } else if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory) {
            if (data->EventKey == ImGuiKey_UpArrow) {
              --options.select_idx;
            } else if (data->EventKey == ImGuiKey_DownArrow) {
              ++options.select_idx;
            }
            options.select_idx += options.optionList.size();
            options.select_idx %= options.optionList.size();
          } else if (data->EventFlag == ImGuiInputTextFlags_CallbackEdit) {
            // Toggle casing of first character
            char c = data->Buf[0];
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
              data->Buf[0] ^= 32;
            data->BufDirty = true;
            int *p_int = (int *)data->UserData;
            *p_int = *p_int + 1;
          }
          return 0;
        };

        ImGui::InputTextWithHint("##searchText", "Run command...",
                                 &currentText[0], 255,
                                 ImGuiInputTextFlags_CallbackCompletion |
                                     ImGuiInputTextFlags_CallbackHistory,
                                 inputCallback, &options);

        auto textInputState = ImGui::GetInputTextState(ImGui::GetItemID());
        ImVec2 textPos{ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y};

        static bool popup_focused_last_frame = false;
        if (ImGui::IsItemActive() || popup_focused_last_frame) {
          popup_focused_last_frame = false;
          ImGui::OpenPopup("##SearchBar");

          ImGui::SetNextWindowPos(
              ImVec2{ImGui::GetCurrentContext()->PlatformImePos.x,
                     ImGui::GetItemRectMax().y},
              ImGuiCond_Always);

          if (ImGui::BeginPopup("##SearchBar",
                                ImGuiWindowFlags_ChildWindow |
                                    ImGuiWindowFlags_NoNavInputs)) {
            int i = 0;
            ImGui::PushAllowKeyboardFocus(false);
            for (auto &&option : options.optionList) {
              if (ImGui::Selectable(option, options.select_idx == i,
                                    ImGuiSelectableFlags_DontClosePopups)) {
              }

              ++i;
            }
            ImGui::PopAllowKeyboardFocus();
          }

          ImGui::EndPopup();
        }
      }

#ifndef __EMSCRIPTEN__
      {
        static bool freezeButtons = false;

        if (freezeButtons) {
          freezeButtons = false;
        } else {
          const float ItemSpacing = 0;

          static float HostButtonWidth = 100.0f;
          float pos = HostButtonWidth + ItemSpacing + 5;
          // ImGui::PushStyleColor(ImGuiColactive_)
          ImGui::SameLine(ImGui::GetWindowWidth() - pos);
          if (ImGui::BeginMenu("X")) {
            ImGui::EndMenu();
            mainWindow->setShouldClose(true);
            freezeButtons = true;
          }
          HostButtonWidth = ImGui::GetItemRectSize().x;

          static float ClientButtonWidth = 100.0f;
          pos += ClientButtonWidth + ItemSpacing;
          ImGui::SameLine(ImGui::GetWindowWidth() - pos);
          if (ImGui::BeginMenu("[  ]")) {
            ImGui::EndMenu();
            static bool maximized = false;
            if (maximized) {
              mainWindow->restore();
              maximized = false;
            } else {
              mainWindow->maximize();
              maximized = true;
            }
            freezeButtons = true;
          }
          ClientButtonWidth = ImGui::GetItemRectSize().x;

          static float LocalButtonWidth = 100.0f;
          pos += LocalButtonWidth + ItemSpacing;
          ImGui::SameLine(ImGui::GetWindowWidth() - pos);
          if (ImGui::BeginMenu("-")) {
            ImGui::EndMenu();
            mainWindow->iconify();
            freezeButtons = true;
          }
          LocalButtonWidth = ImGui::GetItemRectSize().x;
        }
      }
#endif

      resizer->setMenuBarWindow(ImGui::GetCurrentWindowRead());
      ImGui::EndMainMenuBar();
    }

    ImGuiViewportP *viewport =
        (ImGuiViewportP *)(void *)ImGui::GetMainViewport();
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar |
                                    ImGuiWindowFlags_NoSavedSettings |
                                    ImGuiWindowFlags_MenuBar;
    float height = ImGui::GetFrameHeight();

    if (ImGui::BeginViewportSideBar("##SecondaryMenuBar", viewport, ImGuiDir_Up,
                                    height, window_flags)) {
      if (ImGui::BeginMenuBar()) {
        ImGui::Text("0 Errors; 0 Warnings; 0 Messages;");
        ImGui::EndMenuBar();
      }
      ImGui::End();
    }

    if (ImGui::BeginViewportSideBar("##MainStatusBar", viewport, ImGuiDir_Down,
                                    height, window_flags)) {
      if (ImGui::BeginMenuBar()) {
        ImGui::Text("Status: all green");
        ImGui::EndMenuBar();
      }
      ImGui::End();
    }

    if (ImGui::BeginViewportSideBar("##LeftSidebar", viewport, ImGuiDir_Left,
                                    35,
                                    window_flags ^ ImGuiWindowFlags_MenuBar)) {
      ImGui::SetCursorPosY(ImGui::GetWindowSize().y / 2);
      ImGui::SmallButton("X");
      ImGui::SmallButton("Y");
      ImGui::SmallButton("Z");
      ImGui::SmallButton("W");

      ImGui::End();
    }

    {
      // open Dialog Simple

      ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose File",
                                              ".*", ".");

      // display
      if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey")) {
        // action if OK
        if (ImGuiFileDialog::Instance()->IsOk()) {
          std::string filePathName =
              ImGuiFileDialog::Instance()->GetFilePathName();
          std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();
          // action
        }

        // close
        ImGuiFileDialog::Instance()->Close();
      }
    }

    ImGui::ShowDemoWindow();
  });
}

void setStyle(float dpi) {
  ImGuiStyle &style = ImGui::GetStyle();
  style = ImGuiStyle{};

  style.WindowPadding = ImVec2(8.00f, 8.00f);
  style.FramePadding = ImVec2(5.00f, 2.00f);
  style.CellPadding = ImVec2(6.00f, 6.00f);
  style.ItemSpacing = ImVec2(6.00f, 6.00f);
  style.ItemInnerSpacing = ImVec2(6.00f, 6.00f);
  style.TouchExtraPadding = ImVec2(0.00f, 0.00f);
  style.IndentSpacing = 25;
  style.ScrollbarSize = 15;
  style.GrabMinSize = 10;
  style.WindowBorderSize = 1;
  style.ChildBorderSize = 1;
  style.PopupBorderSize = 1;
  style.FrameBorderSize = 1;
  style.TabBorderSize = 1;
  style.WindowRounding = 0;
  style.ChildRounding = 0;
  style.FrameRounding = 0;
  style.PopupRounding = 0;
  style.ScrollbarRounding = 0;
  style.GrabRounding = 0;
  style.LogSliderDeadzone = 0;
  style.TabRounding = 0;

  style.ScaleAllSizes(dpi);

  ImVec4 *colors = style.Colors;
  colors[ImGuiCol_Text] = ImVec4(0.78f, 0.80f, 0.81f, 1.00f);
  colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
  colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
  colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
  colors[ImGuiCol_PopupBg] = ImVec4(0.19f, 0.19f, 0.19f, 0.92f);
  colors[ImGuiCol_Border] = ImVec4(0.19f, 0.19f, 0.19f, 0.29f);
  colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.24f);
  colors[ImGuiCol_FrameBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
  colors[ImGuiCol_FrameBgHovered] = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
  colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
  colors[ImGuiCol_TitleBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
  colors[ImGuiCol_TitleBgActive] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
  colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
  colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
  colors[ImGuiCol_ScrollbarBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
  colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
  colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 0.54f);
  colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
  colors[ImGuiCol_CheckMark] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
  colors[ImGuiCol_SliderGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
  colors[ImGuiCol_SliderGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
  colors[ImGuiCol_Button] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
  colors[ImGuiCol_ButtonHovered] = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
  colors[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
  colors[ImGuiCol_Header] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
  colors[ImGuiCol_HeaderHovered] = ImVec4(0.00f, 0.00f, 0.00f, 0.36f);
  colors[ImGuiCol_HeaderActive] = ImVec4(0.20f, 0.22f, 0.23f, 0.33f);
  colors[ImGuiCol_Separator] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
  colors[ImGuiCol_SeparatorHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
  colors[ImGuiCol_SeparatorActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
  colors[ImGuiCol_ResizeGrip] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
  colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
  colors[ImGuiCol_ResizeGripActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
  colors[ImGuiCol_Tab] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
  colors[ImGuiCol_TabHovered] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
  colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.20f, 0.20f, 0.36f);
  colors[ImGuiCol_TabUnfocused] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
  colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
  colors[ImGuiCol_DockingPreview] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
  colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
  colors[ImGuiCol_PlotLines] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
  colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.77f, 0.91f, 0.99f, 1.00f);
  colors[ImGuiCol_PlotHistogram] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
  colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.77f, 0.91f, 0.99f, 1.00f);
  colors[ImGuiCol_TableHeaderBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
  colors[ImGuiCol_TableBorderStrong] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
  colors[ImGuiCol_TableBorderLight] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
  colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
  colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
  colors[ImGuiCol_TextSelectedBg] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
  colors[ImGuiCol_DragDropTarget] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
  colors[ImGuiCol_NavHighlight] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
  colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
  colors[ImGuiCol_NavWindowingDimBg] = ImVec4(1.00f, 0.00f, 0.00f, 0.20f);
  colors[ImGuiCol_ModalWindowDimBg] = ImVec4(1.00f, 0.00f, 0.00f, 0.35f);
}
extern "C" void setDPI(float dpi) {
  auto &&io = ImGui::GetIO();
  io.Fonts->Clear();
  io.Fonts->AddFontFromFileTTF("./res/Inter-VariableFont_slnt,wght.ttf",
                               int(14 * dpi));
  ImGui_ImplOpenGL3_CreateFontsTexture();
  setStyle(dpi);
}

int main(int argc, char **argv) {
  // chdir to exe location
  std::filesystem::current_path(
      std::filesystem::weakly_canonical(std::filesystem::path(argv[0]))
          .parent_path());

  [[maybe_unused]] glfw::GlfwLibrary library = glfw::init();

  glfw::WindowHints hints;
  hints.clientApi = glfw::ClientApi::OpenGl;
  hints.contextVersionMajor = 3;
  hints.contextVersionMinor = 3;
#if !defined(__EMSCRIPTEN__) && defined(_WIN32)
  hints.transparentFramebuffer = true;
  hints.decorated = false;
#endif
  hints.apply();

#ifdef __EMSCRIPTEN__
  const int WND_WIDTH =
      EM_ASM_INT({ return window.innerWidth * window.devicePixelRatio; });
  const int WND_HEIGHT =
      EM_ASM_INT({ return window.innerHeight * window.devicePixelRatio; });
#else
  const int WND_WIDTH = 800;
  const int WND_HEIGHT = 600;
#endif

  glfw::Window wnd(WND_WIDTH, WND_HEIGHT, "GLFWPP ImGui integration example");
  mainWindow = &wnd;
  WindowResizer wndResizer{wnd};
  resizer = &wndResizer;

  glfw::makeContextCurrent(wnd);

#ifndef __EMSCRIPTEN__
  if (glewInit() != GLEW_OK) {
    throw std::runtime_error("Could not initialize GLEW");
  }
#endif

  initImgui(wnd);

#ifndef __EMSCRIPTEN__
  setDPI(std::get<0>(wnd.getContentScale()));

  wnd.contentScaleEvent.setCallback(
      [](const glfw::Window &, float dpi, float) { setDPI(dpi); });
#else
  setDPI(EM_ASM_DOUBLE({ return window.devicePixelRatio; }));
#endif

  auto [r, g, b, a] = ImGui::GetStyleColorVec4(ImGuiCol_DockingEmptyBg);
  glClearColor(r, g, b, a);

#ifdef _WIN32
  acrylic_swca(glfw::native::getWin32Window(wnd));
#endif

  auto mainLoop = []() {
    render();

    glfw::pollEvents();
    mainWindow->swapBuffers();
#ifndef __EMSCRIPTEN__
    resizer->next();
#endif
  };

  wnd.framebufferSizeEvent.setCallback(
      [&](const glfw::Window &wnd, int width, int height) {
        glViewport(0, 0, width, height);
        // render();
      });

#ifndef __EMSCRIPTEN__
  while (!wnd.shouldClose()) {
    mainLoop();
  }
#else
  emscripten_set_main_loop(mainLoop, 0, true);
#endif

  cleanupImgui();
}

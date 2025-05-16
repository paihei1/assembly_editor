#include <iostream>
#include <string>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include "ctre.hpp"
#include "instructions.h"
#include "draw_main.h"
#include "changes.h"

int main(int argc, char* argv[]) {
    //try {
        // InstructionSetExtension ext = read_instruction_set_extension("x86/binary.txt");
    //}
    //catch (ParserError &e) {
    //    std::cout << e.what() << "\n";
    //}
    test_stuff();
    // return 0;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("asm",
        800, 600, SDL_WINDOW_RESIZABLE);

    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);
    ImGui::StyleColorsDark();

    int menu_height = 20;
    MainState main_state = init_example_main_state();
    MainViewPort main_view = {
        .frame = {0,menu_height,800,600 - menu_height},
        .vertical_scroll_instructions = 0,
        .side_scroll_pixels = 0,
        .zoom_vertical = 40,
        .main_state = &main_state
    };
    TTF_Init();
    TTF_Font* font = TTF_OpenFont("inputs/UbuntuSansMono-Regular.ttf", 24);
    if (!font) {
        std::cout << "could not load font.\n";
        return 0;
    }

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                running = false;
            if (event.type == SDL_EVENT_WINDOW_RESIZED) {
                main_view.frame.w = event.window.data1;
                main_view.frame.h = event.window.data2 - menu_height;
                break; // otherwise we redraw only when resizing is finished
            }
            if (main_view_port_handle_event(main_view, event, io.WantCaptureKeyboard, io.WantCaptureMouse)) break;
        }

        // Start the ImGui frame
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // --- Menu Bar ---
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New")) {
                    // Handle New action
                }
                if (ImGui::MenuItem("Open")) {
                    // Handle Open action
                }
                if (ImGui::MenuItem("Exit")) {
                    running = false;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit")) {
                if (main_state.change_count && ImGui::MenuItem("Undo")) {
                    undo_last_change(main_state.instructions, main_state.extensions, &main_state.undo_redo_list[main_state.change_count - 1]);
                    main_state.change_count--;
                }
                if (main_state.change_count != main_state.undo_redo_list.size() && ImGui::MenuItem("Redo")) {
                    apply_change(main_state.instructions, main_state.extensions, &main_state.undo_redo_list[main_state.change_count]);
                    main_state.change_count++;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Instructions")) {
                if (ImGui::MenuItem("Copy to Clipboard")) {
                    std::string result = "";
                    for (auto instruction : main_state.instructions) {
                        result += print_instruction(instruction, main_state.extensions);
                        result += "\n";
                    }
                    SDL_SetClipboardText(result.c_str());
                }
                if (ImGui::MenuItem("Paste from Clipboard")) {
                    char* clipboard = SDL_GetClipboardText();
                    main_state.instructions = load_instructions(clipboard, main_state.extensions);
                    SDL_free(clipboard);
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
        ImGui::Render();

        // ----- SDL2 Rendering (direct) -----
        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255); // Clear background
        SDL_RenderClear(renderer);

        draw_main(renderer, font, main_view);

        // Render ImGui UI (menu bar)
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);

        // Now draw your custom SDL content *below* the menu bar
        // Example: Draw a red rectangle in your "main canvas"
        
        // SDL_FRect rect = { 100, 80, 300, 200 };  // Y offset leaves space for menu
        // SDL_SetRenderDrawColor(renderer, 200, 50, 50, 255);
        // SDL_RenderFillRect(renderer, &rect);

        // Example: Draw another shape
        // SDL_SetRenderDrawColor(renderer, 50, 150, 250, 255);
        // SDL_RenderLine(renderer, 100, 350, 400, 450);

        // running = false;
        SDL_RenderPresent(renderer);
    }

    // Cleanup
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    TTF_CloseFont(font);
    TTF_Quit();
    return 0;
}
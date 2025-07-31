#include "SceneManager.h"
#include <imgui.h>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <algorithm>

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace SS
{
    SceneManager::SceneManager() {
        meshFiles.clear();
        try {
            for (const auto& entry : fs::directory_iterator("assets/models")) {
                if (entry.path().extension() == ".glb") {
                    meshFiles.push_back(entry.path().string());
                }
            }
        }
        catch (...) {
            std::cerr << "Warning: Cannot scan assets/models folder\n";
        }

        musicFiles.clear();
        try {
            for (const auto& entry : fs::directory_iterator("assets/musics")) {
                if (entry.path().extension() == ".wav" || entry.path().extension() == ".ogg" || entry.path().extension() == ".mp3") {
                    musicFiles.push_back(entry.path().string());
                }
            }
        }
        catch (...) {
            std::cerr << "Warning: Cannot scan assets/musics folder\n";
        }

        LoadFromFile("scenes.json");
        sceneNameBuf[0] = '\0';
    }

    void SceneManager::renderImGui(
        std::function<void(const std::string&)> onMeshSelected,
        std::function<void(const std::string&)> onMusicSelected,
        std::function<void(const Scene&)> onSceneLoaded,
        glm::vec3& currentLightPos,
        float& currentAmbient) {
        ImGui::Begin("Scene Editor");

        // --- MESH SELECTION ---
        ImGui::Text("Select Mesh (.glb):");
        if (ImGui::BeginCombo("##MeshCombo", meshFiles.empty() ? "None" : meshFiles[selectedMesh].c_str())) {
            for (int i = 0; i < meshFiles.size(); ++i) {
                bool sel = (selectedMesh == i);
                if (ImGui::Selectable(meshFiles[i].c_str(), sel)) {
                    selectedMesh = i;
                    if (onMeshSelected) {
                        onMeshSelected(meshFiles[i]);
                    }
                }
                if (sel) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }


        // --- MUSIC SELECTION ---
        ImGui::Text("Select Music (.ogg):");
        if (ImGui::BeginCombo("##MusicCombo", musicFiles.empty() ? "None" : musicFiles[selectedMusic].c_str())) {
            for (int i = 0; i < musicFiles.size(); ++i) {
                bool sel = (selectedMusic == i);
                if (ImGui::Selectable(musicFiles[i].c_str(), sel)) {
                    selectedMusic = i;
                    if (onMusicSelected) {
                        onMusicSelected(musicFiles[i]);
                    }
                }
                if (sel) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }


        // --- NAME AND SAVE ---
        ImGui::InputText("Scene Name", sceneNameBuf, IM_ARRAYSIZE(sceneNameBuf));
        if (ImGui::Button("Save Scene") && sceneNameBuf[0] != '\0') {
            Scene newScene = {
                sceneNameBuf,
                meshFiles[selectedMesh],
                musicFiles[selectedMusic],
                currentLightPos,
                currentAmbient
            };
            scenes.push_back(newScene);
            SaveToFile("scenes.json");
            sceneNameBuf[0] = '\0';
        }

        ImGui::Separator();
        ImGui::Text("Saved Scenes:");

        // --- SCENE LIST ---
        for (int i = 0; i < scenes.size(); ++i) {
            ImGui::PushID(i);

            float fullWidth = ImGui::GetContentRegionAvail().x;
            float buttonWidth = 110.0f; // width for each button approx
            float spacing = 5.0f;
            float buttonsTotalWidth = (buttonWidth + spacing) * 2; // Delete + Save buttons

            bool deleted = false;

            ImGui::BeginGroup();
            if (ImGui::Selectable(scenes[i].name.c_str(), selectedScene == i, 0, ImVec2(fullWidth - buttonsTotalWidth, 0))) {
                selectedScene = i;

                auto normalize = [](const std::string& path) -> std::string {
                    std::string result = path;
                    std::replace(result.begin(), result.end(), '\\', '/');
                    return result;
                    };

                std::string sceneMesh = normalize(scenes[i].meshPath);
                std::string sceneMusic = normalize(scenes[i].musicPath);

                for (int m = 0; m < meshFiles.size(); ++m) {
                    if (normalize(meshFiles[m]) == sceneMesh) {
                        selectedMesh = m;
                        break;
                    }
                }

                for (int m = 0; m < musicFiles.size(); ++m) {
                    if (normalize(musicFiles[m]) == sceneMusic) {
                        selectedMusic = m;
                        break;
                    }
                }

                if (onSceneLoaded)
                    onSceneLoaded(scenes[i]);
            }

            ImGui::SameLine();
            if (ImGui::Button("Save")) {
                scenes[i].meshPath = meshFiles[selectedMesh];
                scenes[i].musicPath = musicFiles[selectedMusic];
                scenes[i].lightPos = currentLightPos;
                scenes[i].ambientIntensity = currentAmbient;

                SaveToFile("scenes.json");
            }

            ImGui::SameLine();
            if (ImGui::Button("Delete")) {
                scenes.erase(scenes.begin() + i);
                SaveToFile("scenes.json");
                deleted = true;
            }

            ImGui::EndGroup();
            ImGui::PopID();

            if (deleted) break;
        }

        ImGui::End();
    }

    void SceneManager::SaveToFile(const std::string& path) const {
        json j;
        for (const auto& s : scenes) {
            j["scenes"].push_back({
                { "name", s.name },
                { "meshPath", s.meshPath },
                { "musicPath", s.musicPath },
                { "lightPos", { s.lightPos.x, s.lightPos.y, s.lightPos.z } },
                { "ambientIntensity", s.ambientIntensity }
                });
        }

        std::ofstream ofs(path);
        if (ofs.is_open()) {
            ofs << j.dump(4);
            ofs.close();
        }
        else {
            std::cerr << "Failed to open file for writing: " << path << "\n";
        }
    }


    void SceneManager::LoadFromFile(const std::string& path) {
        std::ifstream ifs(path);
        if (!ifs.is_open()) {
            scenes = {
                { "The Dark Knight", "assets/models/Batman.glb", "assets/musics/Somthing.ogg", glm::vec3(3,3,3), 0.5f },
                { "Man of Tomorrow", "assets/models/Superman.glb", "assets/musics/Punkrocker.ogg", glm::vec3(2,2,2), 0.6f },
                { "Heisenburg", "assets/models/Walter.glb", "assets/musics/BreakBad.ogg", glm::vec3(1,1,1), 0.4f }
            };
            SaveToFile(path);
            return;
        }

        json j;
        try {
            ifs >> j;
            scenes.clear();
            for (const auto& s : j["scenes"]) {
                glm::vec3 light(3.0f);
                if (s.contains("lightPos") && s["lightPos"].is_array() && s["lightPos"].size() == 3) {
                    light = glm::vec3(s["lightPos"][0], s["lightPos"][1], s["lightPos"][2]);
                }

                float ambient = s.value("ambientIntensity", 0.5f);

                scenes.push_back({
                    s.value("name", ""),
                    s.value("meshPath", ""),
                    s.value("musicPath", ""),
                    light,
                    ambient
                    });
            }
        }
        catch (...) {
            std::cerr << "Failed to parse scenes.json\n";
        }
    }
}

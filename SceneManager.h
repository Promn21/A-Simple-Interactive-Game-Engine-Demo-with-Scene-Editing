#pragma once
#include <string>
#include <vector>
#include <functional>
#include <glm/glm.hpp>


namespace SS
{
    struct Scene {
        std::string name;
        std::string meshPath;
        std::string musicPath;
        glm::vec3 lightPos = glm::vec3(3.0f, 3.0f, 3.0f);
        float ambientIntensity = 0.5f;
    };

    class SceneManager {
    public:
        std::vector<std::string> meshFiles;
        std::vector<std::string> musicFiles;
        std::vector<Scene> scenes;

        int selectedMesh = 0;
        int selectedMusic = 0;
        int selectedScene = -1;

        SceneManager();


        void renderImGui(std::function<void(const std::string&)> onMeshSelected,
            std::function<void(const std::string&)> onMusicSelected,
            std::function<void(const Scene&)> onSceneLoaded,
            glm::vec3& currentLightPos,
            float& currentAmbient);
        void LoadFromFile(const std::string& path);
        void SaveToFile(const std::string& path) const;

        char sceneNameBuf[128] = { 0 };

    };
}
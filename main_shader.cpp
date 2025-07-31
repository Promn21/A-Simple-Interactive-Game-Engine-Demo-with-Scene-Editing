#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image_write.h"
#include "stb_image.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "ModelManager.h"
#include "SceneManager.h"
#include "SoundManager.h"

#include <iostream>
#include <functional>


// Vertex Shader source code
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec3 Normal;
out vec2 TexCoord;
out vec3 FragPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

// Fragment Shader source code
const char* fragmentShaderSource = R"(
#version 330 core
in vec3 Normal;
in vec2 TexCoord;
in vec3 FragPos;

out vec4 FragColor;

uniform float ambientIntensity;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform bool hasBaseColor;
uniform sampler2D baseColorTexture;

void main() {
    vec3 norm = normalize(Normal);
    vec3 lightColor = vec3(1.0);
    vec3 ambient = ambientIntensity * lightColor;
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    vec3 result = ambient + diffuse;

    vec4 color = hasBaseColor ? texture(baseColorTexture, TexCoord) : vec4(result,1.0);
    FragColor = color * vec4(result, 1.0);
}
)";


// Helper function to compile a shader
unsigned int compileShader(unsigned int type, const char* source) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader compilation error:\n" << infoLog << "\n";
    }
    return shader;
}

// Helper function to create shader program from vertex and fragment shaders
unsigned int createShaderProgram() {
    unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    int success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "Shader program linking error:\n" << infoLog << "\n";
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

// Load scene's model and music
void loadScene(const SS::Scene& scene, SS::Model& model, SS::SoundManager& soundManager, std::string& currentMusic) {
    std::cout << "Loading Scene: " << scene.name << "\n";

    if (!model.LoadFromFile(scene.meshPath)) {
        std::cerr << "Failed to load model: " << scene.meshPath << "\n";
    }
    soundManager.stopMusic();
    soundManager.playMusic(scene.musicPath);
    currentMusic = scene.musicPath;
}

int main() {
    // 1. Initialize sound manager
    SS::SoundManager soundManager;
    if (soundManager.init() != 1) {
        std::cerr << "Failed to initialize audio engine\n";
        return -1;
    }

    // 2. Initialize GLFW and OpenGL context
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 800, "Scene Manager Demo", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW\n";
        return -1;
    }

    glViewport(0, 0, 1280, 800);
    glEnable(GL_DEPTH_TEST);

    // 3. Compile and link shader program
    unsigned int shaderProgram = createShaderProgram();

    // 4. Setup ImGui context and bindings
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // 5. Create SceneManager and Model instances
    SS::SceneManager sceneManager;
    SS::Model currentModel;
    std::string currentMusic;

    // 6. Camera and lighting initial setup
    glm::vec3 camPos(-0.6f, 1.0f, 3.0f);
    glm::vec3 camCenter(0.6f, 1.0f, 0.0f);
    glm::vec3 lightPos(3.0f, 4.0f, 3.0f);
    float camZoom = 37.0f;
    float ambientIntensity = 0.5f;

    // 7. Load the first scene if available
    if (!sceneManager.scenes.empty()) {
        SS::Scene& first = sceneManager.scenes[0];

        // Find selected indices for mesh and music
        for (int i = 0; i < sceneManager.meshFiles.size(); ++i) {
            if (sceneManager.meshFiles[i] == first.meshPath) {
                sceneManager.selectedMesh = i;
                break;
            }
        }
        for (int i = 0; i < sceneManager.musicFiles.size(); ++i) {
            if (sceneManager.musicFiles[i] == first.musicPath) {
                sceneManager.selectedMusic = i;
                break;
            }
        }

        // Load first scene model and music
        loadScene(first, currentModel, soundManager, currentMusic);
        lightPos = first.lightPos;
        ambientIntensity = first.ambientIntensity;
    }

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Clear buffers
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Render Scene UI, with callbacks for model, music, and scene changes
        sceneManager.renderImGui(
            [&](const std::string& meshPath) {
                currentModel.LoadFromFile(meshPath);
            },
            [&](const std::string& musicPath) {
                soundManager.stopMusic();
                soundManager.playMusic(musicPath);
                currentMusic = musicPath;
            },
            [&](const SS::Scene& scene) {
                loadScene(scene, currentModel, soundManager, currentMusic);
                lightPos = scene.lightPos;
                ambientIntensity = scene.ambientIntensity;
            },
            lightPos,
            ambientIntensity
        );

        // Music control UI
        ImGui::Begin("Music Control");
        if (ImGui::Button("Play Music")) {
            if (!sceneManager.musicFiles.empty()) {
                std::string pathToPlay = sceneManager.musicFiles[sceneManager.selectedMusic];
                soundManager.playMusic(pathToPlay);
                currentMusic = pathToPlay;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Stop Music")) {
            soundManager.stopMusic();
        }
        ImGui::End();

        // Camera & Light UI
        ImGui::Begin("Camera & Light");
        ImGui::SliderFloat3("Camera Position", &camPos[0], -10.0f, 10.0f);
        ImGui::SliderFloat3("Camera LookAt", &camCenter[0], -10.0f, 10.0f);
        ImGui::SliderFloat("Field of View", &camZoom, 10.0f, 90.0f);
        ImGui::SliderFloat("Ambient Intensity", &ambientIntensity, 0.0f, 1.0f);
        ImGui::SliderFloat3("Light Position", &lightPos[0], -10.0f, 10.0f);
        ImGui::End();

        // Use shader program and update uniforms
        glUseProgram(shaderProgram);
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        glm::mat4 viewMatrix = glm::lookAt(camPos, camCenter, glm::vec3(0, 1, 0));
        glm::mat4 projectionMatrix = glm::perspective(glm::radians(camZoom), 1280.0f / 800.0f, 0.1f, 100.0f);

        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(viewMatrix));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));

        glUniform1f(glGetUniformLocation(shaderProgram, "ambientIntensity"), ambientIntensity);
        glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"), 1, glm::value_ptr(lightPos));
        glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, glm::value_ptr(camPos));

        // Draw the current model
        currentModel.Draw(shaderProgram);

        // Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glDeleteProgram(shaderProgram);
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}


#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>

struct Camera {
    glm::vec3 position{0.0f, 1.0f, 12.0f};
    float yaw = -90.0f, pitch = -4.0f, fov = 45.0f;
    float sensitivity = 0.1f;
    bool limitPitch = true, invertY = false;
    glm::vec3 forward() const {
        float y = glm::radians(yaw), p = glm::radians(pitch);
        return glm::normalize(glm::vec3(std::cos(y)*std::cos(p), std::sin(p), std::sin(y)*std::cos(p)));
    }
    glm::vec3 right() const {
        float y = glm::radians(yaw);
        return glm::vec3(-std::sin(y), 0.0f, std::cos(y));
    }
};
Camera camera;
bool captured = true, firstMouse = true, alternateTheme = false;
double lastX = 0.0, lastY = 0.0;

void mouseMoved(GLFWwindow*, double x, double y) {
    if (!captured) return;
    if (firstMouse) {
        lastX = x;
        lastY = y;
        firstMouse = false;
    }
    float dx = static_cast<float>(x-lastX)*camera.sensitivity;
    float dy = static_cast<float>(lastY-y)*camera.sensitivity;
    lastX = x;
    lastY = y;
    camera.yaw = std::remainder(camera.yaw+dx, 360.0f);
    camera.pitch += camera.invertY ? -dy : dy;
    if (camera.limitPitch) camera.pitch = std::clamp(camera.pitch, -89.0f, 89.0f);
    else camera.pitch = std::remainder(camera.pitch, 360.0f);
}
void scrolled(GLFWwindow*, double, double y) {
    if (captured) camera.fov = std::clamp(camera.fov-static_cast<float>(y), 1.0f, 90.0f);
}
void focusChanged(GLFWwindow*, int) { firstMouse = true; }
void keyPressed(GLFWwindow* window, int key, int, int action, int) {
    if (action != GLFW_PRESS) return;
    switch (key) {
        case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(window, GLFW_TRUE); break;
        case GLFW_KEY_TAB:
            captured = !captured;
            firstMouse = true;
            glfwSetInputMode(window, GLFW_CURSOR, captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
            break;
        case GLFW_KEY_L:
            camera.limitPitch = !camera.limitPitch;
            if (camera.limitPitch) camera.pitch = std::clamp(camera.pitch, -89.0f, 89.0f);
            std::cout << "Pitch limit: " << camera.limitPitch << '\n';
            break;
        case GLFW_KEY_I:
            camera.invertY = !camera.invertY;
            std::cout << "Invert Y: " << camera.invertY << '\n';
            break;
        case GLFW_KEY_C: alternateTheme = !alternateTheme; break;
        case GLFW_KEY_R: camera = Camera{}; firstMouse = true; break;
        case GLFW_KEY_P: {
            glm::vec3 d = camera.forward();
            std::cout << std::fixed << std::setprecision(3)
                      << "Position: " << camera.position.x << ", " << camera.position.y << ", " << camera.position.z
                      << " | Direction: " << d.x << ", " << d.y << ", " << d.z
                      << " | Yaw: " << camera.yaw << " | Pitch: " << camera.pitch << " | FOV: " << camera.fov << '\n';
            break;
        }
        default: break;
    }
}
void moveCamera(GLFWwindow* window, float deltaTime) {
    if (!glfwGetWindowAttrib(window, GLFW_FOCUSED)) return;
    glm::vec3 movement(0.0f);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) movement += camera.forward();
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) movement -= camera.forward();
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) movement += camera.right();
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) movement -= camera.right();
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) movement.y += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) movement.y -= 1.0f;
    float speed = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ? 9.0f : 3.0f;
    if (glm::length(movement) > 0.0001f) camera.position += glm::normalize(movement)*speed*deltaTime;
}
GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[2048];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::cerr << "Shader compilation failed: " << log << '\n';
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}
GLuint createProgram() {
    const char* vs = R"(
#version 330 core
layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
uniform mat4 model, view, projection;
out vec3 worldPosition, worldNormal;
void main() {
    vec4 world = model * vec4(position, 1.0);
    worldPosition = world.xyz;
    worldNormal = mat3(transpose(inverse(model))) * normal;
    gl_Position = projection * view * world;
}
)";
    const char* fs = R"(
#version 330 core
in vec3 worldPosition, worldNormal;
uniform vec3 color, cameraPosition;
out vec4 fragmentColor;
void main() {
    vec3 n = normalize(worldNormal);
    vec3 light = normalize(vec3(-3.0,6.0,8.0)-worldPosition);
    vec3 eye = normalize(cameraPosition-worldPosition);
    float diffuse = max(dot(n,light),0.0);
    float specular = pow(max(dot(n,normalize(light+eye)),0.0),48.0);
    float rim = pow(1.0-max(dot(n,eye),0.0),3.0);
    fragmentColor = vec4(color*(0.28+0.72*diffuse+0.18*rim)+vec3(0.35)*specular,1.0);
}
)";
    GLuint vertex = compileShader(GL_VERTEX_SHADER, vs);
    GLuint fragment = compileShader(GL_FRAGMENT_SHADER, fs);
    if (!vertex || !fragment) {
        if (vertex) glDeleteShader(vertex);
        if (fragment) glDeleteShader(fragment);
        return 0;
    }
    GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[2048];
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        std::cerr << "Program linking failed: " << log << '\n';
        glDeleteProgram(program);
        return 0;
    }
    return program;
}
int main() {
    glfwSetErrorCallback([](int code, const char* text) { std::cerr << "GLFW " << code << ": " << text << '\n'; });
    if (!glfwInit()) return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif
    glfwWindowHint(GLFW_SAMPLES, 4);
    GLFWwindow* window = glfwCreateWindow(1280, 800, "Week 10 | Chromatic Orbit | C: theme | Tab: cursor", nullptr, nullptr);
    if (!window) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);
    if (!gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress))) {
        std::cerr << "Could not load OpenGL\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    glfwSwapInterval(1);
    glfwSetCursorPosCallback(window, mouseMoved);
    glfwSetScrollCallback(window, scrolled);
    glfwSetKeyCallback(window, keyPressed);
    glfwSetWindowFocusCallback(window, focusChanged);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    GLuint program = createProgram();
    if (!program) { glfwDestroyWindow(window); glfwTerminate(); return 1; }
    const float vertices[] = {
        -0.5f,-0.5f,0.5f,0,0,1, 0.5f,-0.5f,0.5f,0,0,1, 0.5f,0.5f,0.5f,0,0,1, -0.5f,0.5f,0.5f,0,0,1,
        0.5f,-0.5f,-0.5f,0,0,-1, -0.5f,-0.5f,-0.5f,0,0,-1, -0.5f,0.5f,-0.5f,0,0,-1, 0.5f,0.5f,-0.5f,0,0,-1,
        -0.5f,-0.5f,-0.5f,-1,0,0, -0.5f,-0.5f,0.5f,-1,0,0, -0.5f,0.5f,0.5f,-1,0,0, -0.5f,0.5f,-0.5f,-1,0,0,
        0.5f,-0.5f,0.5f,1,0,0, 0.5f,-0.5f,-0.5f,1,0,0, 0.5f,0.5f,-0.5f,1,0,0, 0.5f,0.5f,0.5f,1,0,0,
        -0.5f,0.5f,0.5f,0,1,0, 0.5f,0.5f,0.5f,0,1,0, 0.5f,0.5f,-0.5f,0,1,0, -0.5f,0.5f,-0.5f,0,1,0,
        -0.5f,-0.5f,-0.5f,0,-1,0, 0.5f,-0.5f,-0.5f,0,-1,0, 0.5f,-0.5f,0.5f,0,-1,0, -0.5f,-0.5f,0.5f,0,-1,0
    };
    const unsigned int indices[] = {
        0,1,2,2,3,0, 4,5,6,6,7,4, 8,9,10,10,11,8,
        12,13,14,14,15,12, 16,17,18,18,19,16, 20,21,22,22,23,20
    };
    GLuint vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,6*sizeof(float),nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,6*sizeof(float),reinterpret_cast<void*>(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    const glm::vec3 positions[] = {
        {0,0,0}, {-3.1f,1.6f,-1.2f}, {2.9f,1.8f,-1.8f}, {-2.9f,-1.6f,0.4f},
        {2.8f,-1.5f,0.2f}, {-0.9f,3.0f,-3.5f}, {1.0f,-2.7f,-2.7f}
    };
    const glm::vec3 palettes[2][7] = {
        {{0.12f,0.78f,1}, {0.63f,0.28f,0.95f}, {0.18f,0.36f,0.95f}, {1,0.43f,0.13f},
         {0.53f,0.24f,0.92f}, {0.20f,0.65f,1}, {1,0.58f,0.20f}},
        {{0.72f,0.38f,1}, {0.20f,0.78f,1}, {1,0.56f,0.22f}, {0.22f,0.39f,1},
         {1,0.40f,0.17f}, {0.68f,0.30f,0.95f}, {0.16f,0.72f,0.95f}}
    };
    GLint modelLoc = glGetUniformLocation(program,"model");
    GLint viewLoc = glGetUniformLocation(program,"view");
    GLint projectionLoc = glGetUniformLocation(program,"projection");
    GLint colorLoc = glGetUniformLocation(program,"color");
    GLint cameraLoc = glGetUniformLocation(program,"cameraPosition");
    std::cout << "OpenGL: " << glGetString(GL_VERSION) << '\n'
              << "WASD: move | Space/Left Shift: up/down | Left Ctrl: boost\n"
              << "Mouse: look (0.1) | Wheel: FOV 1..90 | L: pitch limit | I: invert Y\n"
              << "P: camera info | R: reset | C: theme | Tab: cursor | Esc: exit\n" << std::flush;
    double previousTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        double now = glfwGetTime();
        float deltaTime = static_cast<float>(now-previousTime);
        previousTime = now;
        glfwPollEvents();
        moveCamera(window, deltaTime);
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        if (width == 0 || height == 0) {
            glfwWaitEventsTimeout(0.05);
            previousTime = glfwGetTime();
            continue;
        }
        glViewport(0,0,width,height);
        if (alternateTheme) glClearColor(0.018f,0.012f,0.045f,1);
        else glClearColor(0.008f,0.018f,0.042f,1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glm::vec3 direction = camera.forward();
        glm::vec3 up = glm::normalize(glm::cross(camera.right(),direction));
        glm::mat4 view = glm::lookAt(camera.position,camera.position+direction,up);
        glm::mat4 projection = glm::perspective(glm::radians(camera.fov),static_cast<float>(width)/height,0.1f,150.0f);
        glUseProgram(program);
        glUniformMatrix4fv(viewLoc,1,GL_FALSE,glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc,1,GL_FALSE,glm::value_ptr(projection));
        glUniform3fv(cameraLoc,1,glm::value_ptr(camera.position));
        glBindVertexArray(vao);
        float time = static_cast<float>(now);
        for (int i=0; i<7; ++i) {
            glm::vec3 position = positions[i];
            position.y += 0.16f*std::sin(time*0.8f+i*1.1f);
            glm::mat4 model = glm::translate(glm::mat4(1),position);
            model = glm::rotate(model,time*(0.22f+i*0.025f)+i*0.45f,glm::normalize(glm::vec3(0.4f,1,0.25f)));
            model = glm::scale(model,glm::vec3(i == 0 ? 1.9f : 0.95f+0.10f*(i%3)));
            glUniformMatrix4fv(modelLoc,1,GL_FALSE,glm::value_ptr(model));
            glUniform3fv(colorLoc,1,glm::value_ptr(palettes[alternateTheme ? 1 : 0][i]));
            glDrawElements(GL_TRIANGLES,36,GL_UNSIGNED_INT,nullptr);
        }
        glfwSwapBuffers(window);
    }
    glDeleteBuffers(1,&ebo);
    glDeleteBuffers(1,&vbo);
    glDeleteVertexArrays(1,&vao);
    glDeleteProgram(program);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
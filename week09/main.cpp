
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <cmath>

const unsigned int WIDTH = 1000;
const unsigned int HEIGHT = 700;

glm::vec3 cameraPos(0.0f, 0.0f, 8.0f);
glm::vec3 cameraFront(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp(0.0f, 1.0f, 0.0f);

float yaw = -90.0f;
float pitch = 0.0f;
float lastX = WIDTH / 2.0f;
float lastY = HEIGHT / 2.0f;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

void processInput(GLFWwindow* window);
void mouseCallback(GLFWwindow* window, double x, double y);

const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

uniform vec3 objectColor;

void main() {
    FragColor = vec4(objectColor, 1.0);
}
)";

int main() {
    if (!glfwInit()) {
        std::cout << "GLFW initialization failed\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(
        WIDTH, HEIGHT, "Week 09 - Colorful 3D Cubes",
        nullptr, nullptr
    );

    if (!window) {
        std::cout << "Window creation failed\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    glfwSetFramebufferSizeCallback(window,
        [](GLFWwindow*, int w, int h) {
            glViewport(0, 0, w, h);
        }
    );

    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)) {
        std::cout << "GLAD initialization failed\n";
        glfwTerminate();
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    float vertices[] = {
        -0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,
         0.5f, 0.5f,-0.5f,  0.5f, 0.5f,-0.5f,
        -0.5f, 0.5f,-0.5f, -0.5f,-0.5f,-0.5f,

        -0.5f,-0.5f, 0.5f,  0.5f,-0.5f, 0.5f,
         0.5f, 0.5f, 0.5f,  0.5f, 0.5f, 0.5f,
        -0.5f, 0.5f, 0.5f, -0.5f,-0.5f, 0.5f,

        -0.5f, 0.5f, 0.5f, -0.5f, 0.5f,-0.5f,
        -0.5f,-0.5f,-0.5f, -0.5f,-0.5f,-0.5f,
        -0.5f,-0.5f, 0.5f, -0.5f, 0.5f, 0.5f,

         0.5f, 0.5f, 0.5f,  0.5f, 0.5f,-0.5f,
         0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,
         0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f,

        -0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,
         0.5f,-0.5f, 0.5f,  0.5f,-0.5f, 0.5f,
        -0.5f,-0.5f, 0.5f, -0.5f,-0.5f,-0.5f,

        -0.5f, 0.5f,-0.5f,  0.5f, 0.5f,-0.5f,
         0.5f, 0.5f, 0.5f,  0.5f, 0.5f, 0.5f,
        -0.5f, 0.5f, 0.5f, -0.5f, 0.5f,-0.5f
    };

    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices),
                 vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE,
        3 * sizeof(float), (void*)0
    );
    glEnableVertexAttribArray(0);

    glm::vec3 positions[] = {
        {-2.5f,  1.0f,  0.0f},
        { 0.0f,  0.0f,  0.0f},
        { 2.5f,  1.0f, -1.0f},
        {-1.5f, -2.0f, -2.0f},
        { 1.8f, -2.0f,  0.0f}
    };

    glm::vec3 colors[] = {
        {1.0f, 0.3f, 0.4f},
        {0.2f, 0.8f, 1.0f},
        {0.4f, 1.0f, 0.5f},
        {1.0f, 0.8f, 0.2f},
        {0.7f, 0.4f, 1.0f}
    };

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.025f, 0.035f, 0.08f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        float aspect = fbHeight > 0
            ? (float)fbWidth / fbHeight
            : (float)WIDTH / HEIGHT;

        glm::mat4 projection = glm::perspective(
            glm::radians(45.0f), aspect, 0.1f, 100.0f
        );

        glm::mat4 view = glm::lookAt(
            cameraPos, cameraPos + cameraFront, cameraUp
        );

        glUniformMatrix4fv(
            glGetUniformLocation(shaderProgram, "projection"),
            1, GL_FALSE, glm::value_ptr(projection)
        );

        glUniformMatrix4fv(
            glGetUniformLocation(shaderProgram, "view"),
            1, GL_FALSE, glm::value_ptr(view)
        );

        glBindVertexArray(VAO);

        for (int i = 0; i < 5; i++) {
            glm::mat4 model(1.0f);
            model = glm::translate(model, positions[i]);

            float angle = currentFrame * (20.0f + i * 12.0f);

            model = glm::rotate(
                model, glm::radians(angle),
                glm::vec3(0.5f, 1.0f, 0.3f)
            );

            glUniformMatrix4fv(
                glGetUniformLocation(shaderProgram, "model"),
                1, GL_FALSE, glm::value_ptr(model)
            );

            glUniform3fv(
                glGetUniformLocation(shaderProgram, "objectColor"),
                1, glm::value_ptr(colors[i])
            );

            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float speed = 2.5f * deltaTime;

    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        speed *= 2.0f;

    glm::vec3 right = glm::normalize(
        glm::cross(cameraFront, cameraUp)
    );

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += speed * cameraFront;

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= speed * cameraFront;

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= speed * right;

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += speed * right;
}

void mouseCallback(GLFWwindow*, double xpos, double ypos) {
    if (firstMouse) {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }

    float xoffset = (float)xpos - lastX;
    float yoffset = lastY - (float)ypos;

    lastX = (float)xpos;
    lastY = (float)ypos;

    float sensitivity = 0.1f;
    yaw += xoffset * sensitivity;
    pitch += yoffset * sensitivity;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) *
              cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) *
              cos(glm::radians(pitch));

    cameraFront = glm::normalize(front);
}

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <iostream>
#include <vector>

const int WIDTH = 1280;
const int HEIGHT = 720;

const float PI = 3.14159265359f;
const float SIZE = 0.12f;
const float GROUND_Y = -0.68f;
const float JUMP_HEIGHT = 4.0f * SIZE;
const float JUMP_PERIOD = 1.2f;
const float TRAVEL_DISTANCE = 1.9f;

bool paused = false;
bool spaceWasPressed = false;

// Пернетақта арқылы ойынды басқару
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    bool pressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;

    if (pressed && !spaceWasPressed)
        paused = !paused;

    spaceWasPressed = pressed;
}

// Геометрияны сақтау және экранға шығару
struct Shape
{
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLsizei indexCount = 0;

    void create(const std::vector<float>& vertices,
                const std::vector<unsigned int>& indices)
    {
        indexCount = static_cast<GLsizei>(indices.size());

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER,
                     vertices.size() * sizeof(float),
                     vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     indices.size() * sizeof(unsigned int),
                     indices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                              2 * sizeof(float), nullptr);
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);
    }

    void draw() const
    {
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, indexCount,
                       GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
    }

    void destroy()
    {
        glDeleteBuffers(1, &ebo);
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
    }
};

// Шейдерді компиляциялау
GLuint compileShader(GLenum type, const char* source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::cerr << "Shader error: " << log << '\n';
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

// Екі шейдерден OpenGL бағдарламасын құру
GLuint createProgram()
{
    const char* vertexSource = R"(
        #version 330 core
        layout(location = 0) in vec2 aPosition;
        uniform mat4 uTransform;

        void main()
        {
            gl_Position = uTransform * vec4(aPosition, 0.0, 1.0);
        }
    )";

    const char* fragmentSource = R"(
        #version 330 core
        uniform vec3 uColor;
        out vec4 FragColor;

        void main()
        {
            FragColor = vec4(uColor, 1.0);
        }
    )";

    GLuint vs = compileShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentSource);

    if (!vs || !fs)
    {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint success = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (!success)
    {
        char log[1024];
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        std::cerr << "Link error: " << log << '\n';
        glDeleteProgram(program);
        return 0;
    }

    return program;
}

// Объектіні берілген трансформациямен салу
void drawShape(GLuint program, const Shape& shape,
               const glm::mat4& model, const glm::vec3& color)
{
    glUseProgram(program);

    glUniformMatrix4fv(
        glGetUniformLocation(program, "uTransform"),
        1, GL_FALSE, glm::value_ptr(model));

    glUniform3fv(
        glGetUniformLocation(program, "uColor"),
        1, glm::value_ptr(color));

    shape.draw();
}

int main()
{
    if (!glfwInit())
    {
        std::cerr << "GLFW initialization failed\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(
        WIDTH, HEIGHT, "Week 06 - Diamond and Triangle",
        nullptr, nullptr);

    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGL(glfwGetProcAddress))
    {
        std::cerr << "GLAD initialization failed\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    glViewport(0, 0, WIDTH, HEIGHT);

    GLuint program = createProgram();
    if (!program)
    {
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    // Ромб төрт төбеден құралады
    Shape diamond;
    diamond.create(
        {0.0f, 1.0f, 1.0f, 0.0f,
         0.0f, -1.0f, -1.0f, 0.0f},
        {0, 1, 2, 0, 2, 3});

    // Үшбұрыш үш төбеден құралады
    Shape triangle;
    triangle.create(
        {-1.0f, -1.0f, 1.0f, -1.0f, 0.0f, 1.0f},
        {0, 1, 2});

    // Жердің геометриясы
    Shape ground;
    ground.create(
        {-1.0f, -1.0f, 1.0f, -1.0f,
          1.0f, 1.0f, -1.0f, 1.0f},
        {0, 1, 2, 0, 2, 3});

    float previousTime = static_cast<float>(glfwGetTime());
    float elapsed = 0.0f;
    float angle = 0.0f;

    while (!glfwWindowShouldClose(window))
    {
        float now = static_cast<float>(glfwGetTime());
        float dt = now - previousTime;
        previousTime = now;

        if (dt > 0.05f)
            dt = 0.05f;

        processInput(window);

        // Анимация уақыты пауза кезінде тоқтайды
        if (!paused)
        {
            elapsed += dt;
            angle += 2.0f * dt;
        }

        float phase = std::fmod(elapsed, JUMP_PERIOD) / JUMP_PERIOD;

        // Ромбтың секіру биіктігі
        float jump = JUMP_HEIGHT * std::sin(PI * phase);
        float diamondY = GROUND_Y + SIZE + jump;

        // Үшбұрыштың көлденең қозғалысы
        float triangleX =
            -TRAVEL_DISTANCE + 2.0f * TRAVEL_DISTANCE * phase;

        glClearColor(0.07f, 0.08f, 0.13f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Жерді салу
        glm::mat4 groundModel(1.0f);
        groundModel = glm::translate(
            groundModel, glm::vec3(0.0f, GROUND_Y - 0.015f, 0.0f));
        groundModel = glm::scale(
            groundModel, glm::vec3(1.0f, 0.015f, 1.0f));

        drawShape(program, ground, groundModel,
                  glm::vec3(0.38f, 0.42f, 0.50f));

        // Ромбтың орны мен өлшемі
        glm::mat4 diamondModel(1.0f);
        diamondModel = glm::translate(
            diamondModel, glm::vec3(0.0f, diamondY, 0.0f));
        diamondModel = glm::rotate(
            diamondModel, 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
        diamondModel = glm::scale(
            diamondModel, glm::vec3(SIZE, SIZE, 1.0f));

        drawShape(program, diamond, diamondModel,
                  glm::vec3(0.15f, 0.85f, 1.0f));

        // Үшбұрыш жылжиды және өз ортасында айналады
        glm::mat4 triangleModel(1.0f);
        triangleModel = glm::translate(
            triangleModel,
            glm::vec3(triangleX, GROUND_Y + SIZE, 0.0f));
        triangleModel = glm::rotate(
            triangleModel, angle, glm::vec3(0.0f, 0.0f, 1.0f));
        triangleModel = glm::scale(
            triangleModel, glm::vec3(SIZE, SIZE, 1.0f));

        drawShape(program, triangle, triangleModel,
                  glm::vec3(1.0f, 0.38f, 0.24f));

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    diamond.destroy();
    triangle.destroy();
    ground.destroy();
    glDeleteProgram(program);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

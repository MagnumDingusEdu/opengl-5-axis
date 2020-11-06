#include "headers/game.h"

bool PROJECTION_MODE = true; // ortho = 1, perspective = 0
bool DISABLE_GL_READ = false;

std::vector<Vertex> generateTriangles();

std::vector<Vertex> generateTorus();

//Private functions
void Game::initGLFW() {
    //INIT GLFW
    if (glfwInit() == GLFW_FALSE) {
        std::cout << "ERROR::GLFW_INIT_FAILED"
                  << "\n";
        glfwTerminate();
    }
}

void Game::initWindow(
        const char *title,
        bool resizable) {
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, this->GL_VERSION_MAJOR);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, this->GL_VERSION_MINOR);
    glfwWindowHint(GLFW_RESIZABLE, resizable);

    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); MAC OS

    this->window = glfwCreateWindow(this->WINDOW_WIDTH, this->WINDOW_HEIGHT, title, nullptr, nullptr);

    if (this->window == nullptr) {
        std::cout << "ERROR::GLFW_WINDOW_INIT_FAILED"
                  << "\n";
        glfwTerminate();
    }

    glfwGetFramebufferSize(this->window, &this->framebufferWidth, &this->framebufferHeight);
    glfwSetFramebufferSizeCallback(window, Game::framebuffer_resize_callback);
    //IMPORTANT WHITH PERSPECTIVE MATRIX!!!

    //glViewport(0, 0, framebufferWidth, framebufferHeight);

    glfwMakeContextCurrent(this->window); //IMPORTANT!!
}

void Game::initGLEW() {
    //INIT GLEW (NEEDS WINDOW AND OPENGL CONTEXT)
    glewExperimental = GL_TRUE;

    //Error
    if (glewInit() != GLEW_OK) {
        std::cout << "ERROR::MAIN.CPP::GLEW_INIT_FAILED"
                  << "\n";
        glfwTerminate();
    }
}

void Game::initOpenGLOptions() {
    glEnable(GL_DEPTH_TEST);
    // glDepthMask(GL_FALSE);

    // glEnable(GL_CULL_FACE);
    // glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    //Input
    // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetKeyCallback(window, Game::changeRenderMode);
}

void Game::initMatrices() {
    this->ViewMatrix = glm::mat4(1.f);
    this->ViewMatrix = glm::lookAt(this->camPosition, this->camPosition + this->camFront, this->worldUp);

    this->ProjectionMatrix = glm::mat4(1.f);
    if (PROJECTION_MODE) {
        this->ProjectionMatrix = glm::ortho(static_cast<float>(mat_left),
                                            static_cast<float>(mat_right),
                                            static_cast<float>(mat_bottom),
                                            static_cast<float>(mat_top),
                                            -100.0f, 100.0f);
    } else {
        this->ProjectionMatrix = glm::perspective(
                glm::radians(this->fov),
                static_cast<float>(this->framebufferWidth) / static_cast<float>(this->framebufferHeight),
                this->nearPlane,
                this->farPlane);
        std::cout << glm::to_string(this->ProjectionMatrix);
    }
}

void Game::initShaders() {
    this->shaders.push_back(new Shader(this->GL_VERSION_MAJOR, this->GL_VERSION_MINOR,
                                       (char *) "shaders/vertex_core.glsl", (char *) "shaders/fragment_core.glsl"));
}

void Game::initMaterials() {
    this->materials.push_back(new Material(glm::vec3(0.4f), glm::vec3(1.f), glm::vec3(1.f),
                                           0, 1));
}

void Game::initModels() {

    std::vector<Mesh *> torusMesh;
    std::vector<Vertex> torus = generateTorus();

    std::vector<Mesh *> bezierMesh;
    std::vector<Vertex> bezier = generateTriangles();

    torusMesh.push_back(
            new Mesh(
                    torus.data(),
                    torus.size(),
                    nullptr,
                    0,
                    glm::vec3(0.f),
                    glm::vec3(0.f),
                    glm::vec3(0.f),
                    glm::vec3(1.f)));

    bezierMesh.push_back(
            new Mesh(
                    bezier.data(),
                    bezier.size(),
                    nullptr,
                    0,
                    glm::vec3(0.f),
                    glm::vec3(0.f, 0.f, 0.f),
                    glm::vec3(0.f),
                    glm::vec3(1.f)));

    this->torusModel = new Model(
            glm::vec3(0.f, 0.f, -40.f),
            this->materials[0],
            torusMesh);

    this->bezierModel = new Model(
            glm::vec3(-5.f, -5.f, -80.f),
            this->materials[0],
            bezierMesh);

    for (auto *&i : torusMesh)
        delete i;

    for (auto *&i : bezierMesh)
        delete i;

    this->models.push_back(torusModel);

}

void Game::initLights() {
    this->lights.push_back(new glm::vec3(0.f, 0.f, 1.f));
}

void Game::initUniforms() {
    //INIT UNIFORMS
    this->shaders[SHADER_CORE_PROGRAM]->setMat4fv(ViewMatrix, "ViewMatrix");
    this->shaders[SHADER_CORE_PROGRAM]->setMat4fv(ProjectionMatrix, "ProjectionMatrix");

    this->shaders[SHADER_CORE_PROGRAM]->setVec3f(*this->lights[0], "lightPos0");
}

void Game::updateUniforms() {
    //Update view matrix (camera)
    this->ViewMatrix = this->camera.getViewMatrix();

    this->shaders[SHADER_CORE_PROGRAM]->setMat4fv(this->ViewMatrix, "ViewMatrix");
    this->shaders[SHADER_CORE_PROGRAM]->setVec3f(this->camera.getPosition(), "cameraPos");
    this->shaders[SHADER_CORE_PROGRAM]->setVec3f(*this->lights[0], "lightPos0");

    //Update framebuffer size and projection matrix
    glfwGetFramebufferSize(this->window, &this->framebufferWidth, &this->framebufferHeight);

    if (PROJECTION_MODE) {
        this->ProjectionMatrix = glm::ortho(static_cast<float>(mat_left),
                                            static_cast<float>(mat_right),
                                            static_cast<float>(mat_bottom),
                                            static_cast<float>(mat_top),
                                            -100.0f, 100.0f);
    } else {
        this->ProjectionMatrix = glm::perspective(
                glm::radians(this->fov),
                static_cast<float>(this->framebufferWidth) / static_cast<float>(this->framebufferHeight),
                this->nearPlane,
                this->farPlane);
    }

    this->shaders[SHADER_CORE_PROGRAM]->setMat4fv(this->ProjectionMatrix, "ProjectionMatrix");
}

//Constructors / Destructors
Game::Game(
        const char *title,
        const int WINDOW_WIDTH, const int WINDOW_HEIGHT,
        const int GL_VERSION_MAJOR, const int GL_VERSION_MINOR,
        bool resizable)
        : WINDOW_WIDTH(WINDOW_WIDTH),
          WINDOW_HEIGHT(WINDOW_HEIGHT),
          GL_VERSION_MAJOR(GL_VERSION_MAJOR),
          GL_VERSION_MINOR(GL_VERSION_MINOR),
          camera(glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 0.f, 1.f), glm::vec3(0.f, 1.f, 0.f)) {
    //Init variables
    this->window = nullptr;
    this->framebufferWidth = this->WINDOW_WIDTH;
    this->framebufferHeight = this->WINDOW_HEIGHT;

    this->camPosition = glm::vec3(0.f, 0.f, 1.f);
    this->worldUp = glm::vec3(0.f, 1.f, 0.f);
    this->camFront = glm::vec3(0.f, 0.f, -1.f);

    this->fov = 120.f;
    this->nearPlane = 0.001f;
    this->farPlane = 100.f;

    this->dt = 0.f;
    this->curTime = 0.f;
    this->lastTime = 0.f;

    this->lastMouseX = 0.0;
    this->lastMouseY = 0.0;
    this->mouseX = 0.0;
    this->mouseY = 0.0;
    this->mouseOffsetX = 0.0;
    this->mouseOffsetY = 0.0;
    this->firstMouse = true;

    this->iv1 = 2 * (this->nearPlane) * (this->farPlane);
    this->iv2 = this->nearPlane + this->farPlane;
    this->iv3 = this->farPlane - this->nearPlane;

    this->depthPixels = (GLfloat *) malloc(4 * WINDOW_WIDTH * WINDOW_HEIGHT);
    this->closestPixel = 0;

    this->surfaceArea = 0;

    this->mat_left = -13.5;
    this->mat_right = 13.5;
    this->mat_bottom = -13.5;
    this->mat_top = 13.5;

    this->currently_visible = TORUS;

    this->initGLFW();
    this->initWindow(title, resizable);
    this->initGLEW();
    this->initOpenGLOptions();

    this->initMatrices();
    this->initShaders();
    this->initMaterials();
    this->initModels();
    this->initLights();
    this->initUniforms();
}

Game::~Game() {
    glfwDestroyWindow(this->window);
    glfwTerminate();

    for (auto &shader : this->shaders)
        delete shader;

    for (auto &material : this->materials)
        delete material;

    for (auto *&i : this->models)
        delete i;

    for (auto &light : this->lights)
        delete light;
}

//Accessor
int Game::getWindowShouldClose() {
    return glfwWindowShouldClose(this->window);
}

//Functions
void Game::updateDt() {
    this->curTime = static_cast<float>(glfwGetTime());
    this->dt = this->curTime - this->lastTime;

    this->lastTime = this->curTime;
}

[[maybe_unused]] void Game::updateMouseInput() {
    glfwGetCursorPos(this->window, &this->mouseX, &this->mouseY);

    if (this->firstMouse) {
        this->lastMouseX = this->mouseX;
        this->lastMouseY = this->mouseY;
        this->firstMouse = false;
    }

    //Calc offset
    this->mouseOffsetX = this->mouseX - this->lastMouseX;
    this->mouseOffsetY = this->lastMouseY - this->mouseY;

    //Set last X and Y
    this->lastMouseX = this->mouseX;
    this->lastMouseY = this->mouseY;

    if (glfwGetMouseButton(this->window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS) {
        *this->lights[0] = this->camera.getPosition();
    }
    if (glfwGetMouseButton(this->window, GLFW_MOUSE_BUTTON_2) == GLFW_PRESS) {
        this->saveDepthMap();
    }
}

void Game::updateKeyboardInput() {
    //Program
    if (glfwGetKey(this->window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(this->window, GLFW_TRUE);
    }

    //Camera
    if (glfwGetKey(this->window, GLFW_KEY_W) == GLFW_PRESS) {
        this->models[0]->move(glm::vec3(0.f, 0.05f, 0.f));
    }
    if (glfwGetKey(this->window, GLFW_KEY_S) == GLFW_PRESS) {
        this->models[0]->move(glm::vec3(0.f, -0.05f, 0.f));
    }
    if (glfwGetKey(this->window, GLFW_KEY_A) == GLFW_PRESS) {
        this->models[1]->rotate(glm::vec3(1.f, 0.f, 0.f));
    }
    if (glfwGetKey(this->window, GLFW_KEY_D) == GLFW_PRESS) {
        this->models[1]->rotate(glm::vec3(-1.f, 0.f, 0.f));
    }
    if (glfwGetKey(this->window, GLFW_KEY_Q) == GLFW_PRESS) {
        this->models[0]->move(glm::vec3(0.f, 0.f, 0.5f));
    }
    if (glfwGetKey(this->window, GLFW_KEY_E) == GLFW_PRESS) {
        this->models[0]->move(glm::vec3(0.f, 0.f, -0.5f));
    }
    if (glfwGetKey(this->window, GLFW_KEY_C) == GLFW_PRESS) {
        this->models[0]->scaleUp(glm::vec3(0.1f));
    }
    if (glfwGetKey(this->window, GLFW_KEY_V) == GLFW_PRESS) {
        this->models[0]->scaleUp(glm::vec3(-0.1f));
    }
}

void Game::updateInput() {
    glfwPollEvents();

    this->updateKeyboardInput();
    // this->updateMouseInput();
    this->camera.updateInput(dt, -1, this->mouseOffsetX, this->mouseOffsetY);
}

void Game::update() {
    //UPDATE INPUT ---
    this->updateDt();
    this->updateInput();


    // this->models[0]->rotate(glm::vec3(0.f, 1.f, 0.f));
    // this->models[1]->rotate(glm::vec3(0.f, 1.f, 0.f));
    // this->models[2]->rotate(glm::vec3(0.f, 1.f, 0.f));

    // std::cout << "FPS: " << 1/this->dt << std::endl;
}

void Game::render() {

    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    //Update the uniforms
    this->updateUniforms();

    //Render models
    for (auto &i : this->models)
        i->render(this->shaders[SHADER_CORE_PROGRAM]);
    glfwSwapBuffers(window);

    glFlush();

    glBindVertexArray(0);
    glUseProgram(0);
    glActiveTexture(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Game::saveDepthMap() {
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    this->updateUniforms();

    //Render models
    for (auto &i : this->models)
        i->render(this->shaders[SHADER_CORE_PROGRAM]);
    glfwSwapBuffers(window);

    if (!DISABLE_GL_READ)
        glReadPixels(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, GL_DEPTH_COMPONENT, GL_FLOAT, this->depthPixels);
    else
        for (int i = 0; i < WINDOW_WIDTH * WINDOW_HEIGHT; i++)
            this->depthPixels[i] = 0.5;

    for (size_t i = 0; i < this->WINDOW_WIDTH * this->WINDOW_HEIGHT; i++)
        depthPixels[i] = 200 * (depthPixels[i]) - 100;


    int initial_bottom = WINDOW_WIDTH / 2;
    int initial_left = (WINDOW_WIDTH) * (WINDOW_HEIGHT / 2);
    int initial_right = ((WINDOW_WIDTH) * ((WINDOW_HEIGHT / 2) + 1)) - 1;
    int initial_top = (WINDOW_WIDTH) * (WINDOW_HEIGHT) - WINDOW_WIDTH / 2;

    int top, bottom, left, right;

//    std::cout << initial_bottom << " " << initial_left << " " << initial_right << " " << initial_top << std::endl;

//    std::cout << "Depthmap : [" << (float) this->depthPixels[initial_bottom] << ", "
//              << (float) this->depthPixels[initial_left] << ", " << (float) this->depthPixels[initial_right] << ", "
//              << (float) this->depthPixels[initial_top] << "]" << std::endl;
    if (!DISABLE_GL_READ) {
//  Calculate Bottom Distance
        int current_pixel = initial_bottom;

        while (depthPixels[current_pixel] == 100)
            current_pixel += WINDOW_WIDTH;

        bottom = current_pixel / WINDOW_WIDTH;

//  Calculate Top Distance
        current_pixel = initial_top;
        while (depthPixels[current_pixel] == 100)
            current_pixel -= WINDOW_WIDTH;

        top = WINDOW_HEIGHT - current_pixel / WINDOW_WIDTH - 1;

//  Calculate Left Distance
        current_pixel = initial_left;

        while (depthPixels[current_pixel] == 100)
            current_pixel++;

        left = (current_pixel % WINDOW_WIDTH);

//  Calculate Right Distance
        current_pixel = initial_right;
        while (depthPixels[current_pixel] == 100)
            current_pixel--;

        right = WINDOW_WIDTH - (current_pixel % WINDOW_WIDTH) - 1;
    } else {
        left = 24;
        right = 24;
        bottom = 24;
        top = 24;
    }


    float average = (float) (top + bottom + left + right) / 4;
    float pixel_radius_outer = (float(WINDOW_WIDTH) / 2) - average;
    float actual_radius_outer = 6.7f;
    scaleFactor = pixel_radius_outer / actual_radius_outer;
    //End Draw
    std::cout << "Coordinate mapping complete. Scale factor : " << scaleFactor << std::endl;
    glFlush();

    glBindVertexArray(0);
    glUseProgram(0);
    glActiveTexture(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

[[maybe_unused]] void Game::rotateBezier() {
    this->models[1]->rotate(glm::vec3(0.f, -45.f, 0.f));
    this->update();
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    this->updateUniforms();

    //Render models
    for (auto &i : this->models)
        i->render(this->shaders[SHADER_CORE_PROGRAM]);
    glfwSwapBuffers(window);

    glReadPixels(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, GL_DEPTH_COMPONENT, GL_FLOAT, this->depthPixels);

    for (size_t i = 0; i < this->WINDOW_WIDTH * this->WINDOW_HEIGHT; i++) {
        this->depthPixels[i] = 200 * (this->depthPixels[i]) - 100;
    }



    // std::cout << "Depthmap : [" << (float)this->depthPixels[0] << ", " << (float)this->depthPixels[WINDOW_WIDTH - 1] << ", " << (float)this->depthPixels[WINDOW_WIDTH * (WINDOW_HEIGHT)-1] << ", " << (float)this->depthPixels[WINDOW_WIDTH * (WINDOW_HEIGHT - 1)] << "]" << std::endl;

    //End Draw
    glFlush();

    glBindVertexArray(0);
    glUseProgram(0);
    glActiveTexture(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Game::calculateNearestPixel() {

    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    this->updateUniforms();

    //Render models
    for (auto &i : this->models)
        i->render(this->shaders[SHADER_CORE_PROGRAM]);
    glfwSwapBuffers(window);

    GLfloat pixels[WINDOW_WIDTH * WINDOW_HEIGHT];
    if (!DISABLE_GL_READ)
        glReadPixels(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, GL_DEPTH_COMPONENT, GL_FLOAT, pixels);
    else
        for(int i = 0; i < WINDOW_WIDTH * WINDOW_HEIGHT; i++)
            pixels[i] = 0.8;
    float minValue = 10000.f;
    for (size_t i = 0; i < this->WINDOW_WIDTH * this->WINDOW_HEIGHT; i++) {
        pixels[i] = 200 * (pixels[i]) - 100;

        if ((depthPixels[i] != 100) && (pixels[i] - depthPixels[i]) < minValue) {
            minValue = (pixels[i] - depthPixels[i]);
            this->closestPixel = i;
        }
    }


    long closest_row_cord = long(float(this->closestPixel) / float(WINDOW_WIDTH)) + 1;
    long closest_column_cord = (this->closestPixel % WINDOW_WIDTH);
//    std::cout << closest_column_cord << "|" << closest_row_cord << std::endl;
    if (closest_column_cord < WINDOW_WIDTH / 2) {
        closest_column_cord = -((WINDOW_WIDTH / 2) - closest_column_cord);
    } else {
        closest_column_cord -= WINDOW_WIDTH / 2;
    }

    if (closest_row_cord < WINDOW_HEIGHT / 2) {
        closest_row_cord = -((WINDOW_HEIGHT / 2) - closest_row_cord);
    } else {
        closest_row_cord -= WINDOW_HEIGHT / 2;
    }

    float x_coord = (float) closest_column_cord / scaleFactor;
    float y_coord = (float) closest_row_cord / scaleFactor;

    std::cout << "Torus touched at " << x_coord << ", " << y_coord << std::endl;
    std::cout << "Z movement required for first point of contact :  " << minValue << std::endl;

//    std::vector<Mesh *> markerMesh;
//    Quad marker = Quad();
//    markerMesh.push_back(new Mesh(&marker));
//    this->models.push_back(new Model(glm::vec3(0.f), this->materials[0], markerMesh));
//
//    this->models.push_back(new Model(glm::vec3(x_coord, y_coord, 0.f), this->materials[0], markerMesh));

    //End Draw
    glFlush();

    glBindVertexArray(0);
    glUseProgram(0);
    glActiveTexture(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Game::swapTorusAndBezier() {
    this->models.pop_back();

    if (currently_visible == TORUS)
        models.push_back(bezierModel);
    else if (currently_visible == BEZIER)
        models.push_back(torusModel);

    currently_visible = !currently_visible;
}

//Static functions
void Game::framebuffer_resize_callback([[maybe_unused]] GLFWwindow *window, int fbW, int fbH) {
    glViewport(0, 0, fbW, fbH);
}

void Game::changeRenderMode([[maybe_unused]] GLFWwindow *window, int key, [[maybe_unused]] int scancode, int action,
                            [[maybe_unused]] int mods) {
    if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
        GLint polygonMode;
        glGetIntegerv(GL_POLYGON_MODE, &polygonMode);

        switch (polygonMode) {
            case GL_LINE:
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                break;
            case GL_FILL:
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                break;

            default:
                break;
        }
    }

    if (key == GLFW_KEY_CAPS_LOCK && action == GLFW_PRESS) {
        PROJECTION_MODE = !PROJECTION_MODE;
    }
    if (key == GLFW_KEY_RIGHT_SHIFT && action == GLFW_PRESS) {
    }
}

[[maybe_unused]] GLfloat *Game::reCalculateClosestPoint(GLfloat *) {
    return nullptr;
}

void Game::initialRender() {
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    //Update the uniforms
    this->updateUniforms();

    //Render models
    for (auto &i : this->models)
        i->render(this->shaders[SHADER_CORE_PROGRAM]);
    glfwSwapBuffers(window);

    GLfloat pixels[WINDOW_WIDTH * WINDOW_HEIGHT];
    glReadPixels(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, GL_DEPTH_COMPONENT, GL_FLOAT, pixels);


    for (size_t i = 0; i < this->WINDOW_WIDTH * this->WINDOW_HEIGHT; i++) {
        pixels[i] = 200 * (pixels[i]) - 100;
        // pixels[i] = pixels[i] - this->depthPixels[i];
    }


    glFlush();

    glBindVertexArray(0);
    glUseProgram(0);
    glActiveTexture(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

template<typename T>
std::vector<T> linspace(T a, T b, size_t N) {
    T h = (b - a) / static_cast<T>(N - 1);
    std::vector<T> xs(N);
    typename std::vector<T>::iterator x;
    T val;
    for (x = xs.begin(), val = a; x != xs.end(); ++x, val += h) // NOLINT(cert-flp30-c)
        *x = val;
    return xs;
}

int binomialCoeff(int n, int k) {
    // Base Cases
    if (k == 0 || k == n)
        return 1;

    // Recur
    return binomialCoeff(n - 1, k - 1) +
           binomialCoeff(n - 1, k);
}

std::vector<Vertex> generateTriangles() {
    int divisions = 151;
    double umin = 0.0, umax = 1.0, vmin = 0.0, vmax = 1.0;

    int N = 4, M = 4;

    int control_points[16][3] = {
            {-75, -75, 15},
            {-75, -25, 0},
            {-75, 25,  25},
            {-75, 75,  20},
            {-25, -75, 15},
            {-25, -25, 20},
            {-25, 25,  25},
            {-25, 75,  30},
            {25,  -75, 35},
            {25,  -25, 40},
            {25,  25,  30},
            {25,  75,  35},
            {75,  -75, 25},
            {75,  -25, 35},
            {75,  25,  35},
            {75,  75,  20},
    };

    std::vector<double> u1 = linspace(umin, umax, divisions);
    std::vector<double> v1 = linspace(vmin, vmax, divisions);

    glm::vec3 finalControlPointList[N][M];

    int temp = 0;

    for (size_t i = 0; i < N; i++) {
        for (size_t j = 0; j < M; j++) {

            finalControlPointList[i][j] = glm::vec3(control_points[temp][0], control_points[temp][1],
                                                    control_points[temp][2]);
            temp++;
        }
    }

    double u, v, b1;
    glm::vec3 triangle_array[divisions][divisions];
    glm::vec3 temp_vector;

    for (size_t i1 = 0; i1 < divisions; i1++) {
        u = u1[i1];
        for (size_t j1 = 0; j1 < divisions; j1++) {
            v = v1[j1];
            temp_vector = glm::vec3(0.f);
            for (size_t i = 0; i < N; i++) {
                for (size_t j = 0; j < M; j++) {
                    b1 = binomialCoeff(N - 1, i) * pow(u, i) * pow((1 - u), (N - 1 - i)) * binomialCoeff(M - 1, j) *
                         pow(v, j) * pow((1 - v), (M - 1 - j));
                    temp_vector += (float) b1 * finalControlPointList[i][j];
                }
            }

            triangle_array[i1][j1] = temp_vector;
        }
    }

    std::vector<Vertex> vertexArray;
    Vertex tempVertex{};
    tempVertex.color = glm::vec3(1.f);
    tempVertex.normal = glm::vec3(1.f);
    tempVertex.texcoord = glm::vec2(0.f, 1.f);

    for (size_t i = 0; i < divisions - 1; i++) {
        for (size_t j = 0; j < divisions - 1; j++) {
            tempVertex.position = triangle_array[j][i];
            vertexArray.push_back(tempVertex);
            tempVertex.position = triangle_array[j + 1][i];
            vertexArray.push_back(tempVertex);
            tempVertex.position = triangle_array[j + 1][i + 1];
            vertexArray.push_back(tempVertex);

            tempVertex.position = triangle_array[j][i];
            vertexArray.push_back(tempVertex);
            tempVertex.position = triangle_array[j + 1][i + 1];
            vertexArray.push_back(tempVertex);
            tempVertex.position = triangle_array[j][i + 1];
            vertexArray.push_back(tempVertex);
        }
    }

    return vertexArray;
}

std::vector<Vertex> generateTorus() {
    float radius_inner = 6.0f;
    float radius_outer = 6.7f;
    int theta_min = 0;
    int theta_max = -180;

    glm::vec3 wt = glm::vec3(0.f, 0.f, 1.f);
    glm::vec3 ut = glm::vec3(1.f, 0.f, 0.f);
    glm::vec3 vt = glm::vec3(0.f, -1.f, 0.f);
    glm::vec3 tc1 = glm::vec3(0.f, 0.f, 0.f);

    glm::vec3 ccc;

    int div_t = 51, div_p = 91;

    std::vector<double> theta_a = linspace((theta_min * M_PI) / 180, (theta_max * M_PI) / 180, div_t);

    double tempTheta;
    double tempPhi;

    std::vector<double> phi_i;

    glm::vec3 triangleVerticesArray[div_t][div_p];

    for (size_t i = 0; i < div_t; i++) {
        tempTheta = theta_a[i];
        phi_i = linspace((double) 0, 2 * M_PI, div_p);

        for (size_t j = 0; j < div_p; j++) {
            tempPhi = phi_i[j];
            ccc = ut * (float) ((radius_outer + radius_inner * cos(tempTheta)) * cos(tempPhi)) +
                  vt * (float) ((radius_outer + radius_inner * cos(tempTheta)) * sin(tempPhi)) +
                  wt * (float) (radius_inner * sin(tempTheta)) + tc1;

            triangleVerticesArray[i][j] = ccc;
        }
    }

    std::vector<Vertex> vertexArray;
    Vertex tempVertex{};
    tempVertex.color = glm::vec3(1.f, 0.f, 0.f);
    tempVertex.normal = glm::vec3(1.f);
    tempVertex.texcoord = glm::vec2(0.f, 1.f);

    for (size_t i = 0; i < div_p - 1; i++) {
        for (size_t j = 0; j < div_t - 1; j++) {
            tempVertex.position = triangleVerticesArray[j][i];
            vertexArray.push_back(tempVertex);
            tempVertex.position = triangleVerticesArray[j + 1][i];
            vertexArray.push_back(tempVertex);
            tempVertex.position = triangleVerticesArray[j + 1][i + 1];
            vertexArray.push_back(tempVertex);

            tempVertex.position = triangleVerticesArray[j][i];
            vertexArray.push_back(tempVertex);
            tempVertex.position = triangleVerticesArray[j + 1][i + 1];
            vertexArray.push_back(tempVertex);
            tempVertex.position = triangleVerticesArray[j][i + 1];
            vertexArray.push_back(tempVertex);
        }
    }

    return vertexArray;
}

// Equation

// (x - 52.5)*150/375,


#include "SphereRenderer.h"
#include "glfWindow/GLFWindow.h"
#include <GL/gl.h>
#include <vector>
#include <glm/glm.hpp>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

namespace spt {

  struct SampleWindow : public GLFCameraWindow {
    SampleWindow(const std::string &title, const std::vector<Sphere> &spheres, const Camera &camera, const float worldScale)
      : GLFCameraWindow(title, camera.position, camera.lookAt, camera.sceneUpDirection, worldScale),
        sample(spheres) {
      sample.setCamera(camera);

      // Setup Dear ImGui context
      IMGUI_CHECKVERSION();
      ImGui::CreateContext();
      ImGuiIO& io = ImGui::GetIO(); (void)io;
      io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

      // GL 3.0 + GLSL 130
      const char* glsl_version = "#version 130";
      glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
      glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

      // Setup Dear ImGui style
      ImGui::StyleColorsDark();
      ImGuiStyle& style = ImGui::GetStyle();

      float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());
      style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
      style.FontScaleDpi = main_scale;        // Set initial font scale. (in docking branch: using io.ConfigDpiScaleFonts=true automatically overrides this for every window depending on the current monitor)

      // Setup Platform/Renderer backends
      ImGui_ImplGlfw_InitForOpenGL(handle, false);
      ImGui_ImplOpenGL3_Init(glsl_version);
    }

    virtual void render() override {
      static bool imgui_callbacks_installed = false;
      if (!imgui_callbacks_installed) {
        ImGui_ImplGlfw_InstallCallbacks(handle);
        imgui_callbacks_installed = true;
      }

      if (cameraFrame.modified) {
        sample.setCamera(Camera{ cameraFrame.get_cameraPosition(), cameraFrame.get_lookAt(), cameraFrame.get_sceneUpDirection() });
        cameraFrame.modified = false;
      }
      sample.render();
    }

    // gets the rendered image from a cuda buffer on the gpu and displays it on the screen using glfw 
    // (which renders a texture on a rectangle via opengl)
    virtual void draw() override {
      sample.downloadPixels(pixels.data());
      if (fbTexture == 0) {
        glGenTextures(1, &fbTexture);
      }

      glBindTexture(GL_TEXTURE_2D, fbTexture);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, framebufferSize.x, framebufferSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

      glDisable(GL_LIGHTING);
      glColor3f(1, 1, 1);
      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();
      glEnable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, fbTexture);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glDisable(GL_DEPTH_TEST);
      glViewport(0, 0, framebufferSize.x, framebufferSize.y);
      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      glOrtho(0.f, (float)framebufferSize.x, 0.f, (float)framebufferSize.y, -1.f, 1.f);

      glBegin(GL_QUADS);
      glTexCoord2f(0.f, 0.f); glVertex3f(0.f, 0.f, 0.f);
      glTexCoord2f(0.f, 1.f); glVertex3f(0.f, (float)framebufferSize.y, 0.f);
      glTexCoord2f(1.f, 1.f); glVertex3f((float)framebufferSize.x, (float)framebufferSize.y, 0.f);
      glTexCoord2f(1.f, 0.f); glVertex3f((float)framebufferSize.x, 0.f,  0.f);
      glEnd();

      // Start the Dear ImGui frame
      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();

      ImGui::ShowDemoWindow();

      ImGui::Render();
      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    virtual void resize(const glm::ivec2 &newSize) override {
      framebufferSize = newSize;
      sample.resize(newSize);
      pixels.resize(newSize.x * newSize.y);
    }

    glm::ivec2 framebufferSize;
    GLuint fbTexture {0};
    SampleRenderer sample;
    std::vector<uint32_t> pixels;
  };

  extern "C" int main(int ac, char **av) {
    try {
      std::vector<Sphere> spheres;

      // center, radius, color, emissionColor, emissiveStrength
      spheres.push_back({glm::vec3( 0.f, 0.f, 0.f), 0.25f, glm::vec3(1.f, 1.f, 1.f), 1.0f, glm::vec3(0.9f, 0.9f, 1.0f), 0.f});
      spheres.push_back({glm::vec3( 2.f, 0.f, 0.f), 0.75f, glm::vec3(.4f, .9f, .4f), 0.f, glm::vec3(0.f), 0.f});
      spheres.push_back({glm::vec3(-2.f, 0.f, 0.f), 0.75f, glm::vec3(1.f, 1.f, 1.f), 0.f, glm::vec3(0.f), 0.f});
      spheres.push_back({glm::vec3(0.f, 0.0f, 2.1f), 1.0f, glm::vec3(1.f, 1.f, 1.f), 0.f, glm::vec3(0.f), 0.25f});

      Camera camera = { glm::vec3(-3.f, 1.f, -4.f), glm::vec3( 0.f, 0.f,  0.f), glm::vec3( 0.f, 1.f,  0.f) };

      const float worldScale = 5.f;
      SampleWindow *window = new SampleWindow("Sphere Path Tracer", spheres, camera, worldScale);
      window->run();

    } catch (std::runtime_error &e) {
      std::cout << "FATAL ERROR: " << e.what() << std::endl;
      exit(1);
    }
    return 0;
  }
}
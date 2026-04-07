#include "SphereRenderer.h"
#include "glfWindow/GLFWindow.h"
#include <GL/gl.h>
#include <vector>
#include <glm/glm.hpp>
#include <chrono>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

namespace spt {
    void generateScene(std::vector<Sphere>& spheres, bool stressTest) {
        spheres.clear();

        if (stressTest) {
            // center, radius, color, emissiveStrength, emissionColor, transparency, refractiveIndex, materialType
            spheres.push_back({ glm::vec3(0.f, 0.f, 0.f), 0.25f, glm::vec3(1.f, 1.f, 1.f), 100.0f, glm::vec3(0.9f, 0.9f, 1.0f), 0.f, 1.0f, MATERIAL_REFLECTIVE });
            spheres.push_back({ glm::vec3(-10.f, 15.f, 0.f), 0.75f, glm::vec3(.4f, .9f, .4f), 0.f, glm::vec3(0.f), 0.f, 1.0f, MATERIAL_LAMBERTIAN });

            spheres.push_back({ glm::vec3(-1.f, -2.5f, 0.f), 0.75f, glm::vec3(1.f, 1.f, 1.f), 0.f, glm::vec3(0.f), 0.f, 1.0f, MATERIAL_LAMBERTIAN });

            // Glass
            spheres.push_back({ glm::vec3(3.f, 0.f, -6.f), 0.75f, glm::vec3(1.f, 1.f, 1.f), 0.f, glm::vec3(0.f), 1.f, 5.f, MATERIAL_REFLECTIVE });
            spheres.push_back({ glm::vec3(2.f, 0.f, -4.f), 0.75f, glm::vec3(1.f, 1.f, 1.f), 0.f, glm::vec3(0.f), 1.f, 1.9f, MATERIAL_REFLECTIVE });
            spheres.push_back({ glm::vec3(2.f, 0.f, -2.f), 0.75f, glm::vec3(1.f, 1.f, 1.f), 0.f, glm::vec3(0.f), 1.f, 1.5f, MATERIAL_REFLECTIVE });
            spheres.push_back({ glm::vec3(2.f, 0.f, -0.f), 0.75f, glm::vec3(1.f, 1.f, 1.f), 0.f, glm::vec3(0.f), 1.f, 1.25f, MATERIAL_REFLECTIVE });
            spheres.push_back({ glm::vec3(2.f, 0.f, 2.f), 0.75f, glm::vec3(1.f, 1.f, 1.f), 0.f, glm::vec3(0.f), 1.f, 1.1f, MATERIAL_REFLECTIVE });
            spheres.push_back({ glm::vec3(2.f, 0.f, 4.f), 0.75f, glm::vec3(1.f, 1.f, 1.f), 0.f, glm::vec3(0.f), 1.f, 0.8f, MATERIAL_REFLECTIVE });

            spheres.push_back({ glm::vec3(-2.f, 0.f, 0.f), 0.75f, glm::vec3(1.f, 1.f, 1.f), 0.f, glm::vec3(0.f), 0.f, 1.0f, MATERIAL_REFLECTIVE });
            spheres.push_back({ glm::vec3(0.f, 0.0f, 2.1f), 1.0f, glm::vec3(1.f, 1.f, 1.f), 0.f, glm::vec3(0.f), 8.f, 1.0f, MATERIAL_REFLECTIVE }); // Transparency is bugged right now

            //spheres.push_back({glm::vec3( 500.f, 0.f, 0.f), 450.f, glm::vec3(.0f, .0f, .0f), 0.f, glm::vec3(0.f), 0.f, 1.0f, MATERIAL_LAMBERTIAN});
            spheres.push_back({ glm::vec3(0.f, -100.f, 0.f), 94.f, glm::vec3(0.75f, 1.f, 1.f), 0.f, glm::vec3(0.f), 0.f, 1.0f, MATERIAL_REFLECTIVE });

            for (int i = 0; i < 4000; i++) {
                // A cylindrical wall of spheres with random positions, colors, and radii, with the rest of the scene at the center
                float radiusCylinderOfSpheresInner = 50.f;
                float radiusCylinderOfSpheresOuter = 100.f;
                float heightCylinderOfSpheres = 200.f;
                // Random float in range [0,1]
                float rx = std::rand() / (float)RAND_MAX;
                float ry = std::rand() / (float)RAND_MAX;
                float rz = std::rand() / (float)RAND_MAX;

                int randomMaterialRange = 4;
                int material = std::rand() % randomMaterialRange;

                float r1 = std::rand() / (float)RAND_MAX * 2. + 0.25f;
                float radius = r1 * r1 * r1; // Square the radius to bias towards smaller spheres

                glm::vec3 pos = glm::vec3(
                    std::cos(rx * 2.f * 3.14159f) * (radiusCylinderOfSpheresInner + (radiusCylinderOfSpheresOuter - radiusCylinderOfSpheresInner) * ry),
                    (rz * heightCylinderOfSpheres) - 25.f,
                    std::sin(rx * 2.f * 3.14159f) * (radiusCylinderOfSpheresInner + (radiusCylinderOfSpheresOuter - radiusCylinderOfSpheresInner) * ry)
                );
                glm::vec3 color = glm::vec3(rx, ry, rz);
                switch (material) {
                case 0:
                    spheres.push_back({ pos, radius, color, 0.f, glm::vec3(0.f), 0.f, 1.0f, MATERIAL_LAMBERTIAN });
                    break;
                case 1:
                    spheres.push_back({ pos, radius, color, 0.f, glm::vec3(0.f), 0.f, 1.0f, MATERIAL_REFLECTIVE });
                    break;
                case 2: { // emmissive, colored light sources
                    float randombrightness = (std::rand() / (float)RAND_MAX) * 4.5f + 0.5f; // Random brightness
                    spheres.push_back({ pos, radius, color, randombrightness, color, 0.f, 1.0f, MATERIAL_LAMBERTIAN });
                    break;
                }
                case 3: { // transparent spheres with random refractive indices
                    float randomRefractiveIndex = (std::rand() / (float)RAND_MAX) * 4.f + 0.5f; // Random refractive index between 0.5 and 4.5
                    spheres.push_back({ pos, radius, color, 0.f, glm::vec3(0.f), 1.f, randomRefractiveIndex, MATERIAL_REFLECTIVE });
                    break;
                }
                default:
                    spheres.push_back({ pos, radius, color, 0.f, glm::vec3(0.f), 0.f, 1.0f, MATERIAL_LAMBERTIAN });
                    break;
                }
            }
        }
        else {
            // center, radius, color, emissionColor, emissiveStrength, transparency, refractiveIndex, materialType
            spheres.push_back({ glm::vec3(0.f, 0.f, 0.f), 0.25f, glm::vec3(1.f, 1.f, 1.f), 100.0f, glm::vec3(0.9f, 0.9f, 1.0f), 0.f, 1.0f, MATERIAL_REFLECTIVE });
            spheres.push_back({ glm::vec3(-2.f, 3.f, 0.f), 0.75f, glm::vec3(.4f, .9f, .4f), 0.f, glm::vec3(0.f), 0.f, 1.0f, MATERIAL_LAMBERTIAN });
            spheres.push_back({ glm::vec3(0.f, 6.f, -3.f), 0.75f, glm::vec3(1.f, 1.f, 1.f), 0.f, glm::vec3(0.f), 0.f, 1.0f, MATERIAL_LAMBERTIAN });
            spheres.push_back({ glm::vec3(2.1f, 0.f, 0.f), 0.75f, glm::vec3(1.f, 1.f, 1.f), 0.f, glm::vec3(0.f), 1.f, 1.25f, MATERIAL_REFLECTIVE });

            spheres.push_back({ glm::vec3(-2.f, 0.f, 0.f), 0.75f, glm::vec3(1.f, 1.f, 1.f), 0.f, glm::vec3(0.f), 0.f, 1.0f, MATERIAL_REFLECTIVE });
            spheres.push_back({ glm::vec3(0.f, 0.0f, 2.1f), 1.0f, glm::vec3(1.f, 1.f, 1.f), 0.f, glm::vec3(0.f), .2f, 1.0f, MATERIAL_REFLECTIVE });
            spheres.push_back({ glm::vec3(100.f, 0.f, 0.f), 94.f, glm::vec3(.0f, .0f, .0f), 0.f, glm::vec3(0.f), 0.f, 1.0f, MATERIAL_LAMBERTIAN });
            spheres.push_back({ glm::vec3(0.f, -100.f, 0.f), 94.f, glm::vec3(0.75f, 1.f, 1.f), 0.f, glm::vec3(0.f), 0.f, 1.0f, MATERIAL_REFLECTIVE });
        }
    }

  struct SampleWindow : public GLFCameraWindow {
    SampleWindow(const std::string &title, const std::vector<Sphere> &spheres, const Camera &camera, const float worldScale)
      : GLFCameraWindow(title, camera.position, camera.lookAt, camera.sceneUpDirection, worldScale),
        sample(spheres),
        startTime(std::chrono::high_resolution_clock::now()) {
      sample.setCamera(camera);

      // Setup Dear ImGui context
      IMGUI_CHECKVERSION();
      ImGui::CreateContext();
      ImGuiIO& io = ImGui::GetIO();
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

      updateFrame();

      if (cameraFrame.modified) {
        sample.setCamera(Camera{ cameraFrame.get_cameraPosition(), cameraFrame.get_lookAt(), cameraFrame.get_sceneUpDirection() });
        cameraFrame.modified = false;
      }
      sample.render();
    }

    // Update function: Called before each frame
    virtual void updateFrame() {
      auto now = std::chrono::high_resolution_clock::now();
      auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count() * 0.001f;  // Elapsed time in seconds
      std::vector<Sphere>& spheres = sample.getSpheres();
      spheres[1].radius = 2.f + 1.5f * sin(elapsed);
      sample.updateSpheres();
    }

    // All the Imgui code for drawing the control panel window
    void DrawControlPanel() {
        ImGui::Begin("Control Panel", (bool *)nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        ImGuiIO& io = ImGui::GetIO();
        ImGui::Text("FPS: %.1f", io.Framerate);

		ImGui::SeparatorText("Global Settings");

        LaunchParams& launchParams = sample.getLaunchParams();

        ImGui::Text("Max Ray Depth:");
        ImGui::SameLine();
        ImGui::InputInt("##maxDepth", &launchParams.maxDepth, 1, 5);

        ImGui::Text("Air Refractive Index:");
		ImGui::SameLine();
        ImGui::InputFloat("##airRefractiveIndex", &launchParams.airRefractiveIndex, 0.01f, 1.0f, "%.3f");

        ImGui::SeparatorText("Spheres");

        std::vector<Sphere>& spheres = sample.getSpheres();
		int i = 0; // Just for numbering the spheres in the UI 
        for (Sphere& s : spheres) {
            i++;

            if (ImGui::TreeNode((void*)i, "Sphere #%d", i)) {
				// Using a table to align the labels and inputs in two columns
                if (ImGui::BeginTable(("##table" + std::to_string(i)).c_str(), 2, ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_SizingFixedFit)) {

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Position:");
                    ImGui::TableSetColumnIndex(1);
					ImGui::PushItemWidth(400.f); // Set a fixed width for all the input fields
                    float pos[3] = { s.center.x, s.center.y, s.center.z };
                    ImGui::InputFloat3(("##position" + std::to_string(i)).c_str(), pos);
                    s.center = glm::vec3(pos[0], pos[1], pos[2]);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Radius:");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::InputFloat(("##radius" + std::to_string(i)).c_str(), &s.radius, 0.01f, 1.0f, "%.3f");

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Color:");
                    ImGui::TableSetColumnIndex(1);
                    ImVec4 color(s.color.r, s.color.g, s.color.b, 1.0f);
                    ImGui::ColorEdit3(("##color" + std::to_string(i)).c_str(), (float*)&color);
                    s.color = glm::vec3(color.x, color.y, color.z);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Emissive Strength:");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::InputFloat(("##emissiveStrength" + std::to_string(i)).c_str(), &s.emissiveStrength, 0.01f, 1.0f, "%.3f");

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Emission Color:");
                    ImGui::TableSetColumnIndex(1);
                    ImVec4 emissionColor(s.emissionColor.r, s.emissionColor.g, s.emissionColor.b, 1.0f);
                    ImGui::ColorEdit3(("##emissionColor" + std::to_string(i)).c_str(), (float*)&emissionColor);
                    s.emissionColor = glm::vec3(emissionColor.x, emissionColor.y, emissionColor.z);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Transparency:");
                    ImGui::TableSetColumnIndex(1);
					ImGui::SliderFloat(("##transparency" + std::to_string(i)).c_str(), &s.transparency, 0.f, 1.f);

                    if (s.transparency > 0.f) {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("Refractive Index:");
                        ImGui::TableSetColumnIndex(1);
                        ImGui::InputFloat(("##refractiveIndex" + std::to_string(i)).c_str(), &s.refractiveIndex, 0.01f, 1.0f, "%.3f");
                    }

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Material Type:");
                    ImGui::TableSetColumnIndex(1);
					const char *materialTypeStrings[] = {"Reflective", "Lambertian"};
					ImGui::Combo(("##materialType" + std::to_string(i)).c_str(), (int *)&s.materialType, materialTypeStrings, IM_ARRAYSIZE(materialTypeStrings));

					ImGui::PopItemWidth();
                    ImGui::EndTable();
                }

                ImGui::TreePop();
				ImGui::Spacing();
            }
        }

        ImGui::Separator();

        if (ImGui::Button("Add Sphere")) {
            spheres.push_back({glm::vec3(-2.f, 0.f, 0.f), 0.75f, glm::vec3(1.f, 1.f, 1.f), 0.f, glm::vec3(0.f), 0.f, 1.0f, MATERIAL_REFLECTIVE});
		}
		ImGui::SameLine();
        if (ImGui::Button("Rebuild")) {
            sample.updateSpheres();
        }

        ImGui::End();
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

      //ImGui::ShowDemoWindow();

      DrawControlPanel();

      ImGui::Render();
      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    virtual void resize(const glm::ivec2 &newSize) override {
      framebufferSize = newSize;
      sample.resize(newSize);
      pixels.resize(newSize.x * newSize.y);
    }

    virtual void key(int action, int key, int mods) override {
		const float speed = 0.5f;

		if (action == GLFW_PRESS && key == GLFW_KEY_F) {
            // Switch to the other scene
            stressTest = !stressTest;
			generateScene(sample.getSpheres(), stressTest);

			// Reset the camera position
			cameraFrame.setOrientation(glm::vec3(-3.f, 1.f, -4.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f));
            cameraFrame.modified = true;
            return;
        }

        if ((action == GLFW_PRESS) || (action == GLFW_REPEAT)) {
            switch (key) {
                case GLFW_KEY_W:
                    cameraFrame.position -= cameraFrame.frame.vz * speed;
                    break;
                case GLFW_KEY_S:
                    cameraFrame.position += cameraFrame.frame.vz * speed;
                    break;
                case GLFW_KEY_A:
                    cameraFrame.position -= cameraFrame.frame.vx * speed;
                    break;
                case GLFW_KEY_D:
                    cameraFrame.position += cameraFrame.frame.vx * speed;
                    break;
                case GLFW_KEY_SPACE:
                    cameraFrame.position += cameraFrame.frame.vy * speed;
                    break;
                case GLFW_KEY_LEFT_SHIFT:
                    cameraFrame.position -= cameraFrame.frame.vy * speed;
                    break;
			}

            cameraFrame.modified = true;
        }
    }

    glm::ivec2 framebufferSize;
    GLuint fbTexture {0};
    SampleRenderer sample;
    std::vector<uint32_t> pixels;
    std::chrono::high_resolution_clock::time_point startTime;
	bool stressTest = false;
  };

  extern "C" int main(int ac, char **av) {
    try {
      std::vector<Sphere> spheres;
      
	  generateScene(spheres, false);

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
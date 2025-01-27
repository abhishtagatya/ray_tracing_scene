#include "application.hpp"
#include "model_ubo.hpp"
#include "utils.hpp"
#include <random>

Application::Application(int initial_width, int initial_height, std::vector<std::string> arguments)
    : PV227Application(initial_width, initial_height, arguments) {
    Application::compile_shaders();
    prepare_cameras();
    prepare_materials();
    prepare_textures();
    prepare_snowman();
    prepare_lights();
    prepare_particles();
    prepare_scene();
    prepare_framebuffers();
}

Application::~Application() {}

// ----------------------------------------------------------------------------
// Shaderes
// ----------------------------------------------------------------------------
void Application::compile_shaders() {
    default_unlit_program = ShaderProgram(lecture_shaders_path / "object.vert", lecture_shaders_path / "unlit.frag");
    default_lit_program = ShaderProgram(lecture_shaders_path / "object.vert", lecture_shaders_path / "lit.frag");

	ray_tracing_program = ShaderProgram(lecture_shaders_path / "full_screen_quad.vert", lecture_shaders_path / "ray_tracing.frag");

    particle_program = ShaderProgram();
    particle_program.add_vertex_shader(lecture_shaders_path / "particle_textured.vert");
    particle_program.add_fragment_shader(lecture_shaders_path / "particle_textured.frag");
    particle_program.add_geometry_shader(lecture_shaders_path / "particle_textured.geom");
    particle_program.link();

    std::cout << "Shaders are reloaded." << std::endl;
}

// ----------------------------------------------------------------------------
// Initialize Scene
// ----------------------------------------------------------------------------
void Application::prepare_cameras() {
    // Sets the default camera position.
    camera.set_eye_position(glm::radians(-45.f), glm::radians(20.f), 25.f);
    // Computes the projection matrix.
    camera_ubo.set_projection(
        glm::perspective(glm::radians(45.f), static_cast<float>(this->width) / static_cast<float>(this->height), 1.0f, 1000.0f));
    camera_ubo.update_opengl_data();
}

void Application::prepare_materials() {
    const PBRMaterialData snow_material = PBRMaterialData(glm::vec3(1.f), glm::vec3(0.04f), 1.0f);
    const PBRMaterialData coal_material = PBRMaterialData(glm::vec3(0.1f), glm::vec3(0.004f), 1.0f);
    snowman.materials[0] = snow_material;
    snowman.materials[1] = snow_material;
    snowman.materials[2] = snow_material;

    snowman.materials[3] = snow_material;
    snowman.materials[4] = snow_material;
    
    snowman.materials[5] = coal_material;
    snowman.materials[6] = coal_material;

    snowman.materials[7] = coal_material;
    snowman.materials[8] = coal_material;
    snowman.materials[9] = coal_material;
}

void Application::prepare_textures() {
    particle_tex = TextureUtils::load_texture_2d(lecture_textures_path / "snow.jpg");
    // Particles are really small, use mipmaps for them.
    TextureUtils::set_texture_2d_parameters(particle_tex, GL_REPEAT, GL_REPEAT, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
}

void Application::prepare_lights() {
    phong_lights_ubo = PhongLightsUBO(3, GL_UNIFORM_BUFFER);
    phong_lights_ubo.set_global_ambient(glm::vec3(0.2f));
}

void Application::prepare_snowman() {
    // Body
    snowman.spheres[0] = glm::vec4(0.0f, 1.2f, 0.0f, 1.5f);
    snowman.spheres[1] = glm::vec4(0.0f, 3.5f, 0.0f, 1.0f);
    snowman.spheres[2] = glm::vec4(0.0f, 4.9f, 0.0f, 0.7f);
    // Hands
    snowman.spheres[3] = glm::vec4(1.0f, 3.6f, 0.0f, 0.5f);
    snowman.spheres[4] = glm::vec4(-1.0f, 3.6f, 0.0f, 0.5f);
    // Eyes
    snowman.spheres[5] = glm::vec4(0.25f, 5.2f, 0.55f, 0.1f);
    snowman.spheres[6] = glm::vec4(-0.25f, 5.2f, 0.55f, 0.1f);
    // Buttons
    snowman.spheres[7] = glm::vec4(0.0f, 3.9f, 0.9f, 0.1f);
    snowman.spheres[8] = glm::vec4(0.0f, 3.5f, 1.0f, 0.1f);
    snowman.spheres[9] = glm::vec4(0.0f, 3.1f, 0.9f, 0.1f);
}

void Application::prepare_particles() {
    // Initialize random number generators
    std::random_device rd;
    std::mt19937 gen(rd());
	std::uniform_int_distribution<> int_dist(0, 2);  // Random light id
    std::uniform_real_distribution<> real_dist(0.0f, 5000.0f);  // Random delay in ms

    // Reserve space for vectors
    particle_data.resize(max_particle_count);

    // Populate the light_ids and delays vectors
    for (int i = 0; i < desired_snow_count; i++) {
		particle_data[i].light_id = i % 3;
		particle_data[i].delay = real_dist(gen);

		particle_data[i].position = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
		particle_data[i].velocity = glm::vec3(0.0f, 0.0f, 0.0f);
		particle_data[i].color = glm::vec3(1.0f, 1.0f, 1.0f);
    }

    // Create the particle SSBO
    glGenBuffers(1, &particle_ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, particle_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Particle) * desired_snow_count, particle_data.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    std::cout << sizeof(Particle) * desired_snow_count << std::endl;

}

void Application::update_particle_buffer() {

    std::cout << sizeof(Particle) * desired_snow_count << std::endl;

    // Initialize random number generators
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> int_dist(0, 2);  // Random light id
    std::uniform_real_distribution<> real_dist(0.0f, 5000.0f);  // Random delay in ms

    // Populate the light_ids and delays vectors
    for (int i = 0; i < desired_snow_count; i++) {
        particle_data[i].light_id = i % 3;
        particle_data[i].lifetime = real_dist(gen);
        particle_data[i].delay = particle_data[i].lifetime;

        particle_data[i].position = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
        particle_data[i].velocity = glm::vec3(0.0f, 0.0f, 0.0f);
        particle_data[i].color = glm::vec3(1.0f, 1.0f, 1.0f);
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, particle_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Particle) * desired_snow_count, particle_data.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void Application::prepare_scene() { 
    snowman_ubo = SnowmanUBO(snowman, GL_DYNAMIC_STORAGE_BIT);
}

void Application::prepare_framebuffers() {}

void Application::resize_fullscreen_textures() {}

// ----------------------------------------------------------------------------
// Update
// ----------------------------------------------------------------------------
glm::vec3 Application::HSVtoRGB(float h, float s, float v) {
    float r, g, b;

    int i = static_cast<int>(h / 60.0f) % 6;
    float f = (h / 60.0f) - i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - f * s);
    float t = v * (1.0f - (1.0f - f) * s);

    switch (i) {
    case 0:
        r = v;
        g = t;
        b = p;
        break;
    case 1:
        r = q;
        g = v;
        b = p;
        break;
    case 2:
        r = p;
        g = v;
        b = t;
        break;
    case 3:
        r = p;
        g = q;
        b = v;
        break;
    case 4:
        r = t;
        g = p;
        b = v;
        break;
    case 5:
        r = v;
        g = p;
        b = q;
        break;
    default:
        r = 0;
        g = 0;
        b = 0;
        break;
    }

    return {r, g, b};
}

void Application::update(float delta) {
    PV227Application::update(delta);

    // Updates the main camera.
    const glm::vec3 eye_position = camera.get_eye_position();
    camera_ubo.set_view(lookAt(eye_position, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
    camera_ubo.update_opengl_data();

    const float app_time_s = (float)elapsed_time * 0.001f;
    t_delta = delta;

    // Updates lights
    phong_lights_ubo.clear();
    std::vector<glm::vec3> positions(3);
    positions[0] = glm::vec3(4, 6, 4) * glm::vec3(cosf(app_time_s + 3.14f), 1, sinf(app_time_s + 3.14f));
    positions[1] = glm::vec3(4, 4, 4) * glm::vec3(cosf(app_time_s - 3.14f / 2.0f), 1, sinf(app_time_s + 3.14f / 2.0f));
    positions[2] = glm::vec3(5, 2, 5) * glm::vec3(cosf(app_time_s), 1, sinf(app_time_s));

    for (int i = 0; i < positions.size(); i++) {
        glm::vec3 color = HSVtoRGB((float)glm::fract(app_time_s * 0.1 + 0.33 * i) * 360.f, 1.0f, 0.8f);
        PhongLightData light = PhongLightData::CreatePointLight(positions[i], glm::vec3(0.0f), color, glm::vec3(0.1f), 1.0f, 0.0f, 0.0f);
        phong_lights_ubo.add(light);
    }

    for (int i = 0; i < light_count; i++) {
        PBRMaterialData light_pbr_material = PBRMaterialData(phong_lights_ubo.get_light(i).diffuse, glm::vec3(0.0), 0.0f);

		snowman.spheres[i + snowman_size] = glm::vec4(positions[i], sphere_light_radius);
        snowman.materials[i + snowman_size] = light_pbr_material;
    }

    snowman_ubo = SnowmanUBO(snowman, GL_DYNAMIC_STORAGE_BIT);
    phong_lights_ubo.update_opengl_data();
}

// ----------------------------------------------------------------------------
// Render
// ----------------------------------------------------------------------------
void Application::render() {
    // Starts measuring the elapsed time.
    glBeginQuery(GL_TIME_ELAPSED, render_time_query);

    // Binds the main window framebuffer.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);

    // Clears the framebuffer color.
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    // Binds the the camera and the lights buffers.
    camera_ubo.bind_buffer_base(CameraUBO::DEFAULT_CAMERA_BINDING);
    phong_lights_ubo.bind_buffer_base(PhongLightsUBO::DEFAULT_LIGHTS_BINDING);

    if (use_ray_tracing) {
		ray_trace_snowman();
    }
    else {
        raster_snowman();
    }

    render_particles();

    // Resets the VAO and the program.
    glBindVertexArray(0);
    glUseProgram(0);

    // Stops measuring the elapsed time.
    glEndQuery(GL_TIME_ELAPSED);

    // Waits for OpenGL - don't forget OpenGL is asynchronous.
    glFinish();

    // Evaluates the query.
    GLuint64 render_time;
    glGetQueryObjectui64v(render_time_query, GL_QUERY_RESULT, &render_time);
    fps_gpu = 1000.f / (static_cast<float>(render_time) * 1e-6f);
}

void Application::ray_trace_snowman() {
    // We do not need depth and depth test.
    // Enable depth testing with always pass for full-screen quad.
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS); // Always pass the depth test for ray tracing.

    // Uses the proper program.
    ray_tracing_program.use();
    ray_tracing_program.uniform("resolution", glm::vec2(width, height));
    ray_tracing_program.uniform("spheres_count", static_cast<int>(snowman_size + light_count));
    ray_tracing_program.uniform("iterations", reflections);
	ray_tracing_program.uniform("shadow_samples", shadow_samples);
	ray_tracing_program.uniform("time", (float)elapsed_time * 0.001f);
    ray_tracing_program.uniform("use_ambient_occlusion", corrective_use_ambient_occlusion);
	ray_tracing_program.uniform("ambient_occlusion_samples", ambient_occlusion_samples);

	// Binds the snowman.
    snowman_ubo.bind_buffer_base(3);

    // Renders the full screen quad to evaluate every pixel.
    // Binds an empty VAO as we do not need any state.
    glBindVertexArray(empty_vao);
    // Calls a draw command with 3 vertices that are generated in vertex shader.
    glDrawArrays(GL_TRIANGLES, 0, 3);

	glDepthFunc(GL_LESS); // Restore the depth function.
}

void Application::raster_snowman() {
    int id = 0;
    // Renders the snowman
    for (glm::vec4 sph : snowman.spheres) {
        default_lit_program.use();

        ModelUBO model_ubo(translate(glm::mat4(1.0f), glm::vec3(sph)) * scale(glm::mat4(1.0f), glm::vec3(sph.w)));

        // Handles the textures.
        default_lit_program.uniform("has_texture", false);
        glBindTextureUnit(0, 0);

        // Note that the materials are hard-coded here since the default lit shader works with PhongMaterial not PBRMaterial as defined in
        // snowman.
        if (id < 5) {
            white_material_ubo.bind_buffer_base(PhongMaterialUBO::DEFAULT_MATERIAL_BINDING);
        } else {
            black_material_ubo.bind_buffer_base(PhongMaterialUBO::DEFAULT_MATERIAL_BINDING);
        }
        model_ubo.bind_buffer_base(ModelUBO::DEFAULT_MODEL_BINDING);
        sphere.bind_vao();
        sphere.draw();
        id++;

		if (id == snowman_size) {
			break;
		}
    }

    // Renders the lights.
    for (int i = 0; i < 3; i++) {
        default_unlit_program.use();

        ModelUBO model_ubo(translate(glm::mat4(1.0f), glm::vec3(phong_lights_ubo.get_light(i).position)) *
                           scale(glm::mat4(1.0f), glm::vec3(sphere_light_radius)));

        // Note that the material is hard-coded here since the default lit shader works with PhongMaterial not PBRMaterial as defined in
        // snowman.
        light_material_ubo.set_material(PhongMaterialData(phong_lights_ubo.get_light(i).diffuse, true, 200.0f, 1.0f));
        light_material_ubo.update_opengl_data();

        light_material_ubo.bind_buffer_base(PhongMaterialUBO::DEFAULT_MATERIAL_BINDING);
        model_ubo.bind_buffer_base(ModelUBO::DEFAULT_MODEL_BINDING);
        sphere.bind_vao();
        sphere.draw();
    }
}

void Application::render_particles() {

    glDepthMask(GL_FALSE);         // Disable depth writing for transparent objects
    glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendFunc(GL_ONE, GL_ONE);

    // Renders the particles
    if (show_particles) {
        particle_program.use();
        particle_program.uniform("t_delta", (float)t_delta * 0.0001f);
        particle_program.uniform("particle_size_vs", particle_size);
        particle_program.uniform("light_radius", sphere_light_radius);

        glBindTextureUnit(0, particle_tex);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, particle_ssbo);

        glBindVertexArray(empty_vao);
        glDrawArrays(GL_POINTS, 0, desired_snow_count);
    }

    // Disables the blending.
    glDepthMask(GL_TRUE);          // Enable depth writing
    glDisable(GL_BLEND);
}

// ----------------------------------------------------------------------------
// GUI
// ----------------------------------------------------------------------------
void Application::render_ui() {
    const float unit = ImGui::GetFontSize();

    ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_NoDecoration);
    ImGui::SetWindowSize(ImVec2(20 * unit, 17 * unit));
    ImGui::SetWindowPos(ImVec2(2 * unit, 2 * unit));

    ImGui::PushItemWidth(150.f);

    std::string fps_cpu_string = "FPS (CPU): ";
    ImGui::Text(fps_cpu_string.append(std::to_string(fps_cpu)).c_str());

    std::string fps_string = "FPS (GPU): ";
    ImGui::Text(fps_string.append(std::to_string(fps_gpu)).c_str());

    ImGui::SliderInt("Reflections Quality", &reflections, 1, 100);

    current_snow_count = desired_snow_count;
    const char* particle_labels[10] = {"256", "512", "1024", "2048", "4096", "8192", "16384", "32768", "65536", "131072"};
    int exponent = static_cast<int>(log2(current_snow_count) - 8); // -8 because we start at 256 = 2^8
    if (ImGui::Combo("Particle Count", &exponent, particle_labels, IM_ARRAYSIZE(particle_labels))) {
        desired_snow_count = static_cast<int>(glm::pow(2, exponent + 8)); // +8 because we start at 256 = 2^8
		update_particle_buffer();
    }

    ImGui::Checkbox("Show Particles", &show_particles);

    ImGui::SliderFloat("Sphere Light Radius", &sphere_light_radius, 0, 1, "%.1f");
    ImGui::SliderInt("Shadow Quality", &shadow_samples, 1, 128);

    ImGui::Checkbox("Use Ray Tracing", &use_ray_tracing);

	if (use_ray_tracing) {
		ImGui::Checkbox("Use Ambient Occlusion", &corrective_use_ambient_occlusion);
		ImGui::SliderInt("Ambient Occlusion Samples", &ambient_occlusion_samples, 4, 64);
	}

    ImGui::End();
}

// ----------------------------------------------------------------------------
// Input Events
// ----------------------------------------------------------------------------
void Application::on_resize(int width, int height) {
    PV227Application::on_resize(width, height);
    resize_fullscreen_textures();
}

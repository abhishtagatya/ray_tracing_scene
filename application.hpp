#pragma once
#include "camera_ubo.hpp"
#include "light_ubo.hpp"
#include "pbr_material_ubo.hpp"
#include "pv227_application.hpp"
#include "ubo_impl.hpp" // required for UBO with snowman

/** The number of spheres forming the snowman. */
constexpr int snowman_size = 10;

/** The number of lights in the scene */
constexpr int light_count = 3;

/** The max number of particles */
constexpr int max_particle_count = 131072;

/** The structure defining the snowman. */
struct Snowman {
    glm::vec4 spheres[snowman_size + light_count];         // The spheres defining the snowman.
    PBRMaterialData materials[snowman_size + light_count]; // The respective materials for each sphere.
};

/** The definition of a snowman ubo. */
class SnowmanUBO : public UBO<Snowman> {
    using UBO<Snowman>::UBO; // copies constructors from the parent class
};

struct Particle {
	glm::vec4 position; // The position of particles (on CPU).
	glm::vec3 velocity; // The velocity of particles (on CPU).
    int light_id; // The id of the light source
	glm::vec3 color; // The colors of all particles (on GPU).
	float delay; // The delay of the particle
	float lifetime; // The lifetime of the particle
};

class Application : public PV227Application {
    // ----------------------------------------------------------------------------
    // Variables (Geometry)
    // ----------------------------------------------------------------------------
  protected:
    /** The definition of the snowman. */
    Snowman snowman;
    /** The buffer with the snowman. */
    SnowmanUBO snowman_ubo;

    // ----------------------------------------------------------------------------
    // Variables (Textures)
    // ----------------------------------------------------------------------------
  protected:
    // ----------------------------------------------------------------------------
    // Variables (Light)
    // ----------------------------------------------------------------------------
  protected:
    /** The UBO storing the data about lights - positions, colors, etc. */
    PhongLightsUBO phong_lights_ubo;
    /** The UBO defining a material that is used for lights during rasterization. */
    PhongMaterialUBO light_material_ubo;

    // ----------------------------------------------------------------------------
    // Variables (Camera)
    // ----------------------------------------------------------------------------
  protected:
    /** The UBO storing the information about camera. */
    CameraUBO camera_ubo;

    // ----------------------------------------------------------------------------
    // Variables (Shaders)
    // ----------------------------------------------------------------------------
	/** The shader program for rendering the snowman using ray tracing. */
	ShaderProgram ray_tracing_program;

	/** The shader program for rendering the particle. */
    ShaderProgram particle_program;

  protected:
    // ----------------------------------------------------------------------------
    // Variables (Frame Buffers)
    // ----------------------------------------------------------------------------
  protected:
    // ----------------------------------------------------------------------------
    // Variables (GUI)
    // ----------------------------------------------------------------------------
  protected:
    /** The number of iterations. */
    int reflections = 3;

    /** The desired snow particle count. */
    int desired_snow_count = 4096;

    /** The current snow particle count. */
    int current_snow_count = 256;

    /** The flag determining if a snow should be visible. */
    bool show_particles = true;

    /** The number of shadow samples. */
    int shadow_samples = 16;

    /** The flag determining if an area light should be present. */
    float sphere_light_radius = 0.5f;

    /** The flag determining if the ambient occlusion should be used. */
    bool corrective_use_ambient_occlusion = true;

	/** The number of ambient occlusion samples. */
	int ambient_occlusion_samples = 16;

	/** The flag determining if the snowman should be rendered using raytracing. */
    bool use_ray_tracing = false;

protected:
    // ----------------------------------------------------------------------------
    // Variables (Particles)
    // ----------------------------------------------------------------------------

    GLuint particle_tex;

	/** The Particle Shader Storage BO */
	GLuint particle_ssbo;

	/** The Particle Data Array */
    std::vector<Particle> particle_data;

	/** Particle Size */
    float particle_size = 0.5f;

    float t_delta = 0;

    // ----------------------------------------------------------------------------
    // Constructors
    // ----------------------------------------------------------------------------
  public:
    Application(int initial_width, int initial_height, std::vector<std::string> arguments = {});

    /** Destroys the {@link Application} and releases the allocated resources. */
    virtual ~Application();

    // ----------------------------------------------------------------------------
    // Shaders
    // ----------------------------------------------------------------------------
    /**
     * {@copydoc PV227Application::compile_shaders}
     */
    void compile_shaders() override;

    // ----------------------------------------------------------------------------
    // Initialize Scene
    // ----------------------------------------------------------------------------
  public:
    /** Prepares the required cameras. */
    void prepare_cameras();

    /** Prepares the required materials. */
    void prepare_materials();

    /** Prepares the required textures. */
    void prepare_textures();

    /** Prepares the lights. */
    void prepare_lights();

    /** Builds a snowman from individual parts. */
    void prepare_snowman();

    /** Prepares particle setup. */
	void prepare_particles();

    /** Prepares the scene objects. */
    void prepare_scene();

    /** Prepares the frame buffer objects. */
    void prepare_framebuffers();

    /** Resizes the full screen textures match the window. */
    void resize_fullscreen_textures();

    /** Updates the Particle Buffer on Change */
	void update_particle_buffer();

    // ----------------------------------------------------------------------------
    // Update
    // ----------------------------------------------------------------------------
    /**
     * Converts color in HSV to RGB.
     *
     * @param 	h	The hue [0-360].
     * @param 	s	The saturation [0-1].
     * @param 	v	The value [0-1].
     *
     * @return	The converted RGC color.
     */
    glm::vec3 HSVtoRGB(float h, float s, float v);

    /**
     * {@copydoc PV227Application::update}
     */
    void update(float delta) override;

    // ----------------------------------------------------------------------------
    // Render
    // ----------------------------------------------------------------------------
  public:
    /** @copydoc PV227Application::render */
    void render() override;

    /** Renders the snowman using rasterization. */
    void raster_snowman();

	/** Renders the snowman using ray tracing. */
	void ray_trace_snowman();

	/** Renders the particles. */
	void render_particles();
    // ----------------------------------------------------------------------------
    // GUI
    // ----------------------------------------------------------------------------
  public:
    /** @copydoc PV227Application::render_ui */
    void render_ui() override;

    // ----------------------------------------------------------------------------
    // Input Events
    // ----------------------------------------------------------------------------
  public:
    /** @copydoc PV227Application::on_resize */
    void on_resize(int width, int height) override;
};

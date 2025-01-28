# Ray Tracing Scene

![](docs/demo.gif)

Ray Tracing and Particle Simulation using OpenGL.

## Details
- Ray Tracing the model (Static Snowman and Dynamic Light Spheres) and Rasterizing the Particle Simulation.
- Ray Tracing with Soft Shadows (Adjustable by Samples and Light Radius) with Spherical Ambient Occlusion.
- Particle Simulation with adjustable configurations on particle count and particle size.
- Particle Simulation motion calculation within Vertex Shader and dissolving effect based on decay on Geometry and Fragment Shader.
- Using Spherical Ambient Occlusion by Ray Tracing.
- Optimization on Early Exit for Low Attenuation.

## Performance

Machine Specification : Intel Core i5-1335U, Intel Iris Xe Graphics, 16GB RAM

| Scene | FPS CPU-GPU |
| --- | --- |
| Ray Tracing Basic* | 14-16 |
| w/ Max Particle Simulation | 13-14 |
| w/ Max Shadow Sample | 2-3 |
| w/ Max Ambient Occlusion | 2-2.5 |

* Basic = (3 - 100 Reflections, 16 Shadow, 16 AO, 4096 Particles)

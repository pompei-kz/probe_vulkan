module;

#include <glm/vec3.hpp>
#include <string>
#include <vector>

export module pipeline;
import cmd;

export namespace cmd {

  struct CmdPipeline : Cmd
  {
    // Identity of pipeline
    std::string id;
  };

  // Material of shape
  struct Material
  {
    glm::vec3 color;
  };

  struct Shape
  {
    // Index of mesh from field `CmdPipeline_ShapeGroup_Materials.meshes`
    uint32_t meshIndex;

    // Moves the mesh to the specified position
    glm::vec3 position;

    // Scaling along the corresponding axes. 1 - no scaling, 0<s<1 - decrease, s>1 - increase.
    float scaleX, scaleY, scaleZ;

    // Rotation angles in degrees around the corresponding axes. Used in the ZYX sequence.
    float rotateZ, rotateY, rotateX;

    // index of material from field `CmdPipeline_ShapeGroup_Materials.materials`
    uint32_t materialIndex;
  };

  struct TriangleIdx
  {
    uint32_t index0, index1, index2;
  };

  // This is a mesh. It consists of triangles. A triangle is defined by a triad of indices from a point array.
  struct Mesh
  {
    std::vector<glm::vec3>   points;
    std::vector<TriangleIdx> triangles;
  };

  // Defines pipeline to draw group of shapes.
  // Each shape is mesh moved to position and rotated on two angles.
  struct CmdPipeline_ShapeGroup_Materials : CmdPipeline
  {
    // Drawing meshes. Shape is modified mesh
    std::vector<Mesh> meshes;

    // Materials for each shape
    std::vector<Material> materials;

    // Drawing shapes with references by index to materials, and to meshes
    std::vector<Shape> shapes;
  };

} // namespace cmd

module;

#include <functional>
#include <glm/vec3.hpp>
#include <memory>
#include <string>
#include <vector>

export module cmd_pipeline;
import cmd;

export namespace cmd {

  /**
   * Adds pipeline. This class is abstract - use its children
   * If pipeline with such id already exists and drawing the old pipeline must be removed and replaced with this pipeline
   */
  struct CmdSetPipeline : Cmd
  {
    /**
     * ID of pipeline
     */
    std::string id; // TODO this field is putting to key of `TriangleApplication::Impl.pipeline_map_`
  };

  /**
   * Removes pipeline by id
   */
  struct CmdRemovePipeline : Cmd
  {
    /**
     * ID of pipeline
     */
    std::string id;
  };

  /**
   * Material of shape
   */
  struct Material
  {
    // Color of material
    glm::vec3 color;
  };

  /**
   * Information for one shape
   */
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

  /**
   * Indexes of one triangle
   */
  struct TriangleIdx
  {
    uint32_t index0, index1, index2;
  };

  /**
   * This is a mesh. It consists of triangles. A triangle is defined by a triad of indices from a point array.
   */
  struct Mesh
  {
    /**
     * Points of vertexes of this mesh
     */
    std::vector<glm::vec3> points;

    /**
     * Triangle indexes of vertexes from field `this->points`
     */
    std::vector<TriangleIdx> triangles;
  };

  /**
   * Adds pipeline based on shapes.
   * Each shape is blended by a specific vector,
   * has a specific material, and has scaling along the Ox, Oy, Oz axes
   * and has rotations around Oz, Oy, Oz.
   * Scaling occurs first, and only then rotations occur in order: around Oz, then around Oy, and then around 0x.
   * Rotations are specified in degrees.
   */
  struct CmdSetPipeline_ShapeGroup : CmdSetPipeline
  {
    /**
     * Drawing meshes. Shape is modified mesh
     */
    std::vector<Mesh> meshes;

    /**
     * Materials for each shape
     */
    std::vector<Material> materials;

    /**
     * Each frame system calls this function and draw shapes.
     *
     * Size of this vector may be changed, but it is a rare event.
     */
    std::function<std::shared_ptr<std::vector<Shape>>()> shapesFn;
  };
} // namespace cmd

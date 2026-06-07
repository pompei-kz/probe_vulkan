module;

#include <functional>
#include <glm/fwd.hpp>
#include <glm/gtc/quaternion.hpp>
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
   * Describes a single shape instance to be rendered.
   *
   * The system renders the mesh specified by `meshIndex`.
   *
   * The mesh is transformed in the following order:
   *  1. Scaled by `scale`.
   *  2. Rotated using the quaternion derived from `rotation`.
   *  3. Translated by `position`.
   *
   * The resulting shape is then rendered using the material
   * specified by `materialIndex`.
   */
  struct Shape
  {
    /**
     * Index of mesh from field `CmdSetPipeline_ShapeGroup.meshes`
     */
    uint32_t meshIndex;

    /**
     * Moves the mesh to the specified position
     */
    glm::vec3 position;

    /**
     * Scaling along the corresponding axes.
     *
     * `scale.x` - sale along axis Ox.
     *
     * `scale.y` - sale along axis Oy.
     *
     * `scale.z` - sale along axis Oz.
     *
     * `S = 1` - no scaling, `0 < S < 1` - decrease, `S > 1` - increase.
     *
     * where S is one of scale.x, scale.y, scale.z
     */
    glm::vec3 scale;

    /**
     * Rotation vector (Exponential Map) for rotate the shape.
     *
     * The direction of this vector is axis of rotation.
     * The length of this vector - is angle in radians
     *
     * To calculate quaternion:
     * <code>
     * float angle = glm::length(rotationVector);
     * if (angle < 1e-6f)
     *     Q = glm::quat(1,0,0,0);
     * else {
     *     glm::vec3 axis = rotationVector / angle;
     *     Q = glm::angleAxis(angle, axis);
     * }
     * </code>
     *
     * Q - is the quaternion for rotation.
     */
    glm::vec3 rotationVector;

    /**
     * Index of material from field `CmdSetPipeline_ShapeGroup.materials`
     */
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
     * Called once per frame to determine the number of shapes.
     *
     * If the returned value matches the result of the previous call,
     * the existing shape vector and shape buffers are reused.
     *
     * If the value differs, the system reallocates the shape vector and
     * resizes the shape buffers to match the new shape count.
     */
    std::function<size_t()> shapeCountFn;

    /**
     * Called once per frame to generate shapes for rendering.
     *
     * The system maintains a std::vector<Shape> whose size is determined by
     * `this->shapeCountFn`.
     *
     * If the shape count has not changed since the previous frame, the same
     * vector instance is reused and still contains the shape data written
     * during the previous frame.
     *
     * If the shape count changes, the vector is reallocated to match the new
     * shape count.
     *
     * The implementation should update the vector contents as needed.
     * After that, all shapes in the vector will be rendered on the screen.
     */
    std::function<void(std::vector<Shape> &)> populateShapesFn;
  };
} // namespace cmd

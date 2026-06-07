module;

#include <glm/vec3.hpp>

export module generator;
import cmd_pipeline;

export namespace gen {

  void populateWithSphera(cmd::Mesh *, glm::vec3, glm::vec3, float, int, int);

  /**
   * Populates `target` with sphera in the beginning of coordinate system and North Pole up to axis Oz.
   * @param target target to populate
   * @param radius radius of sphera
   * @param latitudeSegments latitude segment count
   * @param longitudeSegments longitude segment count
   */
  void populateWithSphera(cmd::Mesh  *target,           //
                          const float radius,           //
                          const int   latitudeSegments, //
                          const int   longitudeSegments)
  {
    populateWithSphera(target, glm::vec3{0, 0, 0}, glm::vec3{0, 0, 1}, radius, latitudeSegments, longitudeSegments);
  }

  /**
   * Populates `target` with sphera.
   *
   * @param target target to populate
   * @param center center of sphera
   * @param northPoleDirection North Pole direction - this vector may not be 1
   * @param radius radius of spera
   * @param latitudeSegments latitude segment count
   * @param longitudeSegments longitude segment count
   */
  void populateWithSphera(cmd::Mesh  *target,             //
                          glm::vec3   center,             //
                          glm::vec3   northPoleDirection, //
                          const float radius,             //
                          const int   latitudeSegments,   //
                          const int   longitudeSegments)
  {
    // TODO implements this function
  }

} // namespace gen

module;

#include <cmath>
#include <stdexcept>

#include <glm/geometric.hpp>
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
    if (target == nullptr) {
      throw std::invalid_argument("hL2kP8mQnS :: target mesh is null");
    }
    if (radius <= 0.0F) {
      throw std::invalid_argument("rN4vT9cDxW :: sphera radius must be positive");
    }
    if (latitudeSegments < 2) {
      throw std::invalid_argument("kZ6pM1yHaF :: sphera latitudeSegments must be at least 2");
    }
    if (longitudeSegments < 3) {
      throw std::invalid_argument("bC8sQ5jVrL :: sphera longitudeSegments must be at least 3");
    }

    constexpr float pi = 3.14159265358979323846F;

    const float northLength2 = glm::dot(northPoleDirection, northPoleDirection);
    if (northLength2 <= 0.0F) {
      throw std::invalid_argument("mF3xD7uLpE :: sphera northPoleDirection must be non-zero");
    }

    const glm::vec3 zAxis  = glm::normalize(northPoleDirection);
    const glm::vec3 helper = std::abs(zAxis.z) < 0.9F ? glm::vec3{0.0F, 0.0F, 1.0F} : glm::vec3{0.0F, 1.0F, 0.0F};
    const glm::vec3 xAxis  = glm::normalize(glm::cross(helper, zAxis));
    const glm::vec3 yAxis  = glm::normalize(glm::cross(zAxis, xAxis));

    target->points.clear();
    target->triangles.clear();

    target->points.reserve(2 + static_cast<size_t>(latitudeSegments - 1) * static_cast<size_t>(longitudeSegments));
    target->triangles.reserve(static_cast<size_t>(longitudeSegments) * 2U * static_cast<size_t>(latitudeSegments - 1));

    const uint32_t topIndex = 0;
    target->points.push_back(center + zAxis * radius);

    for (int latitude = 1; latitude < latitudeSegments; ++latitude) {
      const float theta    = pi * static_cast<float>(latitude) / static_cast<float>(latitudeSegments);
      const float sinTheta = std::sin(theta);
      const float cosTheta = std::cos(theta);

      for (int longitude = 0; longitude < longitudeSegments; ++longitude) {
        const float     phi    = 2.0F * pi * static_cast<float>(longitude) / static_cast<float>(longitudeSegments);
        const glm::vec3 radial = std::cos(phi) * sinTheta * xAxis + std::sin(phi) * sinTheta * yAxis + cosTheta * zAxis;
        target->points.push_back(center + radius * radial);
      }
    }

    const uint32_t bottomIndex = static_cast<uint32_t>(target->points.size());
    target->points.push_back(center - zAxis * radius);

    const auto ringIndex = [longitudeSegments](const int latitudeRing, const int longitude) {
      const int wrappedLongitude = longitude % longitudeSegments;
      return static_cast<uint32_t>(1 + latitudeRing * longitudeSegments + wrappedLongitude);
    };

    for (int longitude = 0; longitude < longitudeSegments; ++longitude) {
      target->triangles.push_back(cmd::TriangleIdx{
          topIndex,
          ringIndex(0, longitude + 1),
          ringIndex(0, longitude),
      });
    }

    for (int latitudeRing = 0; latitudeRing < latitudeSegments - 2; ++latitudeRing) {
      for (int longitude = 0; longitude < longitudeSegments; ++longitude) {
        const uint32_t upper0 = ringIndex(latitudeRing, longitude);
        const uint32_t upper1 = ringIndex(latitudeRing, longitude + 1);
        const uint32_t lower0 = ringIndex(latitudeRing + 1, longitude);
        const uint32_t lower1 = ringIndex(latitudeRing + 1, longitude + 1);

        target->triangles.push_back(cmd::TriangleIdx{upper0, upper1, lower0});
        target->triangles.push_back(cmd::TriangleIdx{upper1, lower1, lower0});
      }
    }

    const int lastRing = latitudeSegments - 2;
    for (int longitude = 0; longitude < longitudeSegments; ++longitude) {
      target->triangles.push_back(cmd::TriangleIdx{
          ringIndex(lastRing, longitude),
          ringIndex(lastRing, longitude + 1),
          bottomIndex,
      });
    }
  }

} // namespace gen

module;

#include <cmath>
#include <glm/geometric.hpp>
#include <glm/vec3.hpp>
#include <numbers>
#include <stdexcept>

export module generator;
import cmd_pipeline;

constexpr float PI = std::numbers::pi_v<float>;

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
   * @param radius radius of sphera
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

    if (const float northLength2 = glm::dot(northPoleDirection, northPoleDirection); northLength2 <= 0.0F) {
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

    target->points.push_back(center + zAxis * radius);

    for (int latitude = 1; latitude < latitudeSegments; ++latitude) {
      const float theta    = PI * static_cast<float>(latitude) / static_cast<float>(latitudeSegments);
      const float sinTheta = std::sin(theta);
      const float cosTheta = std::cos(theta);

      for (int longitude = 0; longitude < longitudeSegments; ++longitude) {
        const float     phi    = 2.0F * PI * static_cast<float>(longitude) / static_cast<float>(longitudeSegments);
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
          0,
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

  void populateWithCylinder(cmd::Mesh *, glm::vec3, glm::vec3, float, int, int, bool, bool);

  /**
   * Populates `target` with a closed cylinder centered in the beginning of coordinate system and its axis along Oz.
   * @param target target to populate
   * @param radius radius of cylinder
   * @param height full height of cylinder along axis Oz
   * @param radialSegments segment count around the axis
   * @param heightSegments segment count along the axis
   */
  void populateWithCylinder(cmd::Mesh  *target,         //
                            const float radius,         //
                            const float height,         //
                            const int   radialSegments, //
                            const int   heightSegments)
  {
    const float halfHeight = height * 0.5F;
    populateWithCylinder(target, glm::vec3{0, 0, -halfHeight}, glm::vec3{0, 0, halfHeight}, radius, radialSegments, heightSegments, false, false);
  }

  /**
   * Populates `target` with a cylinder spanning between two base centers.
   *
   * The cylinder axis goes from `center1` to `center2`; its height equals the distance
   * between them. Each base can be capped or left open independently.
   *
   * @param target target to populate
   * @param center1 center of the first base
   * @param center2 center of the second base
   * @param radius radius of cylinder
   * @param radialSegments segment count around the axis
   * @param heightSegments segment count along the axis
   * @param open1 if true, the base at `center1` is left open (no cap); if false, it is capped
   * @param open2 if true, the base at `center2` is left open (no cap); if false, it is capped
   */
  void populateWithCylinder(cmd::Mesh  *target,         //
                            glm::vec3   center1,        //
                            glm::vec3   center2,        //
                            const float radius,         //
                            const int   radialSegments, //
                            const int   heightSegments, //
                            const bool  open1,          //
                            const bool  open2)
  {
    if (target == nullptr) {
      throw std::invalid_argument("qD2hN7vKsP :: target mesh is null");
    }
    if (radius <= 0.0F) {
      throw std::invalid_argument("wL5cR1mTxB :: cylinder radius must be positive");
    }
    if (radialSegments < 3) {
      throw std::invalid_argument("tK3sB9yMwL :: cylinder radialSegments must be at least 3");
    }
    if (heightSegments < 1) {
      throw std::invalid_argument("nF6xD2cRpV :: cylinder heightSegments must be at least 1");
    }

    const glm::vec3 axis = center2 - center1;
    if (const float axisLength2 = glm::dot(axis, axis); axisLength2 <= 0.0F) {
      throw std::invalid_argument("jH4mQ8vTaC :: cylinder center1 and center2 must differ");
    }

    const float     height = glm::length(axis);
    const glm::vec3 zAxis  = axis / height;
    const glm::vec3 helper = std::abs(zAxis.z) < 0.9F ? glm::vec3{0.0F, 0.0F, 1.0F} : glm::vec3{0.0F, 1.0F, 0.0F};
    const glm::vec3 xAxis  = glm::normalize(glm::cross(helper, zAxis));
    const glm::vec3 yAxis  = glm::normalize(glm::cross(zAxis, xAxis));

    target->points.clear();
    target->triangles.clear();

    const size_t ringCount = static_cast<size_t>(heightSegments) + 1U;
    const size_t capCount  = static_cast<size_t>(open1 ? 0 : 1) + static_cast<size_t>(open2 ? 0 : 1);
    target->points.reserve(ringCount * static_cast<size_t>(radialSegments) + capCount);
    target->triangles.reserve(static_cast<size_t>(heightSegments) * static_cast<size_t>(radialSegments) * 2U +
                              capCount * static_cast<size_t>(radialSegments));

    // Точки боковой поверхности: (heightSegments + 1) колец по radialSegments точек.
    // Кольцо 0 лежит в основании center1, кольцо heightSegments - в основании center2.
    for (int ring = 0; ring <= heightSegments; ++ring) {
      const float     offset = height * static_cast<float>(ring) / static_cast<float>(heightSegments);
      const glm::vec3 base   = center1 + zAxis * offset;

      for (int radial = 0; radial < radialSegments; ++radial) {
        const float     phi             = 2.0F * PI * static_cast<float>(radial) / static_cast<float>(radialSegments);
        const glm::vec3 radialDirection = std::cos(phi) * xAxis + std::sin(phi) * yAxis;
        target->points.push_back(base + radius * radialDirection);
      }
    }

    // Центральные точки крышек добавляются только если соответствующее основание закрыто.
    uint32_t center2Index = 0;
    if (!open2) {
      center2Index = static_cast<uint32_t>(target->points.size());
      target->points.push_back(center2);
    }
    uint32_t center1Index = 0;
    if (!open1) {
      center1Index = static_cast<uint32_t>(target->points.size());
      target->points.push_back(center1);
    }

    const auto ringIndex = [radialSegments](const int ring, const int radial) {
      const int wrappedRadial = radial % radialSegments;
      return static_cast<uint32_t>(ring * radialSegments + wrappedRadial);
    };

    // Боковая поверхность: два треугольника на каждый сегмент, нормалями наружу.
    for (int ring = 0; ring < heightSegments; ++ring) {
      for (int radial = 0; radial < radialSegments; ++radial) {
        const uint32_t lower0 = ringIndex(ring, radial);
        const uint32_t lower1 = ringIndex(ring, radial + 1);
        const uint32_t upper0 = ringIndex(ring + 1, radial);
        const uint32_t upper1 = ringIndex(ring + 1, radial + 1);

        target->triangles.push_back(cmd::TriangleIdx{lower0, upper0, lower1});
        target->triangles.push_back(cmd::TriangleIdx{lower1, upper0, upper1});
      }
    }

    // Крышка основания center2, нумерация наружу (нормаль вдоль center2 - center1).
    if (!open2) {
      for (int radial = 0; radial < radialSegments; ++radial) {
        target->triangles.push_back(cmd::TriangleIdx{
            center2Index,
            ringIndex(heightSegments, radial + 1),
            ringIndex(heightSegments, radial),
        });
      }
    }

    // Крышка основания center1, нумерация наружу (нормаль вдоль center1 - center2).
    if (!open1) {
      for (int radial = 0; radial < radialSegments; ++radial) {
        target->triangles.push_back(cmd::TriangleIdx{
            center1Index,
            ringIndex(0, radial),
            ringIndex(0, radial + 1),
        });
      }
    }
  }

} // namespace gen

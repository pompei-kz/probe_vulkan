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

  void populateWithCylinder(cmd::Mesh *, glm::vec3, glm::vec3, float, float, int, int);

  /**
   * Populates `target` with a cylinder centered in the beginning of coordinate system and its axis along Oz.
   * @param target target to populate
   * @param radius radius of cylinder
   * @param height full height of cylinder along its axis
   * @param radialSegments segment count around the axis
   * @param heightSegments segment count along the axis
   */
  void populateWithCylinder(cmd::Mesh  *target, //
                            const float radius, //
                            const float height, //
                            const int   radialSegments, //
                            const int   heightSegments)
  {
    populateWithCylinder(target, glm::vec3{0, 0, 0}, glm::vec3{0, 0, 1}, radius, height, radialSegments, heightSegments);
  }

  /**
   * Populates `target` with a cylinder.
   *
   * The cylinder extends symmetrically by `height / 2` in both directions along `axisDirection`
   * from `center`, and is capped on both ends.
   *
   * @param target target to populate
   * @param center center of the cylinder (midpoint of its axis)
   * @param axisDirection direction of the cylinder axis - this vector may not be of length 1
   * @param radius radius of cylinder
   * @param height full height of cylinder along its axis
   * @param radialSegments segment count around the axis
   * @param heightSegments segment count along the axis
   */
  void populateWithCylinder(cmd::Mesh  *target,        //
                            glm::vec3   center,        //
                            glm::vec3   axisDirection, //
                            const float radius,        //
                            const float height,        //
                            const int   radialSegments, //
                            const int   heightSegments)
  {
    if (target == nullptr) {
      throw std::invalid_argument("qD2hN7vKsP :: target mesh is null");
    }
    if (radius <= 0.0F) {
      throw std::invalid_argument("wL5cR1mTxB :: cylinder radius must be positive");
    }
    if (height <= 0.0F) {
      throw std::invalid_argument("zG8pV4nQdH :: cylinder height must be positive");
    }
    if (radialSegments < 3) {
      throw std::invalid_argument("tK3sB9yMwL :: cylinder radialSegments must be at least 3");
    }
    if (heightSegments < 1) {
      throw std::invalid_argument("nF6xD2cRpV :: cylinder heightSegments must be at least 1");
    }

    if (const float axisLength2 = glm::dot(axisDirection, axisDirection); axisLength2 <= 0.0F) {
      throw std::invalid_argument("jH4mQ8vTaC :: cylinder axisDirection must be non-zero");
    }

    const glm::vec3 zAxis  = glm::normalize(axisDirection);
    const glm::vec3 helper = std::abs(zAxis.z) < 0.9F ? glm::vec3{0.0F, 0.0F, 1.0F} : glm::vec3{0.0F, 1.0F, 0.0F};
    const glm::vec3 xAxis  = glm::normalize(glm::cross(helper, zAxis));
    const glm::vec3 yAxis  = glm::normalize(glm::cross(zAxis, xAxis));

    target->points.clear();
    target->triangles.clear();

    const size_t ringCount = static_cast<size_t>(heightSegments) + 1U;
    target->points.reserve(ringCount * static_cast<size_t>(radialSegments) + 2U);
    target->triangles.reserve(static_cast<size_t>(heightSegments) * static_cast<size_t>(radialSegments) * 2U +
                              static_cast<size_t>(radialSegments) * 2U);

    const float halfHeight = height * 0.5F;

    // Точки боковой поверхности: (heightSegments + 1) колец по radialSegments точек.
    for (int ring = 0; ring <= heightSegments; ++ring) {
      const float     offset = -halfHeight + height * static_cast<float>(ring) / static_cast<float>(heightSegments);
      const glm::vec3 base   = center + zAxis * offset;

      for (int radial = 0; radial < radialSegments; ++radial) {
        const float     phi    = 2.0F * PI * static_cast<float>(radial) / static_cast<float>(radialSegments);
        const glm::vec3 radialDirection = std::cos(phi) * xAxis + std::sin(phi) * yAxis;
        target->points.push_back(base + radius * radialDirection);
      }
    }

    const uint32_t topCenterIndex = static_cast<uint32_t>(target->points.size());
    target->points.push_back(center + zAxis * halfHeight);
    const uint32_t bottomCenterIndex = static_cast<uint32_t>(target->points.size());
    target->points.push_back(center - zAxis * halfHeight);

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

    // Верхняя крышка, нумерация наружу (нормаль вдоль +axisDirection).
    for (int radial = 0; radial < radialSegments; ++radial) {
      target->triangles.push_back(cmd::TriangleIdx{
          topCenterIndex,
          ringIndex(heightSegments, radial + 1),
          ringIndex(heightSegments, radial),
      });
    }

    // Нижняя крышка, нумерация наружу (нормаль вдоль -axisDirection).
    for (int radial = 0; radial < radialSegments; ++radial) {
      target->triangles.push_back(cmd::TriangleIdx{
          bottomCenterIndex,
          ringIndex(0, radial),
          ringIndex(0, radial + 1),
      });
    }
  }

} // namespace gen

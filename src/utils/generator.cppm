module;

#include <cmath>
#include <glm/geometric.hpp>
#include <glm/vec3.hpp>
#include <numbers>
#include <stdexcept>
#include <vector>

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

  void populateWithCylinder(cmd::Mesh *, glm::vec3, glm::vec3, float, float, int, int, bool, bool);

  // Радиус, который меньше или равен этому значению, считается нулевым - основание вырождается в вершину (конус).
  constexpr float CYLINDER_RADIUS_EPSILON = 1e-6F;

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
    populateWithCylinder(target, glm::vec3{0, 0, -halfHeight}, glm::vec3{0, 0, halfHeight}, radius, radius, radialSegments, heightSegments, false, false);
  }

  /**
   * Populates `target` with a cylinder / truncated cone spanning between two base centers.
   *
   * The axis goes from `center1` to `center2`; its height equals the distance between them.
   * The radius is interpolated linearly along the axis from `radius1` at `center1` to `radius2`
   * at `center2`. If one of the radii is zero (within `CYLINDER_RADIUS_EPSILON`), that base
   * collapses to a single apex vertex and the result is a cone (such a base never has a cap).
   * Each non-degenerate base can be capped or left open independently.
   *
   * @param target target to populate
   * @param center1 center of the first base
   * @param center2 center of the second base
   * @param radius1 radius of the base at `center1` (zero means an apex at `center1`)
   * @param radius2 radius of the base at `center2` (zero means an apex at `center2`)
   * @param radialSegments segment count around the axis
   * @param heightSegments segment count along the axis
   * @param open1 if true, the base at `center1` is left open (no cap); if false, it is capped
   * @param open2 if true, the base at `center2` is left open (no cap); if false, it is capped
   */
  void populateWithCylinder(cmd::Mesh  *target,         //
                            glm::vec3   center1,        //
                            glm::vec3   center2,        //
                            const float radius1,        //
                            const float radius2,        //
                            const int   radialSegments, //
                            const int   heightSegments, //
                            const bool  open1,          //
                            const bool  open2)
  {
    if (target == nullptr) {
      throw std::invalid_argument("qD2hN7vKsP :: target mesh is null");
    }
    if (radius1 < 0.0F) {
      throw std::invalid_argument("wL5cR1mTxB :: cylinder radius1 must be non-negative");
    }
    if (radius2 < 0.0F) {
      throw std::invalid_argument("sV9yB3nKqW :: cylinder radius2 must be non-negative");
    }
    if (radialSegments < 3) {
      throw std::invalid_argument("tK3sB9yMwL :: cylinder radialSegments must be at least 3");
    }
    if (heightSegments < 1) {
      throw std::invalid_argument("nF6xD2cRpV :: cylinder heightSegments must be at least 1");
    }

    const bool apex1 = radius1 <= CYLINDER_RADIUS_EPSILON;
    const bool apex2 = radius2 <= CYLINDER_RADIUS_EPSILON;
    if (apex1 && apex2) {
      throw std::invalid_argument("pH7cM4vTxR :: cylinder radius1 and radius2 must not both be zero");
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

    // Является ли кольцо вырожденным в вершину (только крайние кольца могут быть таковыми).
    const auto ringIsApex = [apex1, apex2, heightSegments](const int ring) {
      return (ring == 0 && apex1) || (ring == heightSegments && apex2);
    };

    const bool cap1 = !open1 && !apex1;
    const bool cap2 = !open2 && !apex2;

    target->points.clear();
    target->triangles.clear();

    // Точное число точек: сумма размеров колец (вершина - 1 точка, иначе radialSegments) плюс центры крышек.
    size_t pointCount = 0;
    for (int ring = 0; ring <= heightSegments; ++ring) {
      pointCount += ringIsApex(ring) ? 1U : static_cast<size_t>(radialSegments);
    }
    pointCount += static_cast<size_t>(cap1 ? 1 : 0) + static_cast<size_t>(cap2 ? 1 : 0);

    target->points.reserve(pointCount);
    target->triangles.reserve(static_cast<size_t>(heightSegments) * static_cast<size_t>(radialSegments) * 2U +
                              static_cast<size_t>(cap1 ? radialSegments : 0) + static_cast<size_t>(cap2 ? radialSegments : 0));

    // Точки колец: кольцо 0 в основании center1, кольцо heightSegments - в основании center2.
    // Радиус интерполируется линейно вдоль оси. Вырожденное кольцо хранит одну точку - вершину.
    std::vector<uint32_t> ringStart(static_cast<size_t>(heightSegments) + 1U);
    for (int ring = 0; ring <= heightSegments; ++ring) {
      ringStart[ring] = static_cast<uint32_t>(target->points.size());

      const float     t      = static_cast<float>(ring) / static_cast<float>(heightSegments);
      const float     ringRadius = radius1 + (radius2 - radius1) * t;
      const glm::vec3 base   = center1 + zAxis * (height * t);

      if (ringIsApex(ring)) {
        target->points.push_back(base);
        continue;
      }

      for (int radial = 0; radial < radialSegments; ++radial) {
        const float     phi             = 2.0F * PI * static_cast<float>(radial) / static_cast<float>(radialSegments);
        const glm::vec3 radialDirection = std::cos(phi) * xAxis + std::sin(phi) * yAxis;
        target->points.push_back(base + ringRadius * radialDirection);
      }
    }

    // Центральные точки крышек добавляются только для закрытых невырожденных оснований.
    uint32_t center2Index = 0;
    if (cap2) {
      center2Index = static_cast<uint32_t>(target->points.size());
      target->points.push_back(center2);
    }
    uint32_t center1Index = 0;
    if (cap1) {
      center1Index = static_cast<uint32_t>(target->points.size());
      target->points.push_back(center1);
    }

    const auto ringPoint = [&ringStart, &ringIsApex, radialSegments](const int ring, const int radial) {
      if (ringIsApex(ring)) {
        return ringStart[ring];
      }
      return ringStart[ring] + static_cast<uint32_t>(radial % radialSegments);
    };

    // Добавляет треугольник, пропуская вырожденные (с совпадающими вершинами у апекса).
    const auto pushTriangle = [target](const uint32_t a, const uint32_t b, const uint32_t c) {
      if (a != b && b != c && a != c) {
        target->triangles.push_back(cmd::TriangleIdx{a, b, c});
      }
    };

    // Боковая поверхность: два треугольника на сегмент, нормалями наружу.
    // У вырожденного кольца совпадающие вершины автоматически отбрасываются, образуя веер конуса.
    for (int ring = 0; ring < heightSegments; ++ring) {
      for (int radial = 0; radial < radialSegments; ++radial) {
        const uint32_t lower0 = ringPoint(ring, radial);
        const uint32_t lower1 = ringPoint(ring, radial + 1);
        const uint32_t upper0 = ringPoint(ring + 1, radial);
        const uint32_t upper1 = ringPoint(ring + 1, radial + 1);

        pushTriangle(lower0, upper0, lower1);
        pushTriangle(lower1, upper0, upper1);
      }
    }

    // Крышка основания center2, нумерация наружу (нормаль вдоль center2 - center1).
    if (cap2) {
      for (int radial = 0; radial < radialSegments; ++radial) {
        target->triangles.push_back(cmd::TriangleIdx{
            center2Index,
            ringPoint(heightSegments, radial + 1),
            ringPoint(heightSegments, radial),
        });
      }
    }

    // Крышка основания center1, нумерация наружу (нормаль вдоль center1 - center2).
    if (cap1) {
      for (int radial = 0; radial < radialSegments; ++radial) {
        target->triangles.push_back(cmd::TriangleIdx{
            center1Index,
            ringPoint(0, radial),
            ringPoint(0, radial + 1),
        });
      }
    }
  }

} // namespace gen

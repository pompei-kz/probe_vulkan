#include <cmath>

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>
#include <gtest/gtest.h>

import generator;
import cmd_pipeline;

namespace {

  // Expected vertex count: two poles + (latitudeSegments - 1) rings of longitudeSegments points.
  size_t expectedPointCount(const int latitudeSegments, const int longitudeSegments)
  {
    return 2 + static_cast<size_t>(latitudeSegments - 1) * static_cast<size_t>(longitudeSegments);
  }

  // Expected triangle count: 2 * longitudeSegments * (latitudeSegments - 1).
  size_t expectedTriangleCount(const int latitudeSegments, const int longitudeSegments)
  {
    return static_cast<size_t>(longitudeSegments) * 2U * static_cast<size_t>(latitudeSegments - 1);
  }

} // namespace

// ---------------------------------------------------------------------------
// Topology: the multi-argument overload must produce the documented counts.
// ---------------------------------------------------------------------------
TEST(PopulateWithSpheraFull, ProducesExpectedVertexAndTriangleCounts)
{
  constexpr int latitudeSegments  = 8;
  constexpr int longitudeSegments = 12;

  cmd::Mesh mesh;

  //
  //
  gen::populateWithSphera(&mesh, glm::vec3{1, 2, 3}, glm::vec3{0, 0, 1}, 2.0F, latitudeSegments, longitudeSegments);
  //
  //

  EXPECT_EQ(mesh.points.size(), expectedPointCount(latitudeSegments, longitudeSegments));
  EXPECT_EQ(mesh.triangles.size(), expectedTriangleCount(latitudeSegments, longitudeSegments));
}

// ---------------------------------------------------------------------------
// Every generated triangle index must reference a valid point.
// ---------------------------------------------------------------------------
TEST(PopulateWithSpheraFull, AllTriangleIndicesAreInRange)
{
  cmd::Mesh mesh;

  //
  //
  gen::populateWithSphera(&mesh, glm::vec3{0, 0, 0}, glm::vec3{0, 0, 1}, 1.0F, 6, 9);
  //
  //

  const auto pointCount = static_cast<uint32_t>(mesh.points.size());
  for (const auto &[index0, index1, index2] : mesh.triangles) {
    EXPECT_LT(index0, pointCount);
    EXPECT_LT(index1, pointCount);
    EXPECT_LT(index2, pointCount);
  }
}

// ---------------------------------------------------------------------------
// Geometry: every point lies on the sphere surface around the given center.
// ---------------------------------------------------------------------------
TEST(PopulateWithSpheraFull, AllPointsLieOnSphereSurface)
{
  constexpr glm::vec3 center{-4.0F, 5.0F, 6.5F};
  constexpr float     radius = 3.25F;

  cmd::Mesh mesh;

  //
  //
  gen::populateWithSphera(&mesh, center, glm::vec3{0, 0, 1}, radius, 10, 14);
  //
  //

  for (const glm::vec3 &point : mesh.points) {
    EXPECT_NEAR(glm::length(point - center), radius, 1e-4F);
  }
}

// ---------------------------------------------------------------------------
// The poles must align with the (normalized) north-pole direction.
// ---------------------------------------------------------------------------
TEST(PopulateWithSpheraFull, PolesFollowNorthPoleDirection)
{
  constexpr glm::vec3 center{1.0F, 1.0F, 1.0F};
  constexpr glm::vec3 northDir{0.0F, 1.0F, 0.0F};
  constexpr float     radius = 2.0F;

  cmd::Mesh mesh;

  //
  //
  gen::populateWithSphera(&mesh, center, northDir, radius, 5, 7);
  //
  //

  const glm::vec3 axis        = glm::normalize(northDir);
  const glm::vec3 expectedTop = center + axis * radius;
  const glm::vec3 expectedBot = center - axis * radius;

  // First point is the North Pole, last point is the South Pole.
  const glm::vec3 &top = mesh.points.front();
  const glm::vec3 &bot = mesh.points.back();

  EXPECT_NEAR(top.x, expectedTop.x, 1e-4F);
  EXPECT_NEAR(top.y, expectedTop.y, 1e-4F);
  EXPECT_NEAR(top.z, expectedTop.z, 1e-4F);

  EXPECT_NEAR(bot.x, expectedBot.x, 1e-4F);
  EXPECT_NEAR(bot.y, expectedBot.y, 1e-4F);
  EXPECT_NEAR(bot.z, expectedBot.z, 1e-4F);
}

// ---------------------------------------------------------------------------
// A non-unit north-pole direction must be normalized internally so the
// resulting radius is unaffected by the direction's magnitude.
// ---------------------------------------------------------------------------
TEST(PopulateWithSpheraFull, NonUnitNorthPoleDirectionIsNormalized)
{
  constexpr glm::vec3 center{0, 0, 0};
  constexpr glm::vec3 longNorthDir{0.0F, 0.0F, 17.0F}; // length 17, not 1
  constexpr float     radius = 4.0F;

  cmd::Mesh mesh;

  //
  //
  gen::populateWithSphera(&mesh, center, longNorthDir, radius, 6, 6);
  //
  //

  for (const glm::vec3 &point : mesh.points) {
    EXPECT_NEAR(glm::length(point - center), radius, 1e-4F);
  }
}

// ---------------------------------------------------------------------------
// Re-populating an existing mesh must clear previous contents (no accumulation).
// ---------------------------------------------------------------------------
TEST(PopulateWithSpheraFull, ReusingMeshClearsPreviousData)
{
  cmd::Mesh mesh;

  //
  //
  gen::populateWithSphera(&mesh, glm::vec3{0, 0, 0}, glm::vec3{0, 0, 1}, 1.0F, 12, 16);
  gen::populateWithSphera(&mesh, glm::vec3{0, 0, 0}, glm::vec3{0, 0, 1}, 1.0F, 4, 5);
  //
  //

  EXPECT_EQ(mesh.points.size(), expectedPointCount(4, 5));
  EXPECT_EQ(mesh.triangles.size(), expectedTriangleCount(4, 5));
}

// ---------------------------------------------------------------------------
// Argument validation: each invalid input throws std::invalid_argument.
// ---------------------------------------------------------------------------
TEST(PopulateWithSpheraFull, NullTargetThrows)
{
  //
  //
  EXPECT_THROW(gen::populateWithSphera(nullptr, glm::vec3{0, 0, 0}, glm::vec3{0, 0, 1}, 1.0F, 4, 4), std::invalid_argument);
  //
  //
}

TEST(PopulateWithSpheraFull, NonPositiveRadiusThrows)
{
  cmd::Mesh mesh;

  //
  //
  EXPECT_THROW(gen::populateWithSphera(&mesh, glm::vec3{0, 0, 0}, glm::vec3{0, 0, 1}, 0.0F, 4, 4), std::invalid_argument);
  EXPECT_THROW(gen::populateWithSphera(&mesh, glm::vec3{0, 0, 0}, glm::vec3{0, 0, 1}, -1.0F, 4, 4), std::invalid_argument);
  //
  //
}

TEST(PopulateWithSpheraFull, TooFewLatitudeSegmentsThrows)
{
  cmd::Mesh mesh;

  //
  //
  EXPECT_THROW(gen::populateWithSphera(&mesh, glm::vec3{0, 0, 0}, glm::vec3{0, 0, 1}, 1.0F, 1, 4), std::invalid_argument);
  //
  //
}

TEST(PopulateWithSpheraFull, TooFewLongitudeSegmentsThrows)
{
  cmd::Mesh mesh;

  //
  //
  EXPECT_THROW(gen::populateWithSphera(&mesh, glm::vec3{0, 0, 0}, glm::vec3{0, 0, 1}, 1.0F, 4, 2), std::invalid_argument);
  //
  //
}

TEST(PopulateWithSpheraFull, ZeroNorthPoleDirectionThrows)
{
  cmd::Mesh mesh;

  //
  //
  EXPECT_THROW(gen::populateWithSphera(&mesh, glm::vec3{0, 0, 0}, glm::vec3{0, 0, 0}, 1.0F, 4, 4), std::invalid_argument);
  //
  //
}

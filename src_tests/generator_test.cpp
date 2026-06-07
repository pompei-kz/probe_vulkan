#include <cmath>
#include <vector>

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

// ===========================================================================
// populateWithCylinder
// ===========================================================================

namespace {

  constexpr float CYL_EPS = 1e-6F;

  // Expected vertex count: each ring has radialSegments points, except a zero-radius end
  // ring that collapses to a single apex vertex; plus one center vertex per closed, non-apex base.
  size_t expectedCylinderPointCount(const float radius1,
                                    const float radius2,
                                    const int   radialSegments,
                                    const int   heightSegments,
                                    const bool  open1,
                                    const bool  open2)
  {
    const bool apex1 = radius1 <= CYL_EPS;
    const bool apex2 = radius2 <= CYL_EPS;

    size_t points = 0;
    for (int ring = 0; ring <= heightSegments; ++ring) {
      const bool apexRing = (ring == 0 && apex1) || (ring == heightSegments && apex2);
      points += apexRing ? 1U : static_cast<size_t>(radialSegments);
    }
    points += static_cast<size_t>(!open1 && !apex1 ? 1 : 0) + static_cast<size_t>(!open2 && !apex2 ? 1 : 0);
    return points;
  }

  // Expected triangle count: 2 per side segment (a segment touching an apex contributes only a
  // single fan of radialSegments triangles), plus one fan per closed, non-apex base.
  size_t expectedCylinderTriangleCount(const float radius1,
                                       const float radius2,
                                       const int   radialSegments,
                                       const int   heightSegments,
                                       const bool  open1,
                                       const bool  open2)
  {
    const bool apex1 = radius1 <= CYL_EPS;
    const bool apex2 = radius2 <= CYL_EPS;

    size_t side = 0;
    for (int ring = 0; ring < heightSegments; ++ring) {
      const bool touchesApex = (ring == 0 && apex1) || (ring + 1 == heightSegments && apex2);
      side += touchesApex ? static_cast<size_t>(radialSegments) : static_cast<size_t>(radialSegments) * 2U;
    }

    const size_t caps = static_cast<size_t>(!open1 && !apex1 ? radialSegments : 0) //
                      + static_cast<size_t>(!open2 && !apex2 ? radialSegments : 0);
    return side + caps;
  }

  // Perpendicular distance from a point to the cylinder axis line (center1 -> center2).
  float distanceToAxis(const glm::vec3 &point, const glm::vec3 &center1, const glm::vec3 &center2)
  {
    const glm::vec3 axis      = glm::normalize(center2 - center1);
    const glm::vec3 relative  = point - center1;
    const float     projected = glm::dot(relative, axis);
    return glm::length(relative - projected * axis);
  }

  // Signed distance of a point along the axis, measured from center1 (0 at center1, height at center2).
  float positionAlongAxis(const glm::vec3 &point, const glm::vec3 &center1, const glm::vec3 &center2)
  {
    return glm::dot(point - center1, glm::normalize(center2 - center1));
  }

  bool containsPoint(const std::vector<glm::vec3> &points, const glm::vec3 &query, const float epsilon = 1e-4F)
  {
    for (const glm::vec3 &point : points) {
      if (glm::length(point - query) < epsilon) return true;
    }
    return false;
  }

} // namespace

// ---------------------------------------------------------------------------
// Topology: closed cylinder produces the documented counts.
// ---------------------------------------------------------------------------
TEST(PopulateWithCylinderFull, ClosedCylinderProducesExpectedCounts)
{
  constexpr int   radialSegments = 12;
  constexpr int   heightSegments = 4;
  constexpr float radius         = 2.0F;
  constexpr bool  open1          = false;
  constexpr bool  open2          = false;

  cmd::Mesh mesh;

  //
  //
  gen::populateWithCylinder(&mesh, glm::vec3{0, 0, 0}, glm::vec3{0, 0, 5}, radius, radius, radialSegments, heightSegments, open1, open2);
  //
  //

  EXPECT_EQ(mesh.points.size(), expectedCylinderPointCount(radius, radius, radialSegments, heightSegments, open1, open2));
  EXPECT_EQ(mesh.triangles.size(), expectedCylinderTriangleCount(radius, radius, radialSegments, heightSegments, open1, open2));
}

// ---------------------------------------------------------------------------
// open1 / open2 remove the corresponding cap (one center vertex + one fan each).
// ---------------------------------------------------------------------------
TEST(PopulateWithCylinderFull, OpenFlagsRemoveCaps)
{
  constexpr int radialSegments = 8;
  constexpr int heightSegments = 3;

  const glm::vec3 c1{0, 0, 0};
  const glm::vec3 c2{0, 0, 4};

  cmd::Mesh bothOpen;
  cmd::Mesh oneOpen;
  cmd::Mesh closed;

  //
  //
  gen::populateWithCylinder(&bothOpen, c1, c2, 1.0F, 1.0F, radialSegments, heightSegments, true, true);
  gen::populateWithCylinder(&oneOpen, c1, c2, 1.0F, 1.0F, radialSegments, heightSegments, true, false);
  gen::populateWithCylinder(&closed, c1, c2, 1.0F, 1.0F, radialSegments, heightSegments, false, false);
  //
  //

  EXPECT_EQ(bothOpen.points.size(), expectedCylinderPointCount(1.0F, 1.0F, radialSegments, heightSegments, true, true));
  EXPECT_EQ(bothOpen.triangles.size(), expectedCylinderTriangleCount(1.0F, 1.0F, radialSegments, heightSegments, true, true));

  EXPECT_EQ(oneOpen.points.size(), expectedCylinderPointCount(1.0F, 1.0F, radialSegments, heightSegments, true, false));
  EXPECT_EQ(oneOpen.triangles.size(), expectedCylinderTriangleCount(1.0F, 1.0F, radialSegments, heightSegments, true, false));

  // Each closed cap adds exactly radialSegments triangles compared with a fully open tube.
  EXPECT_EQ(closed.triangles.size(), bothOpen.triangles.size() + 2U * static_cast<size_t>(radialSegments));
}

// ---------------------------------------------------------------------------
// Geometry: with both ends open, every vertex lies on the side surface
// (distance == radius from the axis) and between the two base planes.
// ---------------------------------------------------------------------------
TEST(PopulateWithCylinderFull, OpenTubePointsLieOnSideSurface)
{
  const glm::vec3 c1{1.0F, 2.0F, 3.0F};
  const glm::vec3 c2{1.0F, 2.0F, 9.0F};
  constexpr float radius = 2.5F;
  const float     height = glm::length(c2 - c1);

  cmd::Mesh mesh;

  //
  //
  gen::populateWithCylinder(&mesh, c1, c2, radius, radius, 16, 5, true, true);
  //
  //

  for (const glm::vec3 &point : mesh.points) {
    EXPECT_NEAR(distanceToAxis(point, c1, c2), radius, 1e-4F);

    const float along = positionAlongAxis(point, c1, c2);
    EXPECT_GE(along, -1e-4F);
    EXPECT_LE(along, height + 1e-4F);
  }
}

// ---------------------------------------------------------------------------
// Closed caps add center vertices placed exactly at center1 and center2.
// ---------------------------------------------------------------------------
TEST(PopulateWithCylinderFull, ClosedCapsAddBaseCenters)
{
  const glm::vec3 c1{-1.0F, 0.0F, 0.0F};
  const glm::vec3 c2{4.0F, 0.0F, 0.0F};

  cmd::Mesh mesh;

  //
  //
  gen::populateWithCylinder(&mesh, c1, c2, 1.5F, 1.5F, 10, 2, false, false);
  //
  //

  EXPECT_TRUE(containsPoint(mesh.points, c1));
  EXPECT_TRUE(containsPoint(mesh.points, c2));
}

// ---------------------------------------------------------------------------
// An open base must NOT introduce its center vertex.
// ---------------------------------------------------------------------------
TEST(PopulateWithCylinderFull, OpenBaseHasNoCenterVertex)
{
  const glm::vec3 c1{0.0F, 0.0F, 0.0F};
  const glm::vec3 c2{0.0F, 0.0F, 6.0F};

  cmd::Mesh mesh;

  //
  //
  gen::populateWithCylinder(&mesh, c1, c2, 1.0F, 1.0F, 12, 3, true, false);
  //
  //

  // center1 is open -> no vertex on the axis there; center2 is closed -> present.
  EXPECT_FALSE(containsPoint(mesh.points, c1));
  EXPECT_TRUE(containsPoint(mesh.points, c2));
}

// ---------------------------------------------------------------------------
// Every generated triangle index must reference a valid point.
// ---------------------------------------------------------------------------
TEST(PopulateWithCylinderFull, AllTriangleIndicesAreInRange)
{
  cmd::Mesh mesh;

  //
  //
  gen::populateWithCylinder(&mesh, glm::vec3{0, 0, 0}, glm::vec3{2, 0, 0}, 1.0F, 1.0F, 9, 4, false, false);
  //
  //

  const auto pointCount = static_cast<uint32_t>(mesh.points.size());
  for (const cmd::TriangleIdx &tri : mesh.triangles) {
    EXPECT_LT(tri.index0, pointCount);
    EXPECT_LT(tri.index1, pointCount);
    EXPECT_LT(tri.index2, pointCount);
  }
}

// ---------------------------------------------------------------------------
// Convenience overload: closed cylinder centered at the origin, axis along Oz.
// ---------------------------------------------------------------------------
TEST(PopulateWithCylinderSimple, ClosedAlongOzCenteredAtOrigin)
{
  constexpr int   radialSegments = 10;
  constexpr int   heightSegments = 2;
  constexpr float height         = 4.0F;

  cmd::Mesh mesh;

  //
  //
  gen::populateWithCylinder(&mesh, 1.0F, height, radialSegments, heightSegments);
  //
  //

  EXPECT_EQ(mesh.points.size(), expectedCylinderPointCount(1.0F, 1.0F, radialSegments, heightSegments, false, false));
  EXPECT_EQ(mesh.triangles.size(), expectedCylinderTriangleCount(1.0F, 1.0F, radialSegments, heightSegments, false, false));

  // Both caps are closed, so their center vertices sit on the Oz axis at +/- height/2.
  EXPECT_TRUE(containsPoint(mesh.points, glm::vec3{0, 0, -height * 0.5F}));
  EXPECT_TRUE(containsPoint(mesh.points, glm::vec3{0, 0, height * 0.5F}));
}

// ---------------------------------------------------------------------------
// Re-populating an existing mesh must clear previous contents (no accumulation).
// ---------------------------------------------------------------------------
TEST(PopulateWithCylinderFull, ReusingMeshClearsPreviousData)
{
  cmd::Mesh mesh;
  gen::populateWithCylinder(&mesh, glm::vec3{0, 0, 0}, glm::vec3{0, 0, 8}, 1.0F, 1.0F, 20, 6, false, false);
  gen::populateWithCylinder(&mesh, glm::vec3{0, 0, 0}, glm::vec3{0, 0, 2}, 1.0F, 1.0F, 5, 1, true, true);

  EXPECT_EQ(mesh.points.size(), expectedCylinderPointCount(1.0F, 1.0F, 5, 1, true, true));
  EXPECT_EQ(mesh.triangles.size(), expectedCylinderTriangleCount(1.0F, 1.0F, 5, 1, true, true));
}

// ---------------------------------------------------------------------------
// Argument validation: each invalid input throws std::invalid_argument.
// ---------------------------------------------------------------------------
TEST(PopulateWithCylinderFull, NullTargetThrows)
{
  //
  //
  EXPECT_THROW(gen::populateWithCylinder(nullptr, glm::vec3{0, 0, 0}, glm::vec3{0, 0, 1}, 1.0F, 1.0F, 8, 2, false, false), std::invalid_argument);
  //
  //
}

TEST(PopulateWithCylinderFull, NegativeRadiusThrows)
{
  cmd::Mesh mesh;

  //
  //
  EXPECT_THROW(gen::populateWithCylinder(&mesh, glm::vec3{0, 0, 0}, glm::vec3{0, 0, 1}, -1.0F, 1.0F, 8, 2, false, false), std::invalid_argument);
  EXPECT_THROW(gen::populateWithCylinder(&mesh, glm::vec3{0, 0, 0}, glm::vec3{0, 0, 1}, 1.0F, -1.0F, 8, 2, false, false), std::invalid_argument);
  //
  //
}

TEST(PopulateWithCylinderFull, BothRadiiZeroThrows)
{
  cmd::Mesh mesh;

  //
  //
  EXPECT_THROW(gen::populateWithCylinder(&mesh, glm::vec3{0, 0, 0}, glm::vec3{0, 0, 1}, 0.0F, 0.0F, 8, 2, false, false), std::invalid_argument);
  //
  //
}

TEST(PopulateWithCylinderFull, TooFewRadialSegmentsThrows)
{
  cmd::Mesh mesh;

  //
  //
  EXPECT_THROW(gen::populateWithCylinder(&mesh, glm::vec3{0, 0, 0}, glm::vec3{0, 0, 1}, 1.0F, 1.0F, 2, 2, false, false), std::invalid_argument);
  //
  //
}

TEST(PopulateWithCylinderFull, TooFewHeightSegmentsThrows)
{
  cmd::Mesh mesh;

  //
  //
  EXPECT_THROW(gen::populateWithCylinder(&mesh, glm::vec3{0, 0, 0}, glm::vec3{0, 0, 1}, 1.0F, 1.0F, 8, 0, false, false), std::invalid_argument);
  //
  //
}

TEST(PopulateWithCylinderFull, EqualBaseCentersThrow)
{
  cmd::Mesh mesh;

  //
  //
  EXPECT_THROW(gen::populateWithCylinder(&mesh, glm::vec3{1, 2, 3}, glm::vec3{1, 2, 3}, 1.0F, 1.0F, 8, 2, false, false), std::invalid_argument);
  //
  //
}

// ===========================================================================
// populateWithCylinder - cone behavior (one radius is zero)
// ===========================================================================

// ---------------------------------------------------------------------------
// A zero radius collapses that base to an apex vertex; counts follow the apex rule.
// ---------------------------------------------------------------------------
TEST(PopulateWithCylinderCone, ZeroRadius1ProducesApexCounts)
{
  constexpr int radialSegments = 12;
  constexpr int heightSegments = 3;

  const glm::vec3 c1{0, 0, 0};
  const glm::vec3 c2{0, 0, 5};

  cmd::Mesh mesh;

  //
  //
  gen::populateWithCylinder(&mesh, c1, c2, 0.0F, 2.0F, radialSegments, heightSegments, false, false);
  //
  //

  EXPECT_EQ(mesh.points.size(), expectedCylinderPointCount(0.0F, 2.0F, radialSegments, heightSegments, false, false));
  EXPECT_EQ(mesh.triangles.size(), expectedCylinderTriangleCount(0.0F, 2.0F, radialSegments, heightSegments, false, false));

  // The apex sits exactly at center1; the opposite (closed) base keeps its cap center.
  EXPECT_TRUE(containsPoint(mesh.points, c1));
  EXPECT_TRUE(containsPoint(mesh.points, c2));
}

// ---------------------------------------------------------------------------
// An apex base is never capped, so its open flag has no effect on the result.
// ---------------------------------------------------------------------------
TEST(PopulateWithCylinderCone, ApexEndIgnoresOpenFlag)
{
  const glm::vec3 c1{0, 0, 0};
  const glm::vec3 c2{0, 0, 4};

  cmd::Mesh apexClosed;
  cmd::Mesh apexOpen;

  //
  //
  gen::populateWithCylinder(&apexClosed, c1, c2, 0.0F, 2.0F, 10, 2, false, false);
  gen::populateWithCylinder(&apexOpen, c1, c2, 0.0F, 2.0F, 10, 2, true, false);
  //
  //

  EXPECT_EQ(apexClosed.points.size(), apexOpen.points.size());
  EXPECT_EQ(apexClosed.triangles.size(), apexOpen.triangles.size());
}

// ---------------------------------------------------------------------------
// A cone must not contain degenerate triangles, and every index must be valid.
// ---------------------------------------------------------------------------
TEST(PopulateWithCylinderCone, NoDegenerateTrianglesAndIndicesInRange)
{
  cmd::Mesh mesh;

  //
  //
  gen::populateWithCylinder(&mesh, glm::vec3{0, 0, 0}, glm::vec3{0, 0, 3}, 2.0F, 0.0F, 9, 4, false, false);
  //
  //

  const auto pointCount = static_cast<uint32_t>(mesh.points.size());
  for (const cmd::TriangleIdx &tri : mesh.triangles) {
    EXPECT_LT(tri.index0, pointCount);
    EXPECT_LT(tri.index1, pointCount);
    EXPECT_LT(tri.index2, pointCount);
    EXPECT_NE(tri.index0, tri.index1);
    EXPECT_NE(tri.index1, tri.index2);
    EXPECT_NE(tri.index0, tri.index2);
  }

  // Apex at center2; base center1 is closed.
  EXPECT_TRUE(containsPoint(mesh.points, glm::vec3{0, 0, 3}));
}

// ---------------------------------------------------------------------------
// A near-zero radius (within epsilon) is also treated as an apex.
// ---------------------------------------------------------------------------
TEST(PopulateWithCylinderCone, NearZeroRadiusIsApex)
{
  constexpr int radialSegments = 8;
  constexpr int heightSegments = 2;

  const glm::vec3 c1{0, 0, 0};
  const glm::vec3 c2{0, 0, 4};

  cmd::Mesh mesh;

  //
  //
  gen::populateWithCylinder(&mesh, c1, c2, 1e-7F, 1.0F, radialSegments, heightSegments, false, false);
  //
  //

  EXPECT_EQ(mesh.points.size(), expectedCylinderPointCount(0.0F, 1.0F, radialSegments, heightSegments, false, false));
  EXPECT_EQ(mesh.triangles.size(), expectedCylinderTriangleCount(0.0F, 1.0F, radialSegments, heightSegments, false, false));
}

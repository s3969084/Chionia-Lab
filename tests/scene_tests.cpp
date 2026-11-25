#include "core/commands/scene_command_queue.hpp"
#include "graphics/conversion/point_vertex_converter.hpp"
#include "lesson/solar_system_lesson.hpp"
#include "scene/procedural/infinity_generator.hpp"
#include "scene/scene.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

    void require(bool condition, const std::string& message) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    }

    void testSceneUpsertAndTransform() {
        chionia::Scene scene;

        chionia::PointCloud initialCloud("first cloud");
        initialCloud.addPoint(glm::vec3(1.0f, 2.0f, 3.0f));

        const chionia::SceneObjectId objectId = scene.upsertPointCloud(
            "procedural/infinity",
            std::move(initialCloud)
        );

        require(scene.getObjectCount() == 1, "Scene should contain one object.");

        const chionia::SceneObject* object = scene.findObject(objectId);
        require(object != nullptr, "Inserted object should be findable by ID.");
        require(
            object->getPointCloud().getPointCount() == 1,
            "Inserted point cloud should be retained."
        );

        chionia::Transform transform;
        transform.setPosition(glm::vec3(4.0f, -2.0f, 1.5f));
        transform.setRotation(glm::vec3(0.0f, 30.0f, 0.0f));
        require(
            scene.setTransform(objectId, transform),
            "Existing scene object should accept a transform."
        );

        chionia::PointCloud replacementCloud("replacement cloud");
        replacementCloud.addPoint(glm::vec3(-1.0f, 0.0f, 2.0f));
        replacementCloud.addPoint(glm::vec3(1.0f, 0.0f, 2.0f));

        const chionia::SceneObjectId replacementId = scene.upsertPointCloud(
            "procedural/infinity",
            std::move(replacementCloud)
        );

        require(
            replacementId == objectId,
            "Upserting the same key should preserve object identity."
        );

        object = scene.findObject(objectId);
        require(object != nullptr, "Upserted object should remain findable.");
        require(
            object->getPointCloud().getPointCount() == 2,
            "Upsert should replace the object's point cloud."
        );
        require(
            std::abs(object->getTransform().getPosition().x - 4.0f) < 0.0001f,
            "Upsert should preserve the existing transform."
        );
        require(
            std::abs(object->getTransform().getRotation().y - 30.0f) < 0.0001f,
            "Upsert should preserve object rotation."
        );
    }

    void testSceneRemovalAndLookup() {
        chionia::Scene scene;

        chionia::PointCloud cloud("removable");
        const chionia::SceneObjectId objectId = scene.upsertPointCloud(
            "asset/removable",
            std::move(cloud)
        );

        require(
            scene.findObjectByKey("asset/removable") == objectId,
            "Scene should look up objects by key."
        );
        require(scene.removeObject(objectId), "Existing object should be removed.");
        require(scene.getObjectCount() == 0, "Removed object should leave the scene empty.");
        require(
            !scene.findObjectByKey("asset/removable").has_value(),
            "Removed object key should no longer resolve."
        );
        require(
            !scene.removeObject(objectId),
            "Removing a missing object should report no change."
        );
    }

    void testSceneCommandQueueFifo() {
        chionia::SceneCommandQueue queue;

        chionia::PointCloud cloud("queued");
        cloud.addPoint(glm::vec3(0.0f));
        queue.enqueue(chionia::UpsertPointCloudCommand{
            "queued/object",
            std::move(cloud)
        });
        queue.enqueue(chionia::RemoveSceneObjectCommand{17});

        auto first = queue.tryDequeue();
        require(first.has_value(), "Queue should return its first command.");
        const auto* upsert =
            std::get_if<chionia::UpsertPointCloudCommand>(&*first);
        require(upsert != nullptr, "First queued command should be the upsert.");
        require(
            upsert->pointCloud.getPointCount() == 1,
            "Queued point cloud data should survive the queue transfer."
        );

        auto second = queue.tryDequeue();
        require(second.has_value(), "Queue should return its second command.");
        const auto* remove =
            std::get_if<chionia::RemoveSceneObjectCommand>(&*second);
        require(remove != nullptr, "Second queued command should be the removal.");
        require(remove->objectId == 17, "Queued removal should preserve its object ID.");
        require(!queue.tryDequeue().has_value(), "Queue should be empty afterward.");
    }

    void testInfinityGenerator() {
        const chionia::InfinityGeneratorConfig config{
            512,
            0.08f
        };

        const chionia::PointCloud pointCloud =
            chionia::InfinityGenerator::generate(config);

        require(
            pointCloud.getName() == "Infinity",
            "Generator should name its point cloud."
        );
        require(
            pointCloud.getPointCount() == config.pointCount,
            "Generator should produce the requested point count."
        );

        const auto& bounds = pointCloud.getBounds();
        require(bounds.has_value(), "Generated geometry should have bounds.");
        require(
            bounds->maximum.x > 0.9f && bounds->minimum.x < -0.9f,
            "Generated curve should extend across both lobes."
        );
        require(
            bounds->maximum.z > 0.0f && bounds->minimum.z < 0.0f,
            "Tube thickness should distribute points on both sides of the plane."
        );

        bool rejectedTooFewPoints = false;
        try {
            chionia::InfinityGenerator::generate({2, 0.0f});
        }
        catch (const std::invalid_argument&) {
            rejectedTooFewPoints = true;
        }
        require(
            rejectedTooFewPoints,
            "Generator should reject fewer than three points."
        );

        bool rejectedInvalidThickness = false;
        try {
            chionia::InfinityGenerator::generate({32, -0.01f});
        }
        catch (const std::invalid_argument&) {
            rejectedInvalidThickness = true;
        }
        require(
            rejectedInvalidThickness,
            "Generator should reject negative thickness."
        );
    }

    void testSolarSystemLessonBuildsAndExtendsScene() {
        chionia::SolarSystemLesson lesson;
        require(!lesson.hasSolarSystem(), "Lesson should start without a scene.");
        require(
            lesson.buildPointCloud().empty(),
            "Lesson should start with an empty point cloud."
        );

        require(
            lesson.applyTranscript(
                "Our solar system consists of the Sun and the objects that orbit it."
            ),
            "Solar-system text should create the first scene."
        );
        require(lesson.hasSolarSystem(), "Solar-system scene should be active.");
        require(
            !lesson.hasGravityExplanation(),
            "The gravity layer should wait for its own explanation."
        );

        const chionia::PointCloud initialScene = lesson.buildPointCloud();
        require(
            initialScene.getPointCount() > 2000,
            "Solar-system scene should contain the Sun, planets, and orbit details."
        );

        const auto initialColors = initialScene.getPoints();
        require(
            initialColors.front().color.r > initialColors.front().color.b,
            "The Sun should have a warm color."
        );
        require(
            lesson.applyTranscript(
                "Gravity from the massive Sun keeps the planets in orbit around it."
            ),
            "Gravity text should add an explanatory layer."
        );
        require(
            lesson.hasGravityExplanation(),
            "Gravity explanation should be recorded."
        );

        const chionia::PointCloud extendedScene = lesson.buildPointCloud();
        require(
            extendedScene.getPointCount() == initialScene.getPointCount(),
            "Adding gravity should preserve the fixed morph particle budget."
        );
        require(
            extendedScene.getPoints().front().color == initialScene.getPoints().front().color,
            "Adding gravity should preserve the existing solar-system points."
        );
        const glm::vec4 gravityColor(0.12f, 0.66f, 0.82f, 0.76f);
        const auto hasGravityPoint = [&gravityColor](const chionia::PointCloud& cloud) {
            for (const auto& point : cloud.getPoints()) {
                if (point.color == gravityColor) {
                    return true;
                }
            }
            return false;
        };
        require(
            !hasGravityPoint(initialScene) && hasGravityPoint(extendedScene),
            "Gravity should add its own visible orbit vectors."
        );
        require(
            !lesson.applyTranscript("The classroom is listening carefully."),
            "Unrelated text should leave the lesson scene unchanged."
        );
        require(
            !lesson.applyTranscript("Gravity keeps the planets in orbit."),
            "Repeating an existing concept should not duplicate it."
        );

        lesson.clear();
        require(!lesson.hasSolarSystem(), "Clearing should remove the solar system.");
        require(
            lesson.buildPointCloud().empty(),
            "Clearing should return an empty point cloud."
        );
    }

    void testPointCloudVertexConversionPreservesColors() {
        chionia::PointCloud cloud("colored points");
        const glm::vec4 expectedColor(0.15f, 0.72f, 0.96f, 0.43f);
        cloud.addPoint(glm::vec3(1.0f, 2.0f, 3.0f), expectedColor);

        const auto vertices =
            chionia::PointVertexConverter::convertPointCloudToVertices(cloud);
        require(vertices.size() == 1, "One point should convert to one vertex.");
        require(
            vertices.front().color == expectedColor,
            "Point-cloud color should reach the GPU vertex data."
        );
    }

}

int main() {
    try {
        testSceneUpsertAndTransform();
        testSceneRemovalAndLookup();
        testSceneCommandQueueFifo();
        testInfinityGenerator();
        testSolarSystemLessonBuildsAndExtendsScene();
        testPointCloudVertexConversionPreservesColors();
    }
    catch (const std::exception& error) {
        std::cerr << "Scene tests failed: " << error.what() << '\n';
        return 1;
    }

    std::cout << "Scene tests passed.\n";
    return 0;
}

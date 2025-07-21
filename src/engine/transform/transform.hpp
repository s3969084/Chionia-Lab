#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace chionia {

    class Transform {
    public:
        Transform();

        void setPosition(const glm::vec3& position);
        void setRotation(const glm::vec3& rotation);
        void setScale(const glm::vec3& scale);

        const glm::vec3& getPosition() const;
        const glm::vec3& getRotation() const;
        const glm::vec3& getScale() const;

        glm::mat4 getModelMatrix() const;

    private:
        glm::vec3 position_;
        glm::vec3 rotation_;
        glm::vec3 scale_;

    };
}
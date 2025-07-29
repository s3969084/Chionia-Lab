#include "transform.hpp"

namespace chionia {

    Transform::Transform()
        : position_(0.0f), rotation_(0.0f), scale_(1.0f) {}

    void Transform::setPosition(const glm::vec3& position) {
        position_ = position;
    }

    void Transform::setRotation(const glm::vec3& rotation) {
        rotation_ = rotation;
    }

    void Transform::setScale(const glm::vec3& scale) {
        scale_ = scale;
    }

    const glm::vec3& Transform::getPosition() const {
        return position_;
    }

    const glm::vec3& Transform::getRotation() const {
        return rotation_;
    }

    const glm::vec3& Transform::getScale() const {
        return scale_;
    }

    glm::mat4 Transform::getModelMatrix() const {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position_);
        model = glm::rotate(model, glm::radians(rotation_.x), glm::vec3(1, 0, 0));
        model = glm::rotate(model, glm::radians(rotation_.y), glm::vec3(0, 1, 0));
        model = glm::rotate(model, glm::radians(rotation_.z), glm::vec3(0, 0, 1));
        model = glm::scale(model, scale_);
        return model;
    }




}
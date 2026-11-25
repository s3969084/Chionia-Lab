#pragma once

namespace chionia {

    struct SceneRenderSettings {
        float morphDurationSeconds = 2.0f;
        float vibrationAmplitude = 0.03f;
        float vibrationSpeed = 8.0f;
        bool vibrationEnabled = true;
        bool morphingEnabled = true;
        bool autoMorphEnabled = false;
        bool groundReflectionEnabled = true;
    };

}

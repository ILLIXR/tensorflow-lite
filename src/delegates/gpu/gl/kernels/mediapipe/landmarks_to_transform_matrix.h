#ifndef TENSORFLOW_LITE_DELEGATES_GPU_GL_KERNELS_MEDIAPIPE_LANDMARKS_TO_TRANSFORM_MATRIX_H_
#define TENSORFLOW_LITE_DELEGATES_GPU_GL_KERNELS_MEDIAPIPE_LANDMARKS_TO_TRANSFORM_MATRIX_H_

#include <memory>

#include "delegates/gpu/common/operations.h"
#include "delegates/gpu/gl/node_shader.h"

namespace tflite {
namespace gpu {
namespace gl {

std::unique_ptr<NodeShader> NewLandmarksToTransformMatrixNodeShader();

}  // namespace gl
}  // namespace gpu
}  // namespace tflite

#endif  // TENSORFLOW_LITE_DELEGATES_GPU_GL_KERNELS_MEDIAPIPE_LANDMARKS_TO_TRANSFORM_MATRIX_H_

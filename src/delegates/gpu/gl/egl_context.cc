/* Copyright 2019 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "delegates/gpu/gl/egl_context.h"

#include <cstring>

#include "delegates/gpu/common/status.h"
#include "delegates/gpu/gl/gl_call.h"
#include "delegates/gpu/gl/gl_errors.h"

namespace tflite {
namespace gpu {
namespace gl {
namespace {

absl::Status GetConfig(EGLDisplay display, const EGLint* attributes,
                       EGLConfig* config) {
  EGLint config_count;
  bool chosen = eglChooseConfig(display, attributes, config, 1, &config_count);
  RETURN_IF_ERROR(GetOpenGlErrors());
  if (!chosen || config_count == 0) {
    return absl::InternalError("No EGL error, but eglChooseConfig failed.");
  }
  return absl::OkStatus();
}

absl::Status CreateContext(EGLDisplay display, EGLContext shared_context,
                           EGLConfig config, EglContext* egl_context) {
  static const EGLint attributes[] = {EGL_CONTEXT_CLIENT_VERSION, 3,
#ifdef _DEBUG  // Add debugging bit
                                      EGL_CONTEXT_FLAGS_KHR,
                                      EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR,
#endif
                                      EGL_NONE};
  EGLContext context =
      eglCreateContext(display, config, shared_context, attributes);
  RETURN_IF_ERROR(GetOpenGlErrors());
  if (context == EGL_NO_CONTEXT) {
    return absl::InternalError("No EGL error, but eglCreateContext failed.");
  }
  *egl_context = EglContext(context, display, config, true);
  return absl::OkStatus();
}

bool HasExtension(EGLDisplay display, const char* name) {
  return std::strstr(eglQueryString(display, EGL_EXTENSIONS), name);
}

}  // namespace

void EglContext::Invalidate() {
  if (context_ != EGL_NO_CONTEXT) {
    if (has_ownership_) {
      eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
      eglDestroyContext(display_, context_);
    }
    context_ = EGL_NO_CONTEXT;
  }
  has_ownership_ = false;
}

EglContext::EglContext(EglContext&& other)
    : context_(other.context_),
      display_(other.display_),
      config_(other.config_),
      has_ownership_(other.has_ownership_) {
  other.context_ = EGL_NO_CONTEXT;
  other.has_ownership_ = false;
}

EglContext& EglContext::operator=(EglContext&& other) {
  if (this != &other) {
    Invalidate();
    using std::swap;
    swap(context_, other.context_);
    display_ = other.display_;
    config_ = other.config_;
    swap(has_ownership_, other.has_ownership_);
  }
  return *this;
}

absl::Status EglContext::MakeCurrent(EGLSurface read, EGLSurface write) {
  bool is_made_current = eglMakeCurrent(display_, write, read, context_);
  RETURN_IF_ERROR(GetOpenGlErrors());
  if (!is_made_current) {
    return absl::InternalError("No EGL error, but eglMakeCurrent failed.");
  }
  return absl::OkStatus();
}

bool EglContext::IsCurrent() const {
  return context_ == eglGetCurrentContext();
}

absl::Status CreateConfiglessContext(EGLDisplay display,
                                     EGLContext shared_context,
                                     EglContext* egl_context) {
  if (!HasExtension(display, "EGL_KHR_no_config_context")) {
    return absl::UnavailableError("EGL_KHR_no_config_context not supported");
  }
  return CreateContext(display, shared_context, EGL_NO_CONFIG_KHR, egl_context);
}

absl::Status CreateSurfacelessContext(EGLDisplay display,
                                      EGLContext shared_context,
                                      EglContext* egl_context) {
  if (!HasExtension(display, "EGL_KHR_create_context")) {
    return absl::UnavailableError("EGL_KHR_create_context not supported");
  }
  if (!HasExtension(display, "EGL_KHR_surfaceless_context")) {
    return absl::UnavailableError("EGL_KHR_surfaceless_context not supported");
  }
  const EGLint attributes[] = {EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR,
                               EGL_NONE};
  EGLConfig config;
  RETURN_IF_ERROR(GetConfig(display, attributes, &config));
  return CreateContext(display, shared_context, config, egl_context);
}

absl::Status CreatePBufferContext(EGLDisplay display, EGLContext shared_context,
                                  EglContext* egl_context) {
  const EGLint attributes[] = {
      EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,     EGL_BIND_TO_TEXTURE_RGB,
      EGL_TRUE,         EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR,
      EGL_NONE};
  EGLConfig config;
  RETURN_IF_ERROR(GetConfig(display, attributes, &config));
  return CreateContext(display, shared_context, config, egl_context);
}

}  // namespace gl
}  // namespace gpu
}  // namespace tflite

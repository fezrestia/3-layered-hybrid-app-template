# Android.mk

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := hybridsynergycamera

LOCAL_C_FLAGS := \
        -Wall \
        -Werror \

LOCAL_C_INCLUDES := \
        jni/includes/android/opengl/ \
        jni/includes/gl_shaders/ \
        jni/includes/gl_elements/ \
        jni/includes/ \

LOCAL_SRC_FILES := \
        android/opengl/android_opengl_matrix.c \
        gl_shaders/ShaderProgramFactory.cpp \
        gl_elements/ElementBase.cpp \
        gl_elements/SimpleFrame.cpp \
        gl_elements/YuvFrame.cpp \
        HybridSynergyCamera.cpp \

LOCAL_LDLIBS := \
        -llog \
        -landroid \
        -lEGL \
        -lGLESv2 \

include $(BUILD_SHARED_LIBRARY)

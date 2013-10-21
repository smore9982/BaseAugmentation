#Instructions to build QC library.
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
#Name of module to build. Will become libQCAR-prebuild.so
LOCAL_MODULE := QCAR-prebuilt

#Points to /Users/more/vuforia-sdk-android-2-6-10/build/lib. MAKE SURE TO CHANGE THIS DEPDENING ON WHERE YOU ARE BUILDING FROM.
#LOCAL_SRC_FILES = ../../../build/lib/$(TARGET_ARCH_ABI)/libQCAR.so
LOCAL_SRC_FILES = /cygdrive/c/Users/Sameer/vuforia/vuforia-sdk-android-2-6-10/build/lib/$(TARGET_ARCH_ABI)/libQCAR.so

#Points to /Users/more/vuforia-sdk-android-2-6-10/build/include
#LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../../build/include
#MAKE SURE TO CHANGE THIS DEPDENING ON WHERE YOU ARE BUILDING FROM.
LOCAL_EXPORT_C_INCLUDES := /cygdrive/c/Users/Sameer/vuforia/vuforia-sdk-android-2-6-10/build/include


#Builds the libQCAR.so library define in LOCAL_SRC_FILES
include $(PREBUILT_SHARED_LIBRARY)


#Build Application libraries
include $(CLEAR_VARS)
LOCAL_MODULE := AUGMENTLIB

#Use OPENGL
OPENGLES_LIB  := -lGLESv2
OPENGLES_DEF  := -DUSE_OPENGL_ES_2_0

LOCAL_CFLAGS := -Wno-write-strings -Wno-psabi $(OPENGLES_DEF)
LOCAL_LDLIBS := \
    -llog $(OPENGLES_LIB)

# Dependency on QCAR-prebuild module.
LOCAL_SHARED_LIBRARIES := QCAR-prebuilt

LOCAL_SRC_FILES = Transition3Dto2D.cpp Augment.cpp SampleUtils.cpp Texture.cpp SampleMath.cpp  

LOCAL_ARM_MODE := arm
include $(BUILD_SHARED_LIBRARY)


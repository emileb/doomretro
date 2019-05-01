
LOCAL_PATH := $(call my-dir)/../src/

include $(CLEAR_VARS)

LOCAL_MODULE    := doomretro

LOCAL_CFLAGS :=  -DRETRO_DOOM -DENGINE_NAME=\"doomretro\" -Wall -Wdeclaration-after-statement  -D_GNU_SOURCE=1 -D_REENTRANT -fsigned-char


LOCAL_C_INCLUDES :=     $(SDL_INCLUDE_PATHS)  \
                        $(TOP_DIR) \
                        $(TOP_DIR)/MobileTouchControls \
                        $(TOP_DIR)/Clibs_OpenTouch \
                        $(TOP_DIR)/Clibs_OpenTouch/idtech1 \
                        $(LOCAL_PATH)/../mobile \

ANDROID_FILES = \
      ../../../Clibs_OpenTouch/idtech1/android_jni.cpp \
      ../../../Clibs_OpenTouch/idtech1/touch_interface.cpp \
      ../mobile/game_interface.c \

FILE_LIST := $(wildcard $(LOCAL_PATH)*.c)
LOCAL_SRC_FILES := $(ANDROID_FILES) $(FILE_LIST:$(LOCAL_PATH)%=%)


LOCAL_LDLIBS += -llog -lz -lGLESv1_CM

LOCAL_SHARED_LIBRARIES := touchcontrols SDL2 SDL2_mixer SDL2_image logwritter core_shared
LOCAL_STATIC_LIBRARIES +=

include $(BUILD_SHARED_LIBRARY)







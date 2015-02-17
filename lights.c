#define LOG_TAG "lights"
#include <cutils/log.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <hardware/lights.h>
#include <dirent.h>
#include <stdio.h>

#ifndef LOGD      
#define LOGE ALOGE
#define LOGI ALOGI
#define LOGV ALOGV
#define LOGD ALOGD
#define LOGW ALOGW
#endif

#define VENDOR 0x04d8
#define PRODUCT 0xf724
#define MAX_BRIGHTNESS 35
#define CMD_SET_BRIGHTNESS 0x0120

static pthread_once_t g_init = PTHREAD_ONCE_INIT;
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static char hidraw_path[128];

const char *SYS_DIR = "/sys/bus/hid/devices";
const char *HIDRAW_DIR = "/sys/bus/hid/devices/%s/hidraw";
const char *FULL_DIR = "/dev/%s";
const char *SEARCH_STR = "04D8:F724";

static int rgb_to_brightness(struct light_state_t const *state)
{
    int color = state->color & 0x00ffffff;

    return ((77*((color>>16) & 0x00ff)) + (150*((color>>8) & 0x00ff)) + (29*(color & 0x00ff))) >> 8;
}

static int set_light_backlight(struct light_device_t *dev, struct light_state_t const *state)
{
    int err = 0;
    int brightness = rgb_to_brightness(state);
    brightness = (int)((((float)brightness) / 255.0) * MAX_BRIGHTNESS);

    if(strlen(hidraw_path) == 0) {
        LOGW("not changing brightness, couldn't find hidraw");
        return err;
    }

    pthread_mutex_lock(&g_lock);

    FILE *fd = fopen(hidraw_path, "wb+");
    if(fd == NULL) {
        LOGW("couldn't open: %s", hidraw_path);
        return err;
    }

    unsigned char buf[3];
    buf[0] = 0x01;
    buf[1] = 0x20;
    buf[2] = brightness;
    int res = fwrite(buf, 1, 3, fd);
    fflush(fd);
    if(res < 0) {
        LOGE("unable to set brightness: %d", errno);
    }

    res = fread(buf, 1, 3, fd);
    if(res < 0) {
        LOGE("unable to read brightness: %d", errno);
    } else {
        LOGV("brightness: %d, ambient light: %d", buf[1] & 0x3F, buf[2]);
    }

    fclose(fd);

    pthread_mutex_unlock(&g_lock);
    return err;
}

static int close_lights(struct light_device_t *dev)
{
    LOGV("close_light is called");
    if (dev)
        free(dev);

    return 0;
}

static int open_lights(const struct hw_module_t *module, char const *name,
                        struct hw_device_t **device)
{
    int (*set_light)(struct light_device_t *dev,
        struct light_state_t const *state);

    LOGV("open_lights: open with %s", name);

    if (0 == strcmp(LIGHT_ID_BACKLIGHT, name))
        set_light = set_light_backlight;
    else
        return -EINVAL;

    pthread_mutex_init(&g_lock, NULL);

    struct light_device_t *dev = malloc(sizeof(struct light_device_t));
    memset(dev, 0, sizeof(*dev));

    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = 0;
    dev->common.module = (struct hw_module_t *)module;
    dev->common.close = (int (*)(struct hw_device_t *))close_lights;
    dev->set_light = set_light;

    *device = (struct hw_device_t *)dev;

    DIR *dir;
    struct dirent *ent;
    if((dir = opendir(SYS_DIR)) != NULL) {
        hidraw_path[0] = '\0';
        while((ent = readdir(dir)) != NULL) {
            if(strstr(ent->d_name, SEARCH_STR) != NULL) {
                LOGI("found touch screen: %s", ent->d_name);
                char rawpath[128];
                sprintf(rawpath, HIDRAW_DIR, ent->d_name);
                DIR *rawdir;
                struct dirent *rawent;
                if((rawdir = opendir(rawpath)) != NULL) {
                    while((rawent = readdir(rawdir)) != NULL) {
                        if(strstr(rawent->d_name, "hidraw") != NULL) {
                            sprintf(hidraw_path, FULL_DIR, rawent->d_name);
                            break;
                        }
                    }
                    closedir(rawdir);
                } else {
		    LOGW("couldn't open path: %s", rawpath);
                }
                break;
            }
        }
        closedir(dir);
    }

    if(strlen(hidraw_path) > 0) {
        LOGI("found hidraw: %s", hidraw_path);
    } else {
        LOGW("couldn't find hidraw!");
    }

    return 0;
}

static struct hw_module_methods_t lights_module_methods = {
    .open =  open_lights,
};

/*const */ struct hw_module_t HAL_MODULE_INFO_SYM = {
    .tag = HARDWARE_MODULE_TAG,
    .version_major = 1,
    .version_minor = 0,
    .id = LIGHTS_HARDWARE_MODULE_ID,
    .name = "lights Module",
    .author = "Google, Inc.",
    .methods = &lights_module_methods,
};

#ifdef __linux__

#include "linux_audio.h"
#include <pulse/pulseaudio.h>
#include <QEventLoop>
#include <QTimer>
#include <cstring>

struct PulseEnumData {
    QVector<AudioDeviceInfo> devices;
    pa_mainloop *mainloop;
    bool done;
};

static void sink_info_cb(pa_context *, const pa_sink_info *info, int eol, void *userdata)
{
    auto *data = static_cast<PulseEnumData *>(userdata);
    if (eol > 0) {
        return;
    }
    if (info) {
        AudioDeviceInfo dev;
        dev.id = QString::fromUtf8(info->name);
        dev.name = QString::fromUtf8(info->description);
        dev.isOutput = true;
        data->devices.append(dev);
    }
}

static void source_info_cb(pa_context *, const pa_source_info *info, int eol, void *userdata)
{
    auto *data = static_cast<PulseEnumData *>(userdata);
    if (eol > 0) {
        data->done = true;
        pa_mainloop_quit(data->mainloop, 0);
        return;
    }
    if (info) {
        // Skip monitor sources
        if (info->monitor_of_sink != PA_INVALID_INDEX)
            return;
        AudioDeviceInfo dev;
        dev.id = QString::fromUtf8(info->name);
        dev.name = QString::fromUtf8(info->description);
        dev.isOutput = false;
        data->devices.append(dev);
    }
}

static void context_state_cb_enum(pa_context *c, void *userdata)
{
    auto *data = static_cast<PulseEnumData *>(userdata);
    switch (pa_context_get_state(c)) {
    case PA_CONTEXT_READY:
        pa_context_get_sink_info_list(c, sink_info_cb, userdata);
        pa_context_get_source_info_list(c, source_info_cb, userdata);
        break;
    case PA_CONTEXT_FAILED:
    case PA_CONTEXT_TERMINATED:
        data->done = true;
        pa_mainloop_quit(data->mainloop, 1);
        break;
    default:
        break;
    }
}

QVector<AudioDeviceInfo> LinuxAudio::enumerateDevices()
{
    PulseEnumData data;
    data.done = false;

    pa_mainloop *ml = pa_mainloop_new();
    data.mainloop = ml;
    pa_mainloop_api *api = pa_mainloop_get_api(ml);
    pa_context *ctx = pa_context_new(api, "QuickSnapAudio");

    pa_context_set_state_callback(ctx, context_state_cb_enum, &data);
    pa_context_connect(ctx, nullptr, PA_CONTEXT_NOFLAGS, nullptr);

    int ret = 0;
    while (!data.done) {
        if (pa_mainloop_iterate(ml, 1, &ret) < 0)
            break;
    }

    pa_context_disconnect(ctx);
    pa_context_unref(ctx);
    pa_mainloop_free(ml);

    return data.devices;
}

struct PulseSetData {
    pa_mainloop *mainloop;
    bool success;
    bool done;
};

static void success_cb(pa_context *, int success, void *userdata)
{
    auto *data = static_cast<PulseSetData *>(userdata);
    data->success = (success != 0);
    data->done = true;
    pa_mainloop_quit(data->mainloop, 0);
}

static void context_state_cb_set(pa_context *c, void *userdata);

struct SetDefaultArgs {
    PulseSetData *data;
    QByteArray deviceIdUtf8;
    bool isOutput;
};

static void context_state_cb_set(pa_context *c, void *userdata)
{
    auto *args = static_cast<SetDefaultArgs *>(userdata);
    auto *data = args->data;

    switch (pa_context_get_state(c)) {
    case PA_CONTEXT_READY:
        if (args->isOutput) {
            pa_context_set_default_sink(c, args->deviceIdUtf8.constData(), success_cb, data);
        } else {
            pa_context_set_default_source(c, args->deviceIdUtf8.constData(), success_cb, data);
        }
        break;
    case PA_CONTEXT_FAILED:
    case PA_CONTEXT_TERMINATED:
        data->done = true;
        data->success = false;
        pa_mainloop_quit(data->mainloop, 1);
        break;
    default:
        break;
    }
}

bool LinuxAudio::setDefaultDevice(const QString &deviceId, bool isOutput)
{
    PulseSetData data;
    data.done = false;
    data.success = false;

    SetDefaultArgs args;
    args.data = &data;
    args.deviceIdUtf8 = deviceId.toUtf8();
    args.isOutput = isOutput;

    pa_mainloop *ml = pa_mainloop_new();
    data.mainloop = ml;
    pa_mainloop_api *api = pa_mainloop_get_api(ml);
    pa_context *ctx = pa_context_new(api, "QuickSnapAudio");

    pa_context_set_state_callback(ctx, context_state_cb_set, &args);
    pa_context_connect(ctx, nullptr, PA_CONTEXT_NOFLAGS, nullptr);

    int ret = 0;
    while (!data.done) {
        if (pa_mainloop_iterate(ml, 1, &ret) < 0)
            break;
    }

    pa_context_disconnect(ctx);
    pa_context_unref(ctx);
    pa_mainloop_free(ml);

    return data.success;
}

#endif // __linux__

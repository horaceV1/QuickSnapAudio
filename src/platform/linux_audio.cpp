#ifdef __linux__

#include "linux_audio.h"
#include <pulse/pulseaudio.h>
#include <QEventLoop>
#include <QTimer>
#include <QElapsedTimer>
#include <QDebug>
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

    // Hard cap so a hung PulseAudio daemon can never freeze the UI thread.
    QElapsedTimer timer;
    timer.start();
    constexpr qint64 kMaxMs = 5000;

    int ret = 0;
    while (!data.done) {
        if (timer.hasExpired(kMaxMs)) {
            qWarning("LinuxAudio::enumerateDevices: PulseAudio timeout after %lld ms",
                     static_cast<long long>(timer.elapsed()));
            break;
        }
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
    int pendingMoves;
    bool enumDone;
};

// Finish the operation once stream enumeration has completed and every
// in-flight move has reported back.
static void finishSetDefaultIfReady(SetDefaultArgs *args)
{
    if (args->enumDone && args->pendingMoves <= 0) {
        args->data->done = true;
        pa_mainloop_quit(args->data->mainloop, 0);
    }
}

// Called when an individual stream has been moved to the target device.
static void move_done_cb(pa_context *, int success, void *userdata)
{
    auto *args = static_cast<SetDefaultArgs *>(userdata);
    Q_UNUSED(success);
    if (args->pendingMoves > 0)
        args->pendingMoves--;
    finishSetDefaultIfReady(args);
}

// Move every currently-playing output stream onto the new default sink so the
// switch takes effect immediately instead of only applying to future streams.
static void sink_input_move_cb(pa_context *c, const pa_sink_input_info *info, int eol, void *userdata)
{
    auto *args = static_cast<SetDefaultArgs *>(userdata);
    if (eol > 0) {
        args->enumDone = true;
        finishSetDefaultIfReady(args);
        return;
    }
    if (info) {
        args->pendingMoves++;
        pa_operation *o = pa_context_move_sink_input_by_name(
            c, info->index, args->deviceIdUtf8.constData(), move_done_cb, args);
        if (o)
            pa_operation_unref(o);
        else if (args->pendingMoves > 0)
            args->pendingMoves--;
    }
}

// Same as above for capture streams routed to the new default source.
static void source_output_move_cb(pa_context *c, const pa_source_output_info *info, int eol, void *userdata)
{
    auto *args = static_cast<SetDefaultArgs *>(userdata);
    if (eol > 0) {
        args->enumDone = true;
        finishSetDefaultIfReady(args);
        return;
    }
    if (info) {
        args->pendingMoves++;
        pa_operation *o = pa_context_move_source_output_by_name(
            c, info->index, args->deviceIdUtf8.constData(), move_done_cb, args);
        if (o)
            pa_operation_unref(o);
        else if (args->pendingMoves > 0)
            args->pendingMoves--;
    }
}

// After the default device has been set, redirect existing streams.
static void default_set_cb(pa_context *c, int success, void *userdata)
{
    auto *args = static_cast<SetDefaultArgs *>(userdata);
    args->data->success = (success != 0);

    pa_operation *o = args->isOutput
        ? pa_context_get_sink_input_info_list(c, sink_input_move_cb, args)
        : pa_context_get_source_output_info_list(c, source_output_move_cb, args);
    if (o) {
        pa_operation_unref(o);
    } else {
        // Could not enumerate streams; the default was still updated.
        args->enumDone = true;
        finishSetDefaultIfReady(args);
    }
}

static void context_state_cb_set(pa_context *c, void *userdata)
{
    auto *args = static_cast<SetDefaultArgs *>(userdata);
    auto *data = args->data;

    switch (pa_context_get_state(c)) {
    case PA_CONTEXT_READY:
        if (args->isOutput) {
            pa_context_set_default_sink(c, args->deviceIdUtf8.constData(), default_set_cb, args);
        } else {
            pa_context_set_default_source(c, args->deviceIdUtf8.constData(), default_set_cb, args);
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
    args.pendingMoves = 0;
    args.enumDone = false;

    pa_mainloop *ml = pa_mainloop_new();
    data.mainloop = ml;
    pa_mainloop_api *api = pa_mainloop_get_api(ml);
    pa_context *ctx = pa_context_new(api, "QuickSnapAudio");

    pa_context_set_state_callback(ctx, context_state_cb_set, &args);
    pa_context_connect(ctx, nullptr, PA_CONTEXT_NOFLAGS, nullptr);

    QElapsedTimer timer;
    timer.start();
    int ret = 0;
    while (!data.done) {
        if (pa_mainloop_iterate(ml, 1, &ret) < 0)
            break;
        // Safety net so a stalled PulseAudio connection cannot hang the UI.
        if (timer.elapsed() > 5000)
            break;
    }

    pa_context_disconnect(ctx);
    pa_context_unref(ctx);
    pa_mainloop_free(ml);

    return data.success;
}

// ---- Mute support ----
struct PulseMuteSetArgs {
    PulseSetData *data;
    QByteArray deviceIdUtf8;
    bool isOutput;
    bool mute;
};

static void mute_context_state_cb(pa_context *c, void *userdata)
{
    auto *args = static_cast<PulseMuteSetArgs *>(userdata);
    auto *data = args->data;
    switch (pa_context_get_state(c)) {
    case PA_CONTEXT_READY:
        if (args->isOutput) {
            pa_context_set_sink_mute_by_name(c, args->deviceIdUtf8.constData(),
                                             args->mute ? 1 : 0, success_cb, data);
        } else {
            pa_context_set_source_mute_by_name(c, args->deviceIdUtf8.constData(),
                                               args->mute ? 1 : 0, success_cb, data);
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

bool LinuxAudio::setMute(const QString &deviceId, bool isOutput, bool mute)
{
    PulseSetData data;
    data.done = false;
    data.success = false;

    PulseMuteSetArgs args;
    args.data = &data;
    args.deviceIdUtf8 = deviceId.toUtf8();
    args.isOutput = isOutput;
    args.mute = mute;

    pa_mainloop *ml = pa_mainloop_new();
    data.mainloop = ml;
    pa_mainloop_api *api = pa_mainloop_get_api(ml);
    pa_context *ctx = pa_context_new(api, "QuickSnapAudio");

    pa_context_set_state_callback(ctx, mute_context_state_cb, &args);
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

struct PulseMuteQuery {
    pa_mainloop *mainloop;
    bool done;
    bool muted;
    bool found;
};

static void sink_mute_cb(pa_context *, const pa_sink_info *info, int eol, void *userdata)
{
    auto *q = static_cast<PulseMuteQuery *>(userdata);
    if (eol > 0) {
        q->done = true;
        pa_mainloop_quit(q->mainloop, 0);
        return;
    }
    if (info) {
        q->muted = info->mute != 0;
        q->found = true;
    }
}

static void source_mute_cb(pa_context *, const pa_source_info *info, int eol, void *userdata)
{
    auto *q = static_cast<PulseMuteQuery *>(userdata);
    if (eol > 0) {
        q->done = true;
        pa_mainloop_quit(q->mainloop, 0);
        return;
    }
    if (info) {
        q->muted = info->mute != 0;
        q->found = true;
    }
}

struct PulseMuteQueryArgs {
    PulseMuteQuery *q;
    QByteArray deviceIdUtf8;
    bool isOutput;
};

static void mute_query_state_cb(pa_context *c, void *userdata)
{
    auto *args = static_cast<PulseMuteQueryArgs *>(userdata);
    auto *q = args->q;
    switch (pa_context_get_state(c)) {
    case PA_CONTEXT_READY:
        if (args->isOutput) {
            pa_context_get_sink_info_by_name(c, args->deviceIdUtf8.constData(), sink_mute_cb, q);
        } else {
            pa_context_get_source_info_by_name(c, args->deviceIdUtf8.constData(), source_mute_cb, q);
        }
        break;
    case PA_CONTEXT_FAILED:
    case PA_CONTEXT_TERMINATED:
        q->done = true;
        pa_mainloop_quit(q->mainloop, 1);
        break;
    default:
        break;
    }
}

bool LinuxAudio::isMuted(const QString &deviceId, bool isOutput)
{
    PulseMuteQuery q;
    q.done = false;
    q.muted = false;
    q.found = false;

    PulseMuteQueryArgs args;
    args.q = &q;
    args.deviceIdUtf8 = deviceId.toUtf8();
    args.isOutput = isOutput;

    pa_mainloop *ml = pa_mainloop_new();
    q.mainloop = ml;
    pa_mainloop_api *api = pa_mainloop_get_api(ml);
    pa_context *ctx = pa_context_new(api, "QuickSnapAudio");

    pa_context_set_state_callback(ctx, mute_query_state_cb, &args);
    pa_context_connect(ctx, nullptr, PA_CONTEXT_NOFLAGS, nullptr);

    int ret = 0;
    while (!q.done) {
        if (pa_mainloop_iterate(ml, 1, &ret) < 0) break;
    }

    pa_context_disconnect(ctx);
    pa_context_unref(ctx);
    pa_mainloop_free(ml);
    return q.muted;
}

#endif // __linux__

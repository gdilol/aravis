/* Aravis - Digital camera library
 *
 * Copyright © 2009-2022 Emmanuel Pacaud
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Author: Emmanuel Pacaud <emmanuel.pacaud@free.fr>
 */

#include <aldinvoker.h>
#include <arvbufferprivate.h>

struct _AldInvoker
{
    /* data */
    GTypeInstance parent;
    ArvCamera *camera;
    ArvStream *current_stream;
    //AldBufferInfo info;
    AldInvokerCallback cb;
    AldInvokerAcquisitionStrategy acq_strategy;
    char *hardware_trigger_source;
    int frame_count_per_trigger;
    AldInvokerBufferStrategy buf_strategy;
    int buffer_count;
    gboolean is_in_acquisition;
};

G_DEFINE_FINAL_TYPE(AldInvoker, ald_invoker, G_TYPE_OBJECT);

static void ald_invoker_class_init(AldInvokerClass *klass)
{
    ;
}
static void ald_invoker_init(AldInvoker *instance)
{
    instance->camera = NULL;
    instance->current_stream = NULL;
    //instance->info = g_malloc0(sizeof(AldBufferInfo));
    instance->cb = NULL;
    instance->acq_strategy = ALD_INVOKER_ACQUISITION_STRATEGY_CONTINUOUS_RUN;
    instance->buf_strategy = ALD_INVOKER_BUFFER_DEFAULT;
    instance->buffer_count = 4;
    instance->is_in_acquisition = FALSE;
}

static void CopyStreamCallback(void *user_data, ArvStreamCallbackType type, ArvBuffer *buffer);
static void InPlaceStreamCallback(void *user_data, ArvStreamCallbackType type, ArvBuffer *buffer);

/**
 * @brief construct a invoker class which provides fast reach to deal with the buffer
 *
 * @param camera
 * @return AldInvoker*, a invoker class
 */
AldInvoker *ald_invoker_new(ArvCamera *camera, AldInvokerCallback cb)
{
    AldInvoker *invoker = g_object_new(ALD_TYPE_INVOKER, NULL);
    invoker->camera = camera;
    invoker->cb = cb;
    return invoker;
}

void ald_invoker_set_buffer_count(AldInvoker *invoker, int buffer_count, GError **error)
{
    invoker->buffer_count = buffer_count;
    g_return_if_fail(invoker->current_stream != NULL);
    if (invoker->is_in_acquisition)
    {
        ald_invoker_stop_acquisition(invoker, error);
    }

    guint size = arv_camera_get_payload(invoker->camera, NULL);
    arv_stream_stop_thread(invoker->current_stream, TRUE);
    for (size_t i = 0; i < buffer_count; i++)
    {
        arv_stream_push_buffer(invoker->current_stream, arv_buffer_new(size, NULL));
    }
    arv_stream_start_thread(invoker->current_stream);

    if (invoker->is_in_acquisition)
    {
        ald_invoker_start_acquisition(invoker, error);
    }
}

void ald_invoker_set_hardware_trigger_source(AldInvoker *invoker, const char *source, GError **error)
{
    if (invoker->hardware_trigger_source != NULL)
    {
        g_free(invoker->hardware_trigger_source);
    }
    invoker->hardware_trigger_source = source;
}

void ald_invoker_set_acquisition_strategy(AldInvoker *invoker, AldInvokerAcquisitionStrategy strategy, GError **error)
{
    invoker->acq_strategy = strategy;
    ald_invoker_stop_acquisition(invoker, error);
    ArvCamera *camera = invoker->camera;
    ArvAcquisitionMode mode = arv_camera_get_acquisition_mode(invoker->camera, error);
    if (mode != ARV_ACQUISITION_MODE_CONTINUOUS)
    {
        arv_camera_set_acquisition_mode(camera, ARV_ACQUISITION_MODE_CONTINUOUS, error);
    }

    switch (invoker->acq_strategy)
    {
    case ALD_INVOKER_ACQUISITION_STRATEGY_CONTINUOUS_RUN:
        arv_camera_set_string(invoker->camera, "TriggerMode", "Off", error);

        break;
    case ALD_INVOKER_ACQUISITION_STRATEGY_SOFTWARETRIGGER:
        arv_camera_set_string(invoker->camera, "TriggerSelector", "FrameBurstStart", error);
        arv_camera_set_string(invoker->camera, "TriggerMode", "On", error);
        arv_camera_set_trigger_source(camera, "Software", error);
        break;
    case ALD_INVOKER_ACQUISITION_STRATEGY_HARDWARETRIGGER:
        arv_camera_set_string(invoker->camera, "TriggerSelector", "FrameBurstStart", error);
        arv_camera_set_string(invoker->camera, "TriggerMode", "On", error);
        g_return_if_fail(invoker->hardware_trigger_source != NULL);
        arv_camera_set_trigger_source(camera, invoker->hardware_trigger_source, error);
        break;
    default:
        break;
    }
}

void ald_invoker_set_buffer_strategy(AldInvoker *invoker, AldInvokerBufferStrategy strategy, GError **error)
{
    invoker->buf_strategy = strategy;
    g_return_if_fail(invoker->current_stream != NULL);

    if (invoker->is_in_acquisition)
    {
        ald_invoker_stop_acquisition(invoker, error);
    }
    ald_invoker_reset_thread(invoker);
    if (invoker->is_in_acquisition)
    {
        ald_invoker_start_acquisition(invoker, error);
    }
}

void ald_invoker_set_frame_count_per_trigger(AldInvoker *invoker, int count, GError **error)
{
    invoker->frame_count_per_trigger = count;
    g_return_if_fail(arv_camera_is_feature_available(invoker->camera, "AcquisitionBurstFrameCount", error));

    if (invoker->is_in_acquisition)
    {
        ald_invoker_stop_acquisition(invoker, error);
    }
    arv_camera_set_integer(invoker->camera, "AcquisitionBurstFrameCount", count, error);
    if (invoker->is_in_acquisition)
    {
        ald_invoker_start_acquisition(invoker, error);
    }
}

gboolean ald_invoker_is_acquisition_strategy_supported(AldInvoker *invoker, AldInvokerAcquisitionStrategy strategy, GError **error)
{
    gboolean supported = FALSE;
    guint n_sources;
    const char **sources = arv_camera_dup_available_trigger_sources(invoker->camera, &n_sources, error);
    guint n_triggers;
    const char **triggers = arv_camera_dup_available_triggers(invoker->camera, &n_triggers, error);
    switch (strategy)
    {
    case ALD_INVOKER_ACQUISITION_STRATEGY_CONTINUOUS_RUN:
        supported = TRUE;
        break;
    case ALD_INVOKER_ACQUISITION_STRATEGY_HARDWARETRIGGER:
        size_t i = 0;
        for (; i < n_sources; i++)
        {
            if (strcmp(sources[i], invoker->hardware_trigger_source) == 0)
            {
                break;
            }
        }
        size_t j = 0;
        for (; j < n_triggers; i++)
        {
            if (strcmp(triggers[j], "FrameBurstStart") == 0)
            {
                break;
            }
        }
        if (i == n_sources && j == n_triggers)
        {
            supported = TRUE;
        }
        break;
    case ALD_INVOKER_ACQUISITION_STRATEGY_SOFTWARETRIGGER:
        if (arv_camera_is_software_trigger_supported(invoker->camera, error))
        {
            size_t j = 0;
            for (; j < n_triggers; i++)
            {
                if (strcmp(triggers[j], "FrameBurstStart") == 0)
                {
                    break;
                }
            }
            if (j == n_triggers)
            {
                supported = TRUE;
            }
        }
        break;
    default:
        break;
    }
    g_free(sources);
    g_free(triggers);
    return supported;
}

gboolean ald_invoker_is_in_acquisition(AldInvoker *invoker)
{
    return invoker->is_in_acquisition;
}

void ald_invoker_start_acquisition(AldInvoker *invoker, GError **error)
{
    g_return_if_fail(invoker->is_in_acquisition == FALSE);
    if (invoker->current_stream == NULL)
    {
        invoker->current_stream = arv_camera_create_stream(invoker->camera,
                                                           invoker->buf_strategy == ALD_INVOKER_BUFFER_DEFAULT ? InPlaceStreamCallback : CopyStreamCallback,
                                                           invoker, error);
        arv_stream_set_emit_signals(invoker->current_stream, TRUE);
        guint size = arv_camera_get_payload(invoker->camera, NULL);
        for (size_t i = 0; i < invoker->buffer_count; i++)
        {
            arv_stream_push_buffer(invoker->current_stream, arv_buffer_new(size, NULL));
        }
    }
    arv_camera_start_acquisition(invoker->camera, error);
    invoker->is_in_acquisition = TRUE;
}

void ald_invoker_stop_acquisition(AldInvoker *invoker, GError **error)
{
    g_return_if_fail(invoker->is_in_acquisition);
    arv_camera_stop_acquisition(invoker->camera, error);
    invoker->is_in_acquisition = FALSE;
}

void ald_invoker_reset_thread(AldInvoker *invoker)
{
    g_return_if_fail(invoker->current_stream != NULL);
    arv_stream_stop_thread(invoker->current_stream, TRUE);
    g_object_unref(invoker->current_stream);
    invoker->current_stream = NULL;
}

static void CopyStreamCallback(void *user_data, ArvStreamCallbackType type, ArvBuffer *buffer)
{
    switch (type)
    {
    case ARV_STREAM_CALLBACK_TYPE_INIT:
        arv_make_thread_high_priority();
        break;
    case ARV_STREAM_CALLBACK_TYPE_BUFFER_DONE:
        AldInvoker *invoker = (AldInvoker *)user_data;
        AldBufferInfo info;
        ArvBuffer *buf = arv_stream_pop_buffer(invoker->current_stream);
        size_t size;
        const void *src = arv_buffer_get_image_data(buf, &size);
        gpointer des = g_malloc(size);
        memcpy_s(des, size, src, size);
        info.data = des;
        info.size = size;
        info.format = arv_buffer_get_image_pixel_format(buf);
        info.width = arv_buffer_get_image_width(buf);
        info.height = arv_buffer_get_image_height(buf);
        info.system_stamp = arv_buffer_get_system_timestamp(buf);
        info.is_copy = invoker->buf_strategy == ALD_INVOKER_BUFFER_COPY;
        arv_stream_push_buffer(invoker->current_stream, buf);
        invoker->cb(info);
        break;
    default:
        break;
    }
}

static void InPlaceStreamCallback(void *user_data, ArvStreamCallbackType type, ArvBuffer *buffer)
{
    switch (type)
    {
    case ARV_STREAM_CALLBACK_TYPE_INIT:
        arv_make_thread_high_priority();
        break;
    case ARV_STREAM_CALLBACK_TYPE_BUFFER_DONE:
        AldInvoker *invoker = (AldInvoker *)user_data;
        AldBufferInfo info;
        ArvBuffer *buf = arv_stream_pop_buffer(invoker->current_stream);
        size_t size;
        const void *src = arv_buffer_get_image_data(buf, &size);
        info.data = src;
        info.size = size;
        info.format = arv_buffer_get_image_pixel_format(buf);
        info.width = arv_buffer_get_image_width(buf);
        info.height = arv_buffer_get_image_height(buf);
        info.system_stamp = arv_buffer_get_system_timestamp(buf);
        info.is_copy = invoker->buf_strategy == ALD_INVOKER_BUFFER_COPY;
        arv_stream_push_buffer(invoker->current_stream, buf);
        invoker->cb(info);
        break;
    default:
        break;
    }
}

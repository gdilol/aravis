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
    AldInvokerCallback cb;
    AldInvokerAcquisitionStrategy acq_strategy;
    char *hardware_trigger_source;
    AldInvokerBufferStrategy buf_strategy;
    int buffer_count;
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
    instance->cb = NULL;
    instance->acq_strategy = ALD_INVOKER_ACQUISITION_STRATEGY_CONTINUOUS_RUN;
    instance->buf_strategy = ALD_INVOKER_BUFFER_DEFAULT;
    instance->buffer_count = 4;
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

void ald_invoker_set_buffer_count(AldInvoker *invoker, int buffer_count)
{
    invoker->buffer_count = buffer_count;
    g_return_if_fail(invoker->current_stream != NULL);

    guint size = arv_camera_get_payload(invoker->camera, NULL);
    arv_stream_stop_thread(invoker->current_stream, TRUE);
    for (size_t i = 0; i < buffer_count; i++)
    {
        arv_stream_push_buffer(invoker->current_stream, arv_buffer_new(size, NULL));
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
    ald_invoker_stop_acquisition(invoker);
    ArvCamera *camera = invoker->camera;
    arv_camera_set_acquisition_mode(camera, arv_acquisition_mode_from_string("Continuous"), error);

    switch (invoker->acq_strategy)
    {
    case ALD_INVOKER_ACQUISITION_STRATEGY_CONTINUOUS_RUN:
        arv_camera_set_trigger(camera, "AcquisitionStart", error);
        arv_camera_set_trigger_source(camera, "Software", error);
        break;
    case ALD_INVOKER_ACQUISITION_STRATEGY_SOFTWARETRIGGER:
        arv_camera_set_trigger(camera, "FrameBurstStart", error);
        arv_camera_set_trigger_source(camera, "Software", error);
        break;
    case ALD_INVOKER_ACQUISITION_STRATEGY_HARDWARETRIGGER:
        arv_camera_set_trigger(camera, "FrameBurstStart", error);
        g_return_if_fail(invoker->hardware_trigger_source != NULL);
        arv_camera_set_trigger_source(camera, invoker->hardware_trigger_source, error);
        break;
    default:
        break;
    }
}

void ald_invoker_set_buffer_strategy(AldInvoker *invoker, AldInvokerBufferStrategy strategy)
{
    invoker->buf_strategy = strategy;
    g_return_if_fail(invoker->current_stream != NULL);
    ald_invoker_reset(invoker);
}

void ald_invoker_start_acquisition(AldInvoker *invoker, GError **error)
{
    if (invoker->current_stream == NULL)
    {
        invoker->current_stream = arv_camera_create_stream(invoker->camera,
                                                           invoker->buf_strategy == ALD_INVOKER_BUFFER_DEFAULT ? InPlaceStreamCallback : CopyStreamCallback,
                                                           invoker, error);
        guint size = arv_camera_get_payload(invoker->camera, NULL);
        for (size_t i = 0; i < invoker->buffer_count; i++)
        {
            arv_stream_push_buffer(invoker->current_stream, arv_buffer_new(size, NULL));
        }
    }
    arv_stream_start_thread(invoker->current_stream);
    if (invoker->acq_strategy == ALD_INVOKER_ACQUISITION_STRATEGY_CONTINUOUS_RUN)
    {
        arv_camera_software_trigger(invoker->camera, error);
    }
}

void ald_invoker_stop_acquisition(AldInvoker *invoker)
{
    g_return_if_fail(invoker->current_stream != NULL);
    arv_stream_stop_thread(invoker->current_stream, FALSE);
}

void ald_invoker_reset(AldInvoker *invoker)
{
    arv_stream_stop_thread(invoker->current_stream, TRUE);
    g_object_unref(invoker->current_stream);
    invoker->current_stream = NULL;
}

static void CopyStreamCallback(void *user_data, ArvStreamCallbackType type, ArvBuffer *buffer)
{
    AldInvoker *invoker = (AldInvoker *)user_data;
    g_return_if_fail(type == ARV_STREAM_CALLBACK_TYPE_BUFFER_DONE);
    AldBufferInfo info;
    ArvBuffer *buf = arv_stream_pop_buffer(invoker->current_stream);
    g_return_if_fail(ARV_IS_BUFFER(buffer));
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
}

static void InPlaceStreamCallback(void *user_data, ArvStreamCallbackType type, ArvBuffer *buffer)
{
    AldInvoker *invoker = (AldInvoker *)user_data;
    g_return_if_fail(type == ARV_STREAM_CALLBACK_TYPE_BUFFER_DONE);
    AldBufferInfo info;
    ArvBuffer *buf = arv_stream_pop_buffer(invoker->current_stream);
    g_return_if_fail(ARV_IS_BUFFER(buffer));
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
}

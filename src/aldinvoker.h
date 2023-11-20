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

#ifndef ALD_INVOKER_H
#define ALD_INVOKER_H

#if !defined(ARV_H_INSIDE) && !defined(ARAVIS_COMPILATION)
#error "Only <arv.h> can be included directly."
#endif

#include <arvapi.h>
#include <arvtypes.h>
#include <arvcamera.h>

G_BEGIN_DECLS

/**
 *AldInvokerBufferStrategy:
 * @ALD_INVOKER_BUFFER_COPY: copy the retrieved buffer into a new allocated memory
 * @ALD_INVOKER_BUFFER_INPLACE: dont copy the data, transfer the data of buffer object
 */
typedef enum
{
    ALD_INVOKER_BUFFER_INPLACE = 0,
    ALD_INVOKER_BUFFER_COPY,
    ALD_INVOKER_BUFFER_DEFAULT = ALD_INVOKER_BUFFER_INPLACE
} AldInvokerBufferStrategy;

typedef enum
{
    ALD_INVOKER_ACQUISITION_STRATEGY_CONTINUOUS_RUN = 0,
    ALD_INVOKER_ACQUISITION_STRATEGY_SOFTWARETRIGGER,
    ALD_INVOKER_ACQUISITION_STRATEGY_HARDWARETRIGGER
} AldInvokerAcquisitionStrategy;

typedef struct
{
    /* data */
    const void *data;
    size_t size;
    ArvPixelFormat format;
    int width;
    int height;
    guint64 system_stamp;
    gboolean is_copy;
    gboolean disposed; // never use
} AldBufferInfo;

typedef void (*AldInvokerCallback)(AldBufferInfo info);

#define ALD_TYPE_INVOKER ald_invoker_get_type()

ARV_API G_DECLARE_FINAL_TYPE(AldInvoker, ald_invoker, ALD, INVOKER, GObject);

ARV_API AldInvoker *ald_invoker_new(ArvCamera *camera, AldInvokerCallback cb);

ARV_API void ald_invoker_set_buffer_count(AldInvoker *invoker, int buffer_count);

ARV_API void ald_invoker_set_acquisition_strategy(AldInvoker *invoker, AldInvokerAcquisitionStrategy strategy, GError** error);

ARV_API void ald_invoker_set_hardware_trigger_source(AldInvoker *invoker, char *source, GError **error)

ARV_API void ald_invoker_set_buffer_strategy(AldInvoker *invoker, AldInvokerBufferStrategy strategy);

ARV_API void ald_invoker_set_frame_count_per_trigger(AldInvoker *invoker, int count);

ARV_API void ald_invoker_start_acquisition(AldInvoker *invoker, GError** error);

ARV_API void ald_invoker_stop_acquisition(AldInvoker *invoker);

ARV_API void ald_invoker_reset(AldInvoker *invoker);

G_END_DECLS

#endif
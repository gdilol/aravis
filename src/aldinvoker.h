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
    ALD_INVOKER_BUFFER_COPY,
    ALD_INVOKER_BUFFER_INPLACE
} AldInvokerBufferStrategy;

typedef struct
{
    /* data */
    void *buf;
    void *data;
    int width;
    int height;
    int stride;
    int channel;
    gboolean disposed;//never used
} AldBufferInfo;

typedef void (*AldInvokerCallback)(AldBufferInfo* info);

#define ALD_TYPE_INVOKER (ald_invoker_get_type())

ARV_API G_DECLARE_FINAL_TYPE(AldInvoker, ald_invoker, ALD, INVOKER, GObject);

ARV_API AldInvoker *ald_invoker_new(ArvCamera *camera);

ARV_API int ald_invoker_get_buffer_count(AldInvoker* invoker);

ARV_API void ald_invoker_set_buffer_count(AldInvoker* invoker, int buffer_count);

ARV_API void ald_invoker_camera_updated();

ARV_API ArvStream *ald_invoker_create_stream(AldInvoker *invoker, AldInvokerBufferStrategy strategy);

G_END_DECLS
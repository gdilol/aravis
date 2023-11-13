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

#include <aldinvokerprivate.h>

static void CopyStreamCallback(void *user_data, ArvStreamCallbackType type, ArvBuffer *buffer)
{
}

static void InPlaceStreamCallback(void *user_data, ArvStreamCallbackType type, ArvBuffer *buffer)
{
}

/**
 * @brief construct a invoker class which provides fast reach to deal with the buffer
 *
 * @param camera
 * @return AldInvoker*, a invoker class
 */
ARV_API AldInvoker *ald_invoker_new(ArvCamera *camera)
{
    AldInvoker *invoker = g_object_new(ALD_TYPE_INVOKER, NULL);
}

ARV_API int ald_invoker_get_buffer_count(AldInvoker *invoker)
{
    return invoker->priv.buffer_count;
}

ARV_API void ald_invoker_set_buffer_count(AldInvoker *invoker, int buffer_count)
{
    invoker->priv.buffer_count = buffer_count;
}
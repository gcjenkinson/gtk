/* GDK - The GIMP Drawing Kit
 * Copyright (C) 2023 Red Hat, Inc.
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "gdksubsurfaceprivate.h"
#include "gdksurfaceprivate.h"
#include "gdktexture.h"

G_DEFINE_TYPE (GdkSubsurface, gdk_subsurface, G_TYPE_OBJECT)

static void
gdk_subsurface_init (GdkSubsurface *self)
{
}

static void
gdk_subsurface_finalize (GObject *object)
{
  GdkSubsurface *subsurface = GDK_SUBSURFACE (object);

  g_ptr_array_remove (subsurface->parent->subsurfaces, subsurface);
  g_clear_object (&subsurface->parent);

  G_OBJECT_CLASS (gdk_subsurface_parent_class)->finalize (object);
}

static void
gdk_subsurface_class_init (GdkSubsurfaceClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = gdk_subsurface_finalize;
}

GdkSurface *
gdk_subsurface_get_parent (GdkSubsurface *subsurface)
{
  g_return_val_if_fail (GDK_IS_SUBSURFACE (subsurface), NULL);

  return subsurface->parent;
}

static void
remove_subsurface (GdkSubsurface *subsurface)
{
  GdkSurface *parent = subsurface->parent;

  if (parent->subsurfaces_above == subsurface)
    parent->subsurfaces_above = subsurface->sibling_above;
  if (parent->subsurfaces_below == subsurface)
    parent->subsurfaces_below = subsurface->sibling_below;

  if (subsurface->sibling_above)
    subsurface->sibling_above->sibling_below = subsurface->sibling_below;
  if (subsurface->sibling_below)
    subsurface->sibling_below->sibling_above = subsurface->sibling_above;

  subsurface->sibling_above = NULL;
  subsurface->sibling_below = NULL;
}

static void
insert_subsurface (GdkSubsurface *subsurface,
                   gboolean       above,
                   GdkSubsurface *sibling)
{
  GdkSurface *parent = subsurface->parent;

  subsurface->above_parent = sibling->above_parent;

  if (above)
    {
      subsurface->sibling_above = sibling->sibling_above;
      sibling->sibling_above = subsurface;
      subsurface->sibling_below = sibling;
      if (subsurface->sibling_above)
        subsurface->sibling_above->sibling_below = subsurface;

      if (parent->subsurfaces_below == sibling)
        parent->subsurfaces_below = subsurface;
    }
  else
    {
      subsurface->sibling_below = sibling->sibling_below;
      sibling->sibling_below = subsurface;
      subsurface->sibling_above = sibling;
      if (subsurface->sibling_below)
        subsurface->sibling_below->sibling_above = subsurface;

      if (parent->subsurfaces_above == sibling)
        parent->subsurfaces_above = subsurface;
    }
}

gboolean
gdk_subsurface_attach (GdkSubsurface         *subsurface,
                       GdkTexture            *texture,
                       const graphene_rect_t *rect,
                       gboolean               above,
                       GdkSubsurface         *sibling)
{
  GdkSurface *parent = subsurface->parent;

  g_return_val_if_fail (GDK_IS_SUBSURFACE (subsurface), FALSE);
  g_return_val_if_fail (GDK_IS_TEXTURE (texture), FALSE);
  g_return_val_if_fail (rect != NULL, FALSE);
  g_return_val_if_fail (sibling != subsurface, FALSE);
  g_return_val_if_fail (sibling == NULL || GDK_IS_SUBSURFACE (sibling), FALSE);
  g_return_val_if_fail (sibling == NULL || sibling->parent == subsurface->parent, FALSE);

  remove_subsurface (subsurface);

  if (sibling)
    {
      insert_subsurface (subsurface, above, sibling);
    }
  else
    {
      sibling = above ? parent->subsurfaces_above : parent->subsurfaces_below;

      if (sibling)
        {
          insert_subsurface (subsurface, !above, sibling);
        }
      else
        {
          subsurface->above_parent = above;

          if (above)
            parent->subsurfaces_above = subsurface;
          else
            parent->subsurfaces_below = subsurface;
        }
    }

  return GDK_SUBSURFACE_GET_CLASS (subsurface)->attach (subsurface, texture, rect, above, sibling);
}

void
gdk_subsurface_detach (GdkSubsurface *subsurface)
{
  g_return_if_fail (GDK_IS_SUBSURFACE (subsurface));

  remove_subsurface (subsurface);

  GDK_SUBSURFACE_GET_CLASS (subsurface)->detach (subsurface);
}

GdkTexture *
gdk_subsurface_get_texture (GdkSubsurface *subsurface)
{
  g_return_val_if_fail (GDK_IS_SUBSURFACE (subsurface), NULL);

  return GDK_SUBSURFACE_GET_CLASS (subsurface)->get_texture (subsurface);
}

void
gdk_subsurface_get_rect (GdkSubsurface   *subsurface,
                         graphene_rect_t *rect)
{
  g_return_if_fail (GDK_IS_SUBSURFACE (subsurface));
  g_return_if_fail (rect != NULL);

  GDK_SUBSURFACE_GET_CLASS (subsurface)->get_rect (subsurface, rect);
}

gboolean
gdk_subsurface_is_above_parent (GdkSubsurface *subsurface)
{
  g_return_val_if_fail (GDK_IS_SUBSURFACE (subsurface), TRUE);

  return subsurface->above_parent;
}

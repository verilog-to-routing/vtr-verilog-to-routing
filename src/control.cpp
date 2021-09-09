/*
 * Copyright 2019 University of Toronto
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Authors: Mario Badr, Sameh Attia and Tanner Young-Schultz
 */

#include "ezgl/control.hpp"

#include "ezgl/camera.hpp"
#include "ezgl/canvas.hpp"

namespace ezgl {

static rectangle zoom_in_world(point2d zoom_point, rectangle world, double zoom_factor)
{
  double const left = zoom_point.x - (zoom_point.x - world.left()) / zoom_factor;
  double const bottom = zoom_point.y + (world.bottom() - zoom_point.y) / zoom_factor;

  double const right = zoom_point.x + (world.right() - zoom_point.x) / zoom_factor;
  double const top = zoom_point.y - (zoom_point.y - world.top()) / zoom_factor;

  return {{left, bottom}, {right, top}};
}

static rectangle zoom_out_world(point2d zoom_point, rectangle world, double zoom_factor)
{
  double const left = zoom_point.x - (zoom_point.x - world.left()) * zoom_factor;
  double const bottom = zoom_point.y + (world.bottom() - zoom_point.y) * zoom_factor;

  double const right = zoom_point.x + (world.right() - zoom_point.x) * zoom_factor;
  double const top = zoom_point.y - (zoom_point.y - world.top()) * zoom_factor;

  return {{left, bottom}, {right, top}};
}

void zoom_in(canvas *cnv, double zoom_factor)
{
  point2d const zoom_point = cnv->get_camera().get_world().center();
  rectangle const world = cnv->get_camera().get_world();

  cnv->get_camera().set_world(zoom_in_world(zoom_point, world, zoom_factor));
  cnv->redraw();
}

void zoom_in(canvas *cnv, point2d zoom_point, double zoom_factor)
{
  zoom_point = cnv->get_camera().widget_to_world(zoom_point);
  rectangle const world = cnv->get_camera().get_world();

  cnv->get_camera().set_world(zoom_in_world(zoom_point, world, zoom_factor));
  cnv->redraw();
}

void zoom_out(canvas *cnv, double zoom_factor)
{
  point2d const zoom_point = cnv->get_camera().get_world().center();
  rectangle const world = cnv->get_camera().get_world();

  cnv->get_camera().set_world(zoom_out_world(zoom_point, world, zoom_factor));
  cnv->redraw();
}

void zoom_out(canvas *cnv, point2d zoom_point, double zoom_factor)
{
  zoom_point = cnv->get_camera().widget_to_world(zoom_point);
  rectangle const world = cnv->get_camera().get_world();

  cnv->get_camera().set_world(zoom_out_world(zoom_point, world, zoom_factor));
  cnv->redraw();
}

void zoom_fit(canvas *cnv, rectangle region)
{
  cnv->get_camera().set_world(region);
  cnv->redraw();
}

void translate(canvas *cnv, double dx, double dy)
{
  rectangle new_world = cnv->get_camera().get_world();
  new_world += ezgl::point2d(dx, dy);

  cnv->get_camera().set_world(new_world);
  cnv->redraw();
}

void translate_up(canvas *cnv, double translate_factor)
{
  rectangle new_world = cnv->get_camera().get_world();
  double dy = new_world.height() / translate_factor;

  translate(cnv, 0.0, dy);
}

void translate_down(canvas *cnv, double translate_factor)
{
  rectangle new_world = cnv->get_camera().get_world();
  double dy = new_world.height() / translate_factor;

  translate(cnv, 0.0, -dy);
}

void translate_left(canvas *cnv, double translate_factor)
{
  rectangle new_world = cnv->get_camera().get_world();
  double dx = new_world.width() / translate_factor;

  translate(cnv, -dx, 0.0);
}

void translate_right(canvas *cnv, double translate_factor)
{
  rectangle new_world = cnv->get_camera().get_world();
  double dx = new_world.width() / translate_factor;

  translate(cnv, dx, 0.0);
}
}

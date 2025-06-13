.. _ezgl:

====
EZGL
====

EZGL is a graphics layer on top of version 3.x of the GTK graphics library. It allows drawing in an arbitrary 2D world coordinate space (instead of in pixel coordinates), handles panning and zooming automatically, and provides easy-to-use functions for common tasks like setting up a window, setting graphics attributes (like colour and line style) and drawing primitives (like lines and polygons). Most of VPR's drawing is performed in ezgl, and GTK functionality not exposed by ezgl can still be accessed by directly calling the relevant gtk functions.

.. toctree::
   :maxdepth: 1

   application
   callback
   camera
   canvas
   color
   control
   graphics
   point
   rectangle
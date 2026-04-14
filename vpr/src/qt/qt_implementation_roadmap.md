[DONE] - Proper render implementation (QPainter refactor + API redesign)
[DONE] - Deffered render + batch render optimization
[IN_PROGRESS] - GPU close renderer
[IN_PROGRESS] - stress test of new render impl, final performance test GTK vs Qt on complex graphic scene
[DONE] - Remove VPR_QT macro and GTK code sections under the #else ... #endif, from now Qt is only single framework used in backend.
[ ] - Rename and clean function arguments or use QWidget:: API directly for gtk calls -> remove qt_compat.h layer completely

STEPS PLANNED TO BE DONE BEFORE Friday (10 April)

[ ] - Convert glade main.ui to Qt designed proper UI format and remove QGladeLoader as temporary solution.
[ ] - Full code comments coverage

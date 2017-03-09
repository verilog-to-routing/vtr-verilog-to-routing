#include "graphics_state.h"

#ifdef X11
t_x11_state *t_x11_state::instance = NULL;

t_x11_state::t_x11_state()
{
	instance = this;
}

t_x11_state::~t_x11_state()
{
	instance = NULL;
}

t_x11_state *t_x11_state::getInstance()
{
	return instance;
}
#endif // X11

#ifdef WIN32
t_win32_state *t_win32_state::instance = NULL;

t_win32_state::t_win32_state()
{
	instance = this;
}

t_win32_state::~t_win32_state()
{
	instance = NULL;
}

t_win32_state *t_win32_state::getInstance()
{
	return instance;
}
#endif // WIN32

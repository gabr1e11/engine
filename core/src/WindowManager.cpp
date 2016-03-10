/**
 * @class	WindowManager
 * @brief	Interface for window manager management
 *
 * @author	Roberto Cano (http://www.robertocano.es)
 */

#include "WindowManager.hpp"
#include "GLFWWindowManager.hpp"

WindowManager *WindowManager::_windowManager = NULL;

WindowManager *WindowManager::GetInstance(WindowManagerType type)
{
	// TODO: Check here that the type of the already instantiated manager
	//  is the same as the requested one
	switch (type)
	{
		case WINDOW_MANAGER_GLFW:
			/* Take care of creating only one of this, as it is using
			 * static callbacks */
			if (_windowManager == NULL) {
				_windowManager = new GLFWWindowManager();
			}
			return _windowManager;
		default:
			return NULL;
	}
}

void WindowManager::DisposeInstance(void)
{
    delete _windowManager;
    _windowManager = NULL;
}

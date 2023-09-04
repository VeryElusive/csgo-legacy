#pragma once
#include "../../../utils/render.h"

namespace Wrappers::Renderer {
	void Rect( int x, int y, int w, int h, Color color ) {
		Render::Rectangle( x, y, w, h, color );
	}

	void RectFilled( int x, int y, int w, int h, Color color ) {
		Render::FilledRectangle( x, y, w, h, color );
	}
}
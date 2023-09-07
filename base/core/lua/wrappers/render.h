#pragma once
#include "../../../utils/render.h"

namespace Wrappers::Renderer {
	void Rect( int x, int y, int w, int h, Color color ) {
		Render::Rectangle( x, y, w, h, color );
	}

	void RectFilled( int x, int y, int w, int h, Color color ) {
		Render::FilledRectangle( x, y, w, h, color );
	}

	void FilledRoundedBox( int x, int y, int w, int h, int points, int radius, Color color ) {
		Render::FilledRoundedBox( { x, y }, { w, h }, points, radius, color );
	}

	void RoundedBox( int x, int y, int w, int h, int points, int radius, Color color ) {
		Render::RoundedBox( { x, y }, { w, h }, points, radius, color );
	}	
	
	void Line( int x, int y, int x2, int y2, Color color ) {
		Render::Line( { x, y }, { x2, y2 }, color );
	}
}
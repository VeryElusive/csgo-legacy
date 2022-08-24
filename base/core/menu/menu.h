#pragma once
#include "../../utils/render.h"
#include "../input_manager.h"
#include "../config.h"
#include "../variables.h"
#include <memory>
#include <optional>

struct multi_item_t {
	std::string name;
	bool* value;
};
struct hsv {
	hsv( float hue, float sat, float val ) {
		this->hue = hue;
		this->sat = sat;
		this->val = val;
	}

	hsv( ) {
		hue = 1.f;
		sat = 1.f;
		sat = 1.f;
	}
	float hue;
	float sat;
	float val;
};
#include "group/group.h"

struct multidropinfo {
	multidropinfo( Vector2D pos, Vector2D size, std::vector<multi_item_t> options ) {
		this->pos = pos;
		this->size = size;
		this->options = options;
	}
	Vector2D pos;
	Vector2D size;
	std::vector<multi_item_t> options;
};

struct dropinfo {
	dropinfo( Vector2D pos, Vector2D size, std::vector<std::string> options, int value ) {
		this->pos = pos;
		this->size = size;
		this->options = options;
		this->value = value;
	}
	Vector2D pos;
	Vector2D size;
	std::vector<std::string> options;
	int value;
};

struct colorpickerinfo {
	colorpickerinfo( Vector2D pos, float hue, Color Color, bool alpha ) {
		this->pos = pos;
		this->hue = hue;
		this->Color = Color;
		this->alpha = alpha;
	}
	Vector2D pos;
	Color Color;
	float hue;
	bool alpha;
};

struct keybindinfo {
	keybindinfo( Vector2D pos, keybind_t key ) {
		this->pos = pos;
		this->key = key;
	}
	Vector2D pos;
	keybind_t key;
};

namespace Menu {
	inline bool Opened{true };
	inline bool DraggingMouse{ };
	inline bool Typing{ };
	inline bool FrameAfterFocus{ };

	inline int CachedGroupSize{ };

	inline float MenuAlpha = 0.f;

	inline const char* OpenedID{ "" };

	inline std::optional<multidropinfo> MultiDropdown;
	inline std::optional<dropinfo> Dropdown;
	inline std::optional<colorpickerinfo> ColorPicker;
	inline std::optional<keybindinfo> KeyBind;

	inline Vector2D Size = { 800, 550 };
	inline Vector2D Pos = { 500, 200 };

	inline Vector2D CursorPos = { 500, 200 };

	inline Color OutlineLight = Color( 65, 65, 65 );
	inline Color BackgroundCol = Color( 28, 28, 28 );
	inline Color GroupCol = Color( 32, 32, 32 );
	inline Color ControlCol = Color( 32, 32, 32 );
	inline Color Accent2Col = Color( 182, 139, 252 );
	inline Color AccentCol = Color( 115, 155, 255 );

	void render( );

	void NewGroupRow( float append_x = 0 );
	void GetElements( );
	void RenderSubtabs( std::vector<std::string> subTabs, int& activeSubtab );

	void DrawRage( );
	void DrawVisual( );
	void DrawMisc( );
	void DrawConfig( );


	hsv rgb2hsv( Color a );
	Color hsv2rgb( hsv hsv );
}
#pragma once
#include "../menu.h"
class MenuGroup {
public:
	Vector2D OldCursorPos;
	Vector2D size;
	float scroll;

	int x1, y1, x2, y2;

	void Begin( const char* name, Vector2D size );
	void End( bool x = true );

	void ListBox( const char* name, std::deque<std::string> options, int& Opt, Vector2D Size );
	void TextInput( std::string& val );
	bool Button( const char* name );
	void Checkbox( const char* name, bool& value );
	void Label( const char* name );
	void Keybind( const char* name, keybind_t& value );
	void ColorPicker( const char* name, Color& value, bool alpha = true );
	void Combo( const char* name, int& value, std::vector<std::string> options );
	void MultiCombo( const char* name, std::vector<multi_item_t> options );
	void Slider( const char* name, int& value, int min, int max );
	void Slider( const char* name, float& value, float min, float max );
};
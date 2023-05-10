#pragma once
#include "../havoc.h"
#include <unordered_map>
#include "../sdk/datatypes/color.h"

enum e_tex_font_creation_flags {
	D3DFONT_BOLD = ( 1 << 0 ),
	D3DFONT_ITALIC = ( 1 << 1 ),
	D3DFONT_UNDERLINE = ( 1 << 2 ),
	D3DFONT_STRIKEOUT = ( 1 << 3 ),
	D3DFONT_ANTIALIAS = ( 1 << 4 )
};

enum e_font_drawing_flags {
	//FONT_RIGHT_ALIGN = ( 1 << 0 ),
	FONT_CENTERED_X = ( 1 << 1 ),
	//FONT_CENTERED_Y = ( 1 << 2 ),
	FONT_OUTLINE = ( 1 << 3 ),
	FONT_DROPSHADOW = ( 1 << 4 ),
	FONT_COLOR_FORMAT = ( 1 << 5 )
};

struct tex_font_t {
	tex_font_t( ) = default;
	tex_font_t( const wchar_t* name, int height, int creation_flags = 0, int highest_char = 128 ) :
		name( name ), height( height ), creation_flags( creation_flags ), highest_char( highest_char ) {
		if ( this->height > 60 )
			this->tex_width = this->tex_height = 8192;
		else if ( this->height > 30 )
			this->tex_width = this->tex_height = 4096;
		else if ( this->height > 15 )
			this->tex_width = this->tex_height = 2048;
		else
			this->tex_width = this->tex_height = 1024;

		this->glyph_coords.resize( highest_char - 32 );
	}

	~tex_font_t( ) { this->release( ); }

	void init( IDirect3DDevice9* dev );
	void release( ) { if ( tex ) tex->Release( ), tex = nullptr; }

	Vector2D get_text_extent( const wchar_t* txt ) const;

	void draw_text( float x, float y, const wchar_t* txt, uint32_t col, int drawing_flags = 0, int shadow_alpha = 255 );

private:
	std::wstring name;
	int height = 0, creation_flags = 0, highest_char = 128;
	IDirect3DTexture9* tex = nullptr;
	int tex_width = 0, tex_height = 0, glyph_spacing = 0;
	float tex_scale = 1.f;
	std::vector<std::array<float, 4>> glyph_coords;
};

struct font_t {
	font_t( const wchar_t* name, int creation_flags = 0, int highest_char = 128 ) : name( name ), creation_flags( creation_flags ), highest_char( highest_char ) {}

	void init( IDirect3DDevice9* dev ) { this->dev = dev; for ( auto& tfe : this->tex_fonts ) tfe.second.init( this->dev ); }
	void release( ) { for ( auto& tfe : this->tex_fonts ) tfe.second.release( ); this->tex_fonts.clear( ); }

	Vector2D get_text_size( int height, const wchar_t* wstr );
	Vector2D get_text_size( int height, const std::string& str );

	void draw_text( int height, const Vector2D& pos, const wchar_t* txt, Color col, int drawing_flags = 0, int shadow_alpha = 255 );

	void draw_text( int height, const Vector2D& pos, const std::string& txt, Color col, int drawing_flags = 0, int shadow_alpha = 255 ) {
		this->draw_text( height, pos, std::wstring( txt.begin( ), txt.end( ) ).c_str( ), col, drawing_flags, shadow_alpha );
	}

private:
	std::wstring name;
	int creation_flags = 0, highest_char = 128;
	std::unordered_map<int, tex_font_t> tex_fonts;
	IDirect3DDevice9* dev = nullptr;
};

namespace Fonts {
	extern std::shared_ptr<font_t> WeaponIcon;
	extern std::shared_ptr<font_t> Verdana;
	extern std::shared_ptr<font_t> Pixel;
	extern std::shared_ptr<font_t> Tahoma;
	extern std::shared_ptr<font_t> MenuTabs;
}
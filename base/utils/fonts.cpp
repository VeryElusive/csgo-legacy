#include "fonts.h"
#include "render.h"

void tex_font_t::init( IDirect3DDevice9* dev ) {
	D3DCAPS9 d3d_caps; dev->GetDeviceCaps( &d3d_caps );

	if ( this->tex_width > d3d_caps.MaxTextureWidth ) {
		this->tex_scale = static_cast< float >( d3d_caps.MaxTextureWidth ) / static_cast< float >( this->tex_width );
		this->tex_width = this->tex_height = d3d_caps.MaxTextureWidth;
	}

	auto hr = dev->CreateTexture( this->tex_width, this->tex_height, 1, D3DUSAGE_DYNAMIC, D3DFMT_A4R4G4B4, D3DPOOL_DEFAULT, &this->tex, nullptr );
	if ( FAILED( hr ) ) return; // maybe throw an exception?

	HDC dc = CreateCompatibleDC( nullptr ); if ( !dc ) return; // maybe throw an exception?

	BITMAPINFO bmi;
	std::memset( &bmi.bmiHeader, 0, sizeof BITMAPINFOHEADER );
	bmi.bmiHeader.biSize = sizeof BITMAPINFOHEADER;
	bmi.bmiHeader.biWidth = this->tex_width;
	bmi.bmiHeader.biHeight = -static_cast< int >( this->tex_height );
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biBitCount = 32;

	int* bitmap_bits = nullptr;
	auto h_bitmap = CreateDIBSection( dc, &bmi, DIB_RGB_COLORS, reinterpret_cast< void** >( &bitmap_bits ), 0, 0 );
	if ( !h_bitmap ) return; // maybe throw an exception?

	SetMapMode( dc, MM_TEXT );

	int height = -MulDiv( this->height, static_cast< int >( GetDeviceCaps( dc, LOGPIXELSY ) * this->tex_scale ), 72 );

	auto h_font = CreateFontW( height, 0, 0, 0,
		this->creation_flags & D3DFONT_BOLD ? FW_BOLD : FW_NORMAL,
		this->creation_flags & D3DFONT_ITALIC,
		this->creation_flags & D3DFONT_UNDERLINE,
		this->creation_flags & D3DFONT_STRIKEOUT,
		DEFAULT_CHARSET, OUT_TT_ONLY_PRECIS, CLIP_DEFAULT_PRECIS,
		this->creation_flags & D3DFONT_ANTIALIAS ? CLEARTYPE_NATURAL_QUALITY : NONANTIALIASED_QUALITY,
		DEFAULT_PITCH,
		this->name.c_str( ) );

	if ( !h_font ) return; // maybe throw an exception?

	auto h_old_bitmap = SelectObject( dc, h_bitmap ), h_old_font = SelectObject( dc, h_font );
	SetTextColor( dc, RGB( 255, 255, 255 ) );
	SetBkColor( dc, 0 );
	SetTextAlign( dc, TA_TOP );

	wchar_t str[ 2 ] = L" ";
	SIZE size; GetTextExtentPoint32W( dc, str, 1, &size );
	int x = this->glyph_spacing = static_cast< int >( std::ceil( size.cy * 0.3f ) ), y = 0;

	for ( int c = 32; c < this->highest_char - 1; ++c ) {
		str[ 0 ] = c;
		GetTextExtentPoint32W( dc, str, 1, &size );

		if ( static_cast< int >( x + size.cx + this->glyph_spacing ) > this->tex_width )
			x = this->glyph_spacing, y += size.cy + 1;

		ExtTextOutW( dc, x, y, ETO_OPAQUE, nullptr, str, 1, nullptr );

		this->glyph_coords[ c - 32 ][ 0 ] = static_cast< float >( x - this->glyph_spacing ) / this->tex_width;
		this->glyph_coords[ c - 32 ][ 1 ] = static_cast< float >( y ) / this->tex_height;
		this->glyph_coords[ c - 32 ][ 2 ] = static_cast< float >( x + size.cx + this->glyph_spacing ) / this->tex_width;
		this->glyph_coords[ c - 32 ][ 3 ] = static_cast< float >( y + size.cy ) / this->tex_height;

		x += size.cx + 2 * this->glyph_spacing;
	}

	D3DLOCKED_RECT d3dlr;
	this->tex->LockRect( 0, &d3dlr, nullptr, 0 );
	auto dst_row = static_cast< uint8_t* >( d3dlr.pBits );

	for ( y = 0; y < this->tex_height; ++y ) {
		auto dst = reinterpret_cast< uint16_t* >( dst_row );

		for ( x = 0; x < this->tex_width; ++x ) {
			auto alpha = static_cast< uint8_t >( ( bitmap_bits[ this->tex_width * y + x ] & 0xFF ) >> 4 );
			*dst++ = alpha > 0 ? static_cast< uint16_t >( ( alpha << 12 ) | 0xfff ) : 0;
		}

		dst_row += d3dlr.Pitch;
	}

	this->tex->UnlockRect( 0 );
	SelectObject( dc, h_old_bitmap ), SelectObject( dc, h_old_font );
	DeleteObject( h_bitmap ), DeleteObject( h_font ), DeleteDC( dc );
}

Vector2D tex_font_t::get_text_extent( const wchar_t* txt ) const {
	auto row_width = 0.f, row_height = ( this->glyph_coords[ 0 ][ 3 ] - this->glyph_coords[ 0 ][ 1 ] ) * tex_height, width = 0.f, height = row_height;

	while ( *txt ) {
		auto c = static_cast< int >( *txt++ );
		if ( c == '\n' ) { row_width = 0.f, height += row_height; continue; }
		if ( c - 32 < 0 || c - 32 >= this->highest_char - 32 ) continue;
		if ( c == '#' ) { for ( int i = 0; i < 8; ++i ) ++txt; continue; }

		auto x = this->glyph_coords[ c - 32 ][ 0 ], x0 = this->glyph_coords[ c - 32 ][ 2 ];
		row_width += ( x0 - x ) * this->tex_width - 2 * this->glyph_spacing;

		if ( row_width > width ) width = row_width;
	}

	return { width, height };
}

void tex_font_t::draw_text( float x, float y, const wchar_t* txt, uint32_t col, int drawing_flags, int shadow_alpha ) {
	// weird shit bruh
	auto sz = this->get_text_extent( txt );
	const auto alpha{ ( ( col >> 24 ) & 0xFF ) / 255 };

	shadow_alpha *= alpha;

	if ( drawing_flags & ( /*FONT_LEFT_ALIGN |*/ FONT_CENTERED_X /* | FONT_CENTERED_Y*/ ) ) {
		/*if ( drawing_flags & FONT_LEFT_ALIGN ) x = std::round( x - sz.x );
		else*/ if ( drawing_flags & FONT_CENTERED_X ) x = std::round( x - sz.x * 0.5f );
		/*if ( drawing_flags & FONT_CENTERED_Y )*/ y = std::round( y - sz.y * 0.5f );
	}

	x -= this->glyph_spacing;
	auto start_x = x;

	auto text_col = col;

	while ( *txt ) {
		auto c = static_cast< int >( *txt++ );
		if ( c == '\n' ) { x = start_x, y += ( this->glyph_coords[ 0 ][ 3 ] - this->glyph_coords[ 0 ][ 1 ] ) * this->tex_height; continue; }
		if ( c - 32 < 0 || c - 32 >= this->highest_char - 32 ) continue;

		/*if ( c == '#' && drawing_flags & FONT_COLOR_FORMAT ) {
			std::wstring col_hex;
			col_hex += *txt;

			for ( int i = 0; i < 7; ++i )
				col_hex += *++txt;

			text_col = col_hex.GetD3D( );

			++txt;
			continue;
		}*/

		auto x0 = this->glyph_coords[ c - 32 ][ 0 ], y0 = this->glyph_coords[ c - 32 ][ 1 ], x1 = this->glyph_coords[ c - 32 ][ 2 ], y1 = this->glyph_coords[ c - 32 ][ 3 ];

		auto w = ( x1 - x0 ) * this->tex_width / this->tex_scale, h = ( y1 - y0 ) * this->tex_height / this->tex_scale;

		if ( c != ' ' ) {
			if ( drawing_flags & FONT_DROPSHADOW ) {
				auto shadowcol = D3DCOLOR_ARGB( shadow_alpha, 0, 0, 0 );

				Render::vertex_t v[ ] = {
					// down & right
					{ x + 0.5f, y + h + 0.5f, shadowcol, x0, y1 },
					{ x + 0.5f, y + 0.5f, shadowcol, x0, y0 },
					{ x + w + 0.5f, y + h + 0.5f, shadowcol, x1, y1 },
					{ x + w + 0.5f, y + 0.5f, shadowcol, x1, y0 },
					{ x + w + 0.5f, y + h + 0.5f, shadowcol, x1, y1 },
					{ x + 0.5f, y + 0.5f, shadowcol, x0, y0 },

					// normal
					{ x - 0.5f, y + h - 0.5f, text_col, x0, y1 },
					{ x - 0.5f, y - 0.5f, text_col, x0, y0 },
					{ x + w - 0.5f, y + h - 0.5f, text_col, x1, y1 },
					{ x + w - 0.5f, y - 0.5f, text_col, x1, y0 },
					{ x + w - 0.5f, y + h - 0.5f, text_col, x1, y1 },
					{ x - 0.5f, y - 0.5f, text_col, x0, y0 }
				};

				Render::add_vertices( v, D3DPT_TRIANGLELIST, true, this->tex );
			}
			else if ( drawing_flags & FONT_OUTLINE ) {
				auto shadowcol = D3DCOLOR_ARGB( shadow_alpha, 0, 0, 0 );

				Render::vertex_t v[ ] = {
					// up
					{ x - 0.5f, y + h - 1.5f, shadowcol, x0, y1 },
					{ x - 0.5f, y - 1.5f, shadowcol, x0, y0 },
					{ x + w - 0.5f, y + h - 1.5f, shadowcol, x1, y1 },
					{ x + w - 0.5f, y - 1.5f, shadowcol, x1, y0 },
					{ x + w - 0.5f, y + h - 1.5f, shadowcol, x1, y1 },
					{ x - 0.5f, y - 1.5f, shadowcol, x0, y0 },

					// down
					{ x - 0.5f, y + h + 0.5f, shadowcol, x0, y1 },
					{ x - 0.5f, y + 0.5f, shadowcol, x0, y0 },
					{ x + w - 0.5f, y + h + 0.5f, shadowcol, x1, y1 },
					{ x + w - 0.5f, y + 0.5f, shadowcol, x1, y0 },
					{ x + w - 0.5f, y + h + 0.5f, shadowcol, x1, y1 },
					{ x - 0.5f, y + 0.5f, shadowcol, x0, y0 },

					// left
					{ x - 1.5f, y + h - 0.5f, shadowcol, x0, y1 },
					{ x - 1.5f, y - 0.5f, shadowcol, x0, y0 },
					{ x + w - 1.5f, y + h - 0.5f, shadowcol, x1, y1 },
					{ x + w - 1.5f, y - 0.5f, shadowcol, x1, y0 },
					{ x + w - 1.5f, y + h - 0.5f, shadowcol, x1, y1 },
					{ x - 1.5f, y - 0.5f, shadowcol, x0, y0 },

					// right
					{ x + 0.5f, y + h - 0.5f, shadowcol, x0, y1 },
					{ x + 0.5f, y - 0.5f, shadowcol, x0, y0 },
					{ x + w + 0.5f, y + h - 0.5f, shadowcol, x1, y1 },
					{ x + w + 0.5f, y - 0.5f, shadowcol, x1, y0 },
					{ x + w + 0.5f, y + h - 0.5f, shadowcol, x1, y1 },
					{ x + 0.5f, y - 0.5f, shadowcol, x0, y0 },

					// up & left
					{ x - 1.5f, y + h - 1.5f, shadowcol, x0, y1 },
					{ x - 1.5f, y - 1.5f, shadowcol, x0, y0 },
					{ x + w - 1.5f, y + h - 1.5f, shadowcol, x1, y1 },
					{ x + w - 1.5f, y - 1.5f, shadowcol, x1, y0 },
					{ x + w - 1.5f, y + h - 1.5f, shadowcol, x1, y1 },
					{ x - 1.5f, y - 1.5f, shadowcol, x0, y0 },

					// up & right
					{ x + 0.5f, y + h - 1.5f, shadowcol, x0, y1 },
					{ x + 0.5f, y - 1.5f, shadowcol, x0, y0 },
					{ x + w + 0.5f, y + h - 1.5f, shadowcol, x1, y1 },
					{ x + w + 0.5f, y - 1.5f, shadowcol, x1, y0 },
					{ x + w + 0.5f, y + h - 1.5f, shadowcol, x1, y1 },
					{ x + 0.5f, y - 1.5f, shadowcol, x0, y0 },

					// down & left
					{ x - 1.5f, y + h + 0.5f, shadowcol, x0, y1 },
					{ x - 1.5f, y + 0.5f, shadowcol, x0, y0 },
					{ x + w - 1.5f, y + h + 0.5f, shadowcol, x1, y1 },
					{ x + w - 1.5f, y + 0.5f, shadowcol, x1, y0 },
					{ x + w - 1.5f, y + h + 0.5f, shadowcol, x1, y1 },
					{ x - 1.5f, y + 0.5f, shadowcol, x0, y0 },

					// down & right
					{ x + 0.5f, y + h + 0.5f, shadowcol, x0, y1 },
					{ x + 0.5f, y + 0.5f, shadowcol, x0, y0 },
					{ x + w + 0.5f, y + h + 0.5f, shadowcol, x1, y1 },
					{ x + w + 0.5f, y + 0.5f, shadowcol, x1, y0 },
					{ x + w + 0.5f, y + h + 0.5f, shadowcol, x1, y1 },
					{ x + 0.5f, y + 0.5f, shadowcol, x0, y0 },

					// normal
					{ x - 0.5f, y + h - 0.5f, text_col, x0, y1 },
					{ x - 0.5f, y - 0.5f, text_col, x0, y0 },
					{ x + w - 0.5f, y + h - 0.5f, text_col, x1, y1 },
					{ x + w - 0.5f, y - 0.5f, text_col, x1, y0 },
					{ x + w - 0.5f, y + h - 0.5f, text_col, x1, y1 },
					{ x - 0.5f, y - 0.5f, text_col, x0, y0 }
				};

				Render::add_vertices( v, D3DPT_TRIANGLELIST, true, this->tex );
			}
			else {
				Render::vertex_t v[ ] = {
					{ x - 0.5f, y + h - 0.5f, text_col, x0, y1 },
					{ x - 0.5f, y - 0.5f, text_col, x0, y0 },
					{ x + w - 0.5f, y + h - 0.5f, text_col, x1, y1 },
					{ x + w - 0.5f, y - 0.5f, text_col, x1, y0 },
					{ x + w - 0.5f, y + h - 0.5f, text_col, x1, y1 },
					{ x - 0.5f, y - 0.5f, text_col, x0, y0 }
				};

				Render::add_vertices( v, D3DPT_TRIANGLELIST, true, this->tex );
			}
		}

		x += w - 2 * this->glyph_spacing;
	}
}

Vector2D font_t::get_text_size( int height, const wchar_t* wstr ) {
	const auto& tfe = this->tex_fonts.find( height );

	static int old_height = height;
	static std::wstring old_wstr = wstr;
	static Vector2D result;

	if ( tfe == this->tex_fonts.end( ) ) {
		auto& tf = this->tex_fonts[ height ] = { this->name.c_str( ), static_cast< int >( height ), this->creation_flags, this->highest_char };
		tf.init( this->dev );
		result = tf.get_text_extent( wstr );
		return { static_cast< int >( result.x ), static_cast< int >( result.y ) };
	}

	if ( height != old_height || wstr != old_wstr ) {
		result = tfe->second.get_text_extent( wstr );
		old_height = height, old_wstr = wstr;
	}

	return { static_cast< int >( result.x ), static_cast< int >( result.y ) };
}

Vector2D font_t::get_text_size( int height, const std::string& str ) {
	static int old_height = height;
	static auto old_str = str;
	static auto result = this->get_text_size( height, std::wstring( str.begin( ), str.end( ) ).c_str( ) );

	if ( height != old_height || str != old_str ) {
		result = this->get_text_size( height, std::wstring( str.begin( ), str.end( ) ).c_str( ) );
		old_height = height, old_str = str;
	}

	return { static_cast< int >( result.x ), static_cast< int >( result.y ) };
}

void font_t::draw_text( int height, const Vector2D& pos, const wchar_t* txt, Color col, int drawing_flags, int shadow_alpha ) {
	const auto& tfe = this->tex_fonts.find( height );
	if ( Render::GlobalAlpha < 1.f )
		col = col.Set<COLOR_A>( col.Get<COLOR_A>( ) * Render::GlobalAlpha );

	if ( tfe == this->tex_fonts.end( ) ) {
		auto& tf = this->tex_fonts[ height ] = { this->name.c_str( ), static_cast< int >( height ), this->creation_flags, this->highest_char };
		tf.init( this->dev );
		tf.draw_text( static_cast< float >( pos.x ), static_cast< float >( pos.y ), txt, col.GetD3D( ), drawing_flags, shadow_alpha );
		return;
	}

	tfe->second.draw_text( static_cast< float >( pos.x ), static_cast< float >( pos.y ), txt, col.GetD3D( ), drawing_flags, shadow_alpha );
}

namespace Fonts {
	/*	Interfaces::Surface->SetFontGlyphSet( Fonts::WeaponIcon = Interfaces::Surface->FontCreate( ), _( "undefeated" ), 12, FW_MEDIUM, NULL, NULL, FONTFLAG_ANTIALIAS | FONTFLAG_DROPSHADOW );
	Interfaces::Surface->SetFontGlyphSet( Fonts::NameESP = Interfaces::Surface->FontCreate( ), _( "Verdana" ), 12, FW_MEDIUM, NULL, NULL, FONTFLAG_DROPSHADOW );
	Interfaces::Surface->SetFontGlyphSet( Fonts::HealthESP = Interfaces::Surface->FontCreate( ), _( "Small Fonts" ), 8, FW_MEDIUM, NULL, NULL, FONTFLAG_OUTLINE );
	Interfaces::Surface->SetFontGlyphSet( Fonts::Menu = Interfaces::Surface->FontCreate( ), _( "Tahoma" ), 13, FW_DONTCARE, NULL, NULL, FONTFLAG_DROPSHADOW );
	Interfaces::Surface->SetFontGlyphSet( Fonts::Logs = Interfaces::Surface->FontCreate( ), _( "Tahoma" ), 11, FW_DONTCARE, NULL, NULL, NULL );
	Interfaces::Surface->SetFontGlyphSet( Fonts::MenuTabs = Interfaces::Surface->FontCreate( ), _( "test2" ), 35, FW_NORMAL, NULL, NULL, FONTFLAG_DROPSHADOW | FONTFLAG_ANTIALIAS );
	Interfaces::Surface->SetFontGlyphSet( Fonts::DamageMarker = Interfaces::Surface->FontCreate( ), _( "Verdana" ), 12, FW_DONTCARE, NULL, NULL, FONTFLAG_DROPSHADOW | FONTFLAG_ANTIALIAS );*/

	std::shared_ptr<font_t> WeaponIcon = std::make_shared<font_t>( _( L"undefeated" ), D3DFONT_ANTIALIAS );
	std::shared_ptr<font_t> Verdana = std::make_shared<font_t>( _( L"Verdana" ), D3DFONT_ANTIALIAS );
	std::shared_ptr<font_t> Pixel = std::make_shared<font_t>( _( L"Small Fonts" ), D3DFONT_ANTIALIAS );
	std::shared_ptr<font_t> Tahoma = std::make_shared<font_t>( _( L"Tahoma" ) );
	std::shared_ptr<font_t> MenuTabs = std::make_shared<font_t>( _( L"test2" ), FONTFLAG_ANTIALIAS );
}
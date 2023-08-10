#pragma once
#include "../../utils/render.h"
#include "../input_manager.h"
#include "../config.h"
#include "../variables.h"
#include <memory>
#include <optional>

// detail: primary namespace which holds the framework.
namespace MenuFramework
{
	// detail: main member variables for the window.
	inline auto m_vecPosition{ Vector2D( 250, 500 ) };
	inline auto m_vecSize{ Vector2D( 650, 550 ) };

	inline auto m_bOpen{ true };

	inline auto m_flAnimation{ 0.f };
	inline auto m_iTab{ 0 };
	inline auto m_iActive{ 0 };

	// detail: commonly-used functions.
	FORCEINLINE float MenuAlpha( float modifier = 255.f ) { 
		return ( modifier * m_flAnimation );
	}

	FORCEINLINE void Animate( bool condition, float speed, float& variable ) {
		if ( condition ) variable += speed * Interfaces::Globals->flFrameTime;
		else variable -= speed * Interfaces::Globals->flFrameTime;
		variable = std::clamp( variable, 0.f, 1.f );
	}

	FORCEINLINE void AnimateLerp( bool condition, float speed, float& variable ) {
		if ( condition ) variable = std::clamp( Math::Interpolate( variable, 1.f, speed * Interfaces::Globals->flFrameTime ), 0.f, 1.f );
		else variable = std::clamp( Math::Interpolate( variable, 0.f, speed * Interfaces::Globals->flFrameTime ), 0.f, 1.f );
	}

	// detail: accent colours.
	namespace Accent {
		inline Color OutlineLight{ Color( 55, 55, 55 ) };
		inline Color Background{ Color( 20, 20, 20 ) };
		inline Color Group{ Color( 25, 25, 25 ) };
		inline Color Control{ Color( 25, 25, 25 ) };
		inline Color Accent2{ Color( 182, 139, 252 ) };
		inline Color Accent{ Color( 115, 155, 255 ) };
	}

	// detail: window class.
	struct Tab_t {
		std::string m_strTitle; float m_flAnim; Vector2D m_vecRenderArea; Vector2D m_vecRenderSize; std::vector<const char*> m_vecSubTabs; int m_iSubTab;
		Tab_t( std::string t, std::vector<const char*> tabs = { } ) : m_strTitle( t ), m_flAnim( 0.f ), m_vecRenderArea( { } ), m_vecRenderSize( { } ), m_vecSubTabs( tabs ) { m_iSubTab = m_vecSubTabs.empty( ) ? -1 : 0; }
	};

	class CWindow {
	private:
		std::vector<Tab_t> m_vecTabs;
		float m_flAnim{ 0.f };
	public:
		CWindow( std::vector<Tab_t> _tabs ) : m_vecTabs( _tabs ) {}

		FORCEINLINE auto& GetTabs( ) {
			return m_vecTabs;
		}

		bool Create( const char* title );
	};

	// detail: element template.
	enum EElementType : int {
		E_CHECKBOX = 0,
		E_SLIDER,
		E_DROPDOWN,
		E_MULTIDROPDOWN,
		E_KEYBIND,
		E_BUTTON,
		E_COLOR,
		E_LIST
	};

	class CElement {
	public:
		Vector2D m_vecContainerSize;
		std::vector<float> m_flAnim{ 0.f, 0.f, 0.f };
		uint32_t m_iVarID;
		EElementType m_iElement;
		std::string m_strTitle;

		virtual int Render( Vector2D& area ) = 0;
		virtual void Overlay( Vector2D& area ) = 0;
	};

	// detail: group class.
	class CGroupbox {
	public:
		bool m_bInit{ false };
		float m_flDimAnim{ 0.f };

		Vector2D m_vecPos{ },
			m_vecSz{ },
			m_vecDraw{ };

		std::vector<std::shared_ptr<CElement>> m_vecElements;

		bool Open( const std::string& title, const Vector2D& position, const Vector2D& size );

		template<typename T>
		void Add( T member ) { member->m_vecContainerSize = m_vecSz; m_vecElements.push_back( member ); }
		
		void Exit( const std::function<bool( )>& visible );
	};

	// detail: checkbox class.
	class CCheckbox : public CElement {
	public:
		CCheckbox( const std::string& title, const uint32_t id ) {
			m_strTitle = title; m_iVarID = id; m_iElement = EElementType::E_CHECKBOX;
		}

		int Render( Vector2D& area ) override;
		void Overlay( Vector2D& area ) override { /* do nothing. */ }
	};

	// detail: slider class.
	class CSlider : public CElement {
	public:
		float m_flMin, m_flMax, m_flVisual;
		bool m_bDragging;
		CSlider( const std::string& title, const float min, const float max, const uint32_t id ) {
			m_strTitle = title; m_flMin = m_flVisual = min; m_flMax = max; m_iVarID = id; m_iElement = EElementType::E_SLIDER;
		}

		int Render( Vector2D& area ) override;
		void Overlay( Vector2D& area ) override { /* do nothing. */ }
	};

	// detail: item class.
	struct Item_t {
		const char* m_strName;
		uint32_t m_iVarID;
		float m_flAnim{ 0.f };

		Item_t( const char* n, const uint32_t v = 0 ) {
			m_strName = n; m_iVarID = v;
		}
	};

	// detail: combobox class.
	class CCombo : public CElement {
	public:
		std::vector<Item_t> m_vecItems;
		CCombo( const std::string& title, std::vector<const char*> items, const uint32_t id ) {
			m_strTitle = title; m_iVarID = id; m_iElement = EElementType::E_DROPDOWN;
			for ( auto& i : items )
				m_vecItems.push_back( Item_t( i ) );
		}

		int Render( Vector2D& area ) override;
		void Overlay( Vector2D& area ) override;
	};

	// detail: multiselect class.
	class CMulti : public CElement {
	public:
		std::vector<Item_t> m_vecItems;
		CMulti( const std::string& title, std::vector<Item_t> items ) {
			m_strTitle = title; m_iElement = EElementType::E_MULTIDROPDOWN;
			for ( auto& i : items )
				m_vecItems.push_back( i );

			m_iVarID = FNV1A::Hash( title.c_str( ) );
		}

		int Render( Vector2D& area ) override;
		void Overlay( Vector2D& area ) override;
	};

	// detail: button class.
	class CButton : public CElement {
	public:
		std::function<void( )> m_fnCallback;
		CButton( const std::string& title, std::function<void( )> callback ) {
			m_strTitle = title; m_fnCallback = callback; m_iElement = EElementType::E_BUTTON;
		}

		int Render( Vector2D& area ) override;
		void Overlay( Vector2D& area ) override { /* do nothing. */ };
	};

	// detail: render.
	void Main( );
};

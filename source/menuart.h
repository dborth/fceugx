/****************************************************************************
 * FCE Ultra GX
 *
 * menuart.h
 *
 * Code-drawn bars and buttons
 *
 * A MenuButtonShape is used as a GuiButton's image. The button mirrors its
 * state into the shape, which fades between the resting, highlighted and
 * pressed looks
 *
 *   MenuButtonShape img(MenuButtonShape::LONG);
 *   GuiButton btn(MENU_BTN_LONG_W, MENU_BTN_LONG_H);
 *   btn.setImage(&img);
 ***************************************************************************/
#pragma once
#include "menustyle.h"

//!A button face: the long or large button
class MenuButtonShape : public GuiShape
{
	public:
		enum Kind { LONG, LARGE };

		MenuButtonShape(Kind kind) :
			GuiShape(kind == LONG ? MENU_BTN_LONG_W : MENU_BTN_LARGE_W,
					kind == LONG ? MENU_BTN_LONG_H : MENU_BTN_LARGE_H)
		{
			setCorners(GuiCorners(kind == LONG ? MENU_BTN_LONG_RADIUS : MENU_BTN_LARGE_RADIUS));
			setStyle(MenuButtonStyle());
		}
};

class MenuIconData : public GuiImageData
{
	public:
		MenuIconData(const uint8_t * png, float offsetX = 0.0f, float offsetY = 0.0f)
		{
			loadClipped(png, MENU_BTN_LARGE_W, MENU_BTN_LARGE_H, MENU_BTN_LARGE_RADIUS, MENU_BTN_ICON_INSET, offsetX, offsetY);
		}
};

//!The header (top) or footer (bottom) bar
class MenuBarShape : public GuiShape
{
	public:
		enum Edge { TOP, BOTTOM };

		MenuBarShape(Edge edge) :
			GuiShape(platform->getVideo()->getScreenWidth() + 2 * MENU_BAR_BLEED,
					(edge == TOP ? MENU_BAR_TOP_H : MENU_BAR_BOTTOM_H) + MENU_BAR_BLEED)
		{
			setPaint(edge == TOP ? MenuBarTopPaint() : MenuBarBottomPaint());
			if(edge == TOP)
			{
				setAlignment(ALIGN_H::LEFT, ALIGN_V::TOP);
				setPosition(-MENU_BAR_BLEED, -MENU_BAR_BLEED);
			}
			else
			{
				setAlignment(ALIGN_H::LEFT, ALIGN_V::BOTTOM);
				setPosition(-MENU_BAR_BLEED, MENU_BAR_BLEED);
			}
		}
};

//!The page behind everything: a vertical gradient over the whole screen
class MenuBackgroundShape : public GuiShape
{
	public:
		MenuBackgroundShape() :
			GuiShape(platform->getVideo()->getScreenWidth() + 2 * MENU_BAR_BLEED,
					platform->getVideo()->getScreenHeight() + 2 * MENU_BAR_BLEED)
		{
			setPaint(MenuBackgroundPaint());
			setAlignment(ALIGN_H::LEFT, ALIGN_V::TOP);
			setPosition(-MENU_BAR_BLEED, -MENU_BAR_BLEED);
		}
};

//!A card: the rounded, softly shadowed panel the game list and its preview sit on
class MenuCardShape : public GuiShape
{
	public:
		MenuCardShape(int w, int h) : GuiShape(w, h)
		{
			setCorners(GuiCorners(GuiTheme::current().radius));
			setStyle(GuiTheme::current().cardStyle());
		}
};

//!A soft shadow cast by the preview image onto its card. Draw it just behind the
//!image and call fit() with the image's displayed size.
class MenuCoverShadow : public GuiShape
{
	public:
		MenuCoverShadow()
		{
			setCorners(GuiCorners(3.0f));
			setPaint(GuiPaint().shadow(14.0f, MenuRGBA(0, 0, 0, 110), 0.0f, 4.0f));
			setVisible(false);
		}

		//!Sizes the shadow to the image, or hides it when there is no image (w or h is 0)
		void fit(int w, int h)
		{
			setVisible(w > 0 && h > 0);
			setSize(w, h);
		}
};

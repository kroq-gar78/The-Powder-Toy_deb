#ifndef LABEL_H
#define LABEL_H

#include <string>

#include "Component.h"
#include "Misc.h"
#include "Colour.h"

namespace ui
{
	class Label : public Component
	{
	protected:
		std::string textFragments;
		std::string textLines;

		std::string text;
		Colour textColour;
		int caret;
		int selectionIndex0;
		int selectionIndex1;

		int selectionXL;
		int selectionXH;
		int selectionYL;
		int selectionYH;
		int selectionLineL;
		int selectionLineH;

		bool multiline;
		bool selecting;
		bool autoHeight;

		void updateMultiline();
		void updateSelection();

		int getLowerSelectionBound();
		int getHigherSelectionBound();
	public:
		//Label(Window* parent_state, std::string labelText);
		Label(Point position, Point size, std::string labelText);
		//Label(std::string labelText);
		virtual ~Label();

		virtual void SetMultiline(bool status);

		virtual void SetText(std::string text);
		virtual std::string GetText();

		virtual bool HasSelection();
		virtual void ClearSelection();

		void SetTextColour(Colour textColour) { this->textColour = textColour; }

		virtual void OnMouseClick(int x, int y, unsigned button);
		virtual void OnMouseUp(int x, int y, unsigned button);
		virtual void OnMouseMoved(int localx, int localy, int dx, int dy);
		virtual void Draw(const Point& screenPos);
		virtual void Tick(float dt);
	};
}

#endif // LABEL_H
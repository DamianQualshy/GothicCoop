namespace GOTHIC_ENGINE {
    struct ChatLine
    {
        string text;
        zCOLOR color;
    };

    class Chat
    {
    private:
        bool isShowing = true;
        unsigned int chatLines = 50;
        std::vector<ChatLine> lines;
        SafeQueue<ChatLine> readyToBeDisplatedLines;
    public:
        void Render() {
            while (!readyToBeDisplatedLines.isEmpty()) {
                if (lines.size() < chatLines)
                    lines.push_back(readyToBeDisplatedLines.dequeue());
                else
                {
                    lines.erase(lines.begin());
                    lines.push_back(readyToBeDisplatedLines.dequeue());
                }
            }

            if (this->IsShowing() == true)
            {
                zSTRING font = screen->GetFontName();
                zCOLOR color = zCOLOR(255, 255, 255, 255);
                screen->SetFont(zSTRING("Font_Old_10_White_Hi.TGA"));
                int lineDistance = 150;

                for (unsigned int i = 0; i < lines.size(); i++)
                {
                    if (lines[i].text.Length() > 0)
                    {
                        screen->SetFontColor(lines[i].color);
                        screen->Print(50, lineDistance, zSTRING(lines[i].text));

                        screen->SetFontColor(color);
                    }
                    lineDistance = lineDistance + 150;
                }

                screen->SetFont(font);
            }
        };

        bool IsShowing() {
            return isShowing;
        }

        void ToggleShowing() {
            isShowing = !isShowing;
        }

        unsigned int GetLines() {
            return lines.size();
        }

        void AddLine(string text, zCOLOR color = zCOLOR(255, 255, 255, 255)) {
            ChatLine line;
            line.text = text;
            line.color = color;

            readyToBeDisplatedLines.enqueue(line);
        }

        void Clear() {
            lines.clear();
        }
    };
}

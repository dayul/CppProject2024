#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iostream>
#include <chrono>

using namespace std;
using namespace sf;

// VimSmall Class
class VimSmall {
public:
    // �⺻ ����
    VimSmall() : showSplashScreen(true), iMode(false), nMode(true), mode("NORMAL") {
        if (!font.loadFromFile("./Pretendard-Medium.otf")) {
            exit(1);
        }

        // �ؽ�Ʈ ��� ����
        text.setFont(font);
        text.setCharacterSize(24);
        text.setFillColor(Color::White);
        text.setPosition(10, 10);

        // Ŀ�� �ؽ�Ʈ ����
        cursor.setFont(font);
        cursor.setCharacterSize(24);
        cursor.setFillColor(Color::Cyan);

        // ���÷��� ȭ�� �ؽ�Ʈ ����
        splashText.setFont(font);
        splashText.setString(R"(
                ||   / /                                     //   ) )                                        
                ||  / /     ( )      _   __               ((            _   __        ___          //     //  
                || / /     / /     // ) )  ) )              \\        // ) )  ) )   //   ) )      //     //   
                ||/ /     / /     // / /  / /                 ) )    // / /  / /   //   / /      //     //    
                |  /     / /     // / /  / /          ((___ / /   // / /  / /   ((___( (   //     //     


                                            Press any key to start . . .
        )");
        splashText.setCharacterSize(25);
        splashText.setFillColor(Color::Cyan);
        splashText.setPosition(100, 290);

        // ���� ǥ���� �ؽ�Ʈ ����
        statusBar.setFont(font);
        statusBar.setCharacterSize(24);
        statusBar.setFillColor(Color::White);
        statusBar.setPosition(10, 770);

        // ������ ����
        window.create(VideoMode(1000, 800), "VimSmall");

        // Ŀ�� �ʱ� ��ġ ����
        cursorPosition = { 0, 0 };

        // �� �ٿ� ǥ�� ������ �ִ� ���� �� ���
        maxCharsPerLine = (window.getSize().x - 20) / 15;

        // ȭ�鿡 ǥ���� �� �� ���
        visibleLines = (window.getSize().y - 40) / (text.getCharacterSize() + 5);

        // �ʱ� �������� �� �� �߰�
        lines.push_back("");

        // ���� ǥ���� ������Ʈ
        updateStatusBar();
    }

    // ���α׷� ���� �Լ�
    void run() {
        while (window.isOpen()) {
            processEvents();  // �̺�Ʈ ó�� �Լ� ȣ��
            window.clear();   // ȭ�� �ʱ�ȭ

            if (showSplashScreen) {
                // ó�� ���÷��� ȭ�� ���
                window.draw(splashText);
            }
            else {
                updateText();  // �ؽ�Ʈ, Ŀ�� ������Ʈ
                window.draw(text);
                window.draw(cursor);
                window.draw(statusBar);
            }

            // �߸��� ��ɾ� �Է� 2�� �� normal ���� ��ȯ
            if (errorMode && cmdClock.getElapsedTime().asSeconds() > 1.5) {
                errorMode = false;
                cmdMode = false;
                changeMode("NORMAL");
            }

            window.display(); 
        }
    }

private:
    // ��� ���� ����
    RenderWindow window;
    Font font;
    Text text, cursor, splashText, statusBar;  // �ؽ�Ʈ 
    vector<string> lines;        // �Էµ� �ؽ�Ʈ ���� ����
    string mode;                 // ���� ��� ("INSERT" �Ǵ� "NORMAL")
    Vector2i cursorPosition;     // Ŀ�� ��ġ( x, y )
    string command;              // ��� ��忡�� �Էµ� ��ɾ� ����
    Clock cmdClock;              // �߸��� ��ɾ� �Է½� sleep
    int scrollOffset = 0;        // ���� ȭ���� ���� ��
    int visibleLines;            // ȭ�鿡 ǥ���� �� �ִ� �� ��
    bool iMode;                  // ���� ��� ����
    bool nMode;                  // normal ��� ����
    bool cmdMode;                // ��� ��� ����
    bool errorMode;              // ��� ��忡�� �߸��� ���
    int maxCharsPerLine;         // �� �ٿ� ǥ�� ������ �ִ� ���� ��
    bool showSplashScreen;       // ���÷��� ȭ�� ǥ�� ����
    bool iflag;                  // insert ��� �� ó�� �ԷµǴ� i �����ϱ�

    // ��ü �̺�Ʈ ó�� �Լ�
    void processEvents() {
        Event event;
        while (window.pollEvent(event)) {
            if (event.type == Event::Closed) {
                // ������ �ݱ� �̺�Ʈ ó��
                window.close();
            }
            if (event.type == Event::TextEntered) {
                if (!showSplashScreen) {
                    // �ؽ�Ʈ �Է� ó��
                    handleTextInput(event);
                }
                else {
                    // ���÷��� ȭ�� �� ������ �ϱ�
                    showSplashScreen = false;
                }
            }
            if (event.type == Event::KeyPressed) {
                // ������ Ű �Է� ó��
                handleKeyInput(event);
            }
        }
    }

    // ��� ���� �Լ�
    void changeMode(string newMode) {
        // insert -> normal
        if (newMode == "INSERT") {
            iMode = true;
            nMode = false;
            iflag = true;       // ù �Է� i ��Ȱ��ȭ
        }

        // normal -> insert
        else if (newMode == "NORMAL") {
            nMode = true;
            iMode = false;
            iflag = false;
        }
        mode = newMode;
        updateStatusBar();  // ���� ǥ���� ������Ʈ
    }

    // �ؽ�Ʈ �Է� ó�� �Լ�
    void handleTextInput(const Event& event) {
        // ��� ����� ���� ��ɾ ó��
        if (cmdMode) {
            if (event.text.unicode == '\b' && !command.empty()) {
                command.pop_back();
                updateStatusBar();
            }
            else if (event.text.unicode == '\r') {
                handleCommand();   // enter�� ��� �����ϱ�
                updateStatusBar(); 
            }
            else if (event.text.unicode < 128) {
                // �Ϲ� ���� �߰�
                command += static_cast<char>(event.text.unicode);
                updateStatusBar(); 
            }
            return;     // �̿��� ó���� ����
        }
        if (iMode) {
            // insert ���� ���� ���� ��, iflag�� true�� ��� ó�� �Է�(i) �����ϰ� �Լ�����
            if (iflag) {
                iflag = false;  // ó�� �Է� ���� �Է��� ó���ϵ��� iflag�� false�� ����
                return;
            }
            if (event.text.unicode == '\b') {
                // �齺���̽� ó��
                if (cursorPosition.x > 0) {
                    // ������ �ִٸ� ���� Ŀ����ġ���� �������� ���� �����
                    lines[cursorPosition.y].erase(cursorPosition.x - 1, 1);
                    cursorPosition.x--;     // Ŀ�� x�� ����
                }
                else if (cursorPosition.y > 0) {
                    // ���� �ٰ� ����
                    cursorPosition.x = lines[cursorPosition.y - 1].size();
                    lines[cursorPosition.y - 1] += lines[cursorPosition.y];
                    lines.erase(lines.begin() + cursorPosition.y);
                    cursorPosition.y--;
                }
            }
            else if (event.text.unicode == '\n' || event.text.unicode == '\r') {
                // ���� �ٿ��� Ŀ�� ��ġ ������ �ؽ�Ʈ�� ���� �ٷ� �̵�
                string remainingText = lines[cursorPosition.y].substr(cursorPosition.x);
                lines[cursorPosition.y].erase(cursorPosition.x);  // ���� ���� Ŀ�� ���� �ؽ�Ʈ ����
                lines.insert(lines.begin() + cursorPosition.y + 1, remainingText); // �� �� ���� �� �ؽ�Ʈ �߰�

                // Ŀ�� ��ġ�� ���� �ٷ� �̵�
                cursorPosition.y++;
                cursorPosition.x = 0;

                // ȭ�鿡 ǥ���� �� �� Ȯ�� �� ��ũ�� ����
                if (cursorPosition.y - scrollOffset >= visibleLines) {
                    scrollOffset++;
                }
            }
            else if (event.text.unicode < 128) {
                // �� �� �Ϲ� ���� �Է� ó��
                // �´� ��(y)�� ���ڸ� ����
                lines[cursorPosition.y].insert(cursorPosition.x, 1, static_cast<char>(event.text.unicode));
                cursorPosition.x++;

                if (cursorPosition.x >= maxCharsPerLine) {
                    // �� ���� �ִ� ���ڸ� �ʰ��� ��� ó��
                    string overflowText = lines[cursorPosition.y].substr(maxCharsPerLine); // �ʰ��� �ؽ�Ʈ
                    lines[cursorPosition.y].erase(maxCharsPerLine);                        // ���� �� �߶󳻱�
                    lines.insert(lines.begin() + cursorPosition.y + 1, overflowText);      // ���� �ٷ� �̵�
                    cursorPosition.y++;
                    cursorPosition.x = 0;
                }
            }
        }
    }

    // Ű �Է� ó��
    void handleKeyInput(const Event& event) {
        // ��� ����
        if (cmdMode) {      // ��� ����϶� Ű �Է� ����
            return;
        }
        if (nMode) {
            if (event.key.code == Keyboard::I) {
                // normal -> insert ���� ��ȯ
                changeMode("INSERT");
            }
            // cmd ���� ���� : SFML������ Colon�� SemiColon�� �����ϰ� ó����
            else if (event.key.code == Keyboard::SemiColon) {
                cmdMode = true;
                command.clear();
                updateStatusBar();
            }
        }
        else if (iMode) {
            if (event.key.code == Keyboard::Escape) {
                // normal -> insert ���� ��ȯ
                changeMode("NORMAL");
            }
        }

        // H, J, K, L �� Ŀ�� �̵�
        if (event.key.code == Keyboard::H && cursorPosition.x > 0) {
            cursorPosition.x--;
        }
        else if (event.key.code == Keyboard::J && cursorPosition.y < lines.size() - 1) {
            cursorPosition.y++;
            cursorPosition.x = min(cursorPosition.x, (int)lines[cursorPosition.y].size());
        }
        else if (event.key.code == Keyboard::K && cursorPosition.y > 0) {
            cursorPosition.y--;
            cursorPosition.x = min(cursorPosition.x, (int)lines[cursorPosition.y].size());
        }
        else if (event.key.code == Keyboard::L && cursorPosition.x < lines[cursorPosition.y].size() && cursorPosition.x < maxCharsPerLine) {
            cursorPosition.x++;
        }

        // ��, �Ʒ� ��ũ��
        if (event.key.code == Keyboard::Up && scrollOffset > 0) {
            scrollOffset--;
        }
        else if (event.key.code == Keyboard::Down && scrollOffset + visibleLines < lines.size()) {
            scrollOffset++;
        }

    }

    // �ؽ�Ʈ�� Ŀ�� ������Ʈ �Լ�
    void updateText() {
        ostringstream oss;

        // ȭ�鿡 ǥ���� �ؽ�Ʈ ����
        for (int i = scrollOffset; i < lines.size() && i < scrollOffset + visibleLines; i++) {
            oss << lines[i] << "\n";
        }

        text.setString(oss.str());
        cursor.setString("|");

        // Ŀ���� Y ��ǥ�� Ŀ���� ��ġ�� �ٿ��� `scrollOffset`�� �� ��
        float cursorY = 10 + (cursorPosition.y - scrollOffset) * (text.getCharacterSize() + 5);

        // Ŀ���� X ��ǥ�� ���� ���� Ŀ�� �ձ����� ���� �� ���
        float cursorX = 10; // ���� ��ġ (���� ����)
        if (cursorPosition.x > 0) {
            string currentLine = lines[cursorPosition.y].substr(0, cursorPosition.x);
            for (char ch : currentLine) {
                cursorX += text.getFont()->getGlyph(ch, text.getCharacterSize(), false).advance;
            }
        }

        // Ŀ�� ��ǥ ����
        cursor.setPosition(cursorX, cursorY);
    }

    // ���� ǥ���� ������Ʈ �Լ�
    void updateStatusBar() {
        if (cmdMode) {
            if (errorMode) {
                statusBar.setFillColor(Color::Red);
                statusBar.setString("Unknown command.. Please enter again");
            }
            else {
                statusBar.setFillColor(Color::White);
                statusBar.setString(command);
            }
        }
        else {
            if(errorMode) {
                statusBar.setFillColor(Color::Red);
                statusBar.setString("Unknown command.. Please enter again");
            }
            else {
                statusBar.setFillColor(Color::White);
                statusBar.setString("-- " + mode + " --");
            } 
        }
    }

    // ���� ���� �Լ�
    void saveToFile(const string& filename) {
        ofstream file(filename + ".txt");
        if (file.is_open()) {
            for (const auto& line : lines) {
                file << line << "\n";
            }
            file.close();
        }
        else {
            cout << filename << "���� ���� ����" << endl;
        }
    }

    // ��ɾ� ó�� �Լ�
    void handleCommand() {

        if (command == ":wq") {
            cout << "wq �϶� : " << endl;
            saveToFile("output");
            window.close();
        }
        else if (command.find(":w ") == 0) {          // :w ��ɾ 0 ��ġ���� Ȯ��
            // �̸��� �־����� �ʾ��� ��� �⺻������ output.txt�� �̸� �ֱ�
            string filename = command.length() > 2 ? trim(command.substr(2)) : "output";
            saveToFile(filename);
            window.close();
        }
        else if (command == ":q!") {            // ���� �� �ϰ� �ٷ� ����
            window.close();
        }
        else {
            errorMode = true;
            cmdClock.restart();     // �ð� �缳��
        }

        command.clear();
        cmdMode = false;
    }

    // ��ɾ� ��ó�� �Լ�
    string trim(const string& str) {
        // size_t : ��ȣ���� ���� Ÿ��
        size_t start = str.find_first_not_of(" \t\n\r"); // ���� ����, ��, �ٹٲ��� �ƴ� ��ġ return
        size_t end = str.find_last_not_of(" \t\n\r");   // ���� ����

        if (start == std::string::npos) {
            return "";       // ���ڿ��� ��� ������ ��� �� ���ڿ� return
        }

        return str.substr(start, end - start + 1);      // �յ� ���� ���� ���ڿ� return
    }
};

// ���� �Լ�
int main() {
    VimSmall editor;
    editor.run();
    return 0;
}
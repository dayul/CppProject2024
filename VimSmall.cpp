#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iostream>

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
        splashText.setFillColor(Color::White);
        splashText.setPosition(100, 290);

        // ���� ǥ���� �ؽ�Ʈ ����
        statusBar.setFont(font);
        statusBar.setCharacterSize(24);
        statusBar.setFillColor(Color::White);
        statusBar.setPosition(10, 770);  // ȭ�� ���� �ϴ�

        // ������ ����
        window.create(VideoMode(1000, 800), "VimSmall");

        // Ŀ�� �ʱ� ��ġ ����
        cursorPosition = { 0, 0 };

        // �� �ٿ� ǥ�� ������ �ִ� ���� �� ���
        maxCharsPerLine = (window.getSize().x - 20) / 15;

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
                window.draw(statusBar);  // ���� ǥ���� ǥ��
            }

            window.display();  // ȭ�� �׸���
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
    bool iMode;                  // ���� ��� ����
    bool nMode;                  // normal ��� ����
    bool cmdMode;                // ��� ��� ����
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
                // Ű �Է� ó��
                handleKeyInput(event);
            }
            if (cmdMode && event.type == Event::TextEntered) {
                // ��� ��忡�� �Է� ó��
                if (event.text.unicode == '\b' && !command.empty()) {
                    command.pop_back();  // �齺���̽��� ��� ����
                }
                else if (event.text.unicode == '\r') {
                    handleCommand();     // ���ͷ� ��� ����
                    cmdMode = false;     // ��� ��� ����
                }
                else if (event.text.unicode < 128) {
                    command += static_cast<char>(event.text.unicode);  // ��� �Է� �߰�
                }
            }

        }
    }

    // ��� ���� �Լ�
    void changeInsertMode(string newMode) {
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
        if (iMode) {
            // insert ���� ���� ���� ��, iflag�� true�� ��� ó�� �Է� �����ϰ� �Լ�����
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
                // �ٹٲ� ó��
                //cout << "Key pressed: " << event.text.unicode << endl;
                string remainingText = lines[cursorPosition.y].substr(cursorPosition.x);        // Ŀ�� ������ �ؽ�Ʈ ����    
                lines[cursorPosition.y].erase(cursorPosition.x);                                // Ŀ�� ������ �ؽ�Ʈ ����
                lines.insert(lines.begin() + cursorPosition.y + 1, remainingText);              // �� �ٿ� ����� �ؽ�Ʈ ��Ÿ����
                cursorPosition.y++;     // Ŀ���� �����ٷ� �̵�
                cursorPosition.x = 0;   // ������ �Ǿ����� Ŀ�� �̵�
            }
            else if (event.text.unicode < 128) {
                // �� �� �Ϲ� ���� �Է� ó��
                if (cursorPosition.x < maxCharsPerLine) {
                    // �´� �ٿ� ���ڸ� ����
                    lines[cursorPosition.y].insert(cursorPosition.x, 1, static_cast<char>(event.text.unicode));
                    cursorPosition.x++;
                }
            }
        }
        else if (cmdMode) {
            if (event.text.unicode == '\b' && !command.empty()) {
                command.pop_back();
            }
            else if (event.text.unicode == '\r') {
                handleCommand();   // enter�� ��� �����ϱ�
                cmdMode = false; // ��� ��� ����
                updateStatusBar(); // ���� ǥ���� ������Ʈ
            }
            else if (event.text.unicode < 128) {
                // �Ϲ� ���� �߰�
                command += static_cast<char>(event.text.unicode);
                updateStatusBar(); // ���� ǥ���� ������Ʈ
            }
        }
    }

    // Ű �Է� ó��
    void handleKeyInput(const Event& event) {
        // ��� ����
        if (nMode) {
            if (event.key.code == Keyboard::I) {
                // normal -> insert ���� ��ȯ
                changeInsertMode("INSERT");
            }
            else if (event.text.unicode == ':') {
                cmdMode = true;
                command.clear();
                updateStatusBar();
            }
            else if (event.key.code == Keyboard::W) {
                // ���� ����
                saveToFile("output.txt");
            }
        }
        else if (iMode) {
            if (event.key.code == Keyboard::Escape) {
                // normal -> insert ���� ��ȯ
                changeInsertMode("NORMAL");
            }
        }

        // Ŀ�� �̵�
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
    }

    // �ؽ�Ʈ�� Ŀ�� ������Ʈ �Լ�
    void updateText() {
        ostringstream oss;
        for (const auto& line : lines) {
            oss << line << "\n";
        }
        text.setString(oss.str());

        cursor.setString("|");
        cursor.setPosition(10 + cursorPosition.x * 12, 10 + cursorPosition.y * 30);
    }

    // ���� ǥ���� ������Ʈ �Լ�
    void updateStatusBar() {
        if (cmdMode) {
            statusBar.setString(":" + command);
        }
        else statusBar.setString("-- " + mode + " --");
    }

    // ���� ���� �Լ�
    void saveToFile(const string& filename) {
        ofstream file(filename);
        if (file.is_open()) {
            for (const auto& line : lines) {
                file << line << "\n";
            }
            file.close();
        }
    }

    // ��ɾ� ó�� �Լ�
    // TODO : ���� ���� ���� �ذ��ϱ�
    void handleCommand() {
        if (command == "wq") {
            saveToFile("output.txt");
        }
        command.clear();
    }
};

// ���� �Լ�
int main() {
    VimSmall editor;
    editor.run();
    return 0;
}
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
    // 기본 설정
    VimSmall() : showSplashScreen(true), iMode(false), nMode(true), mode("NORMAL") {
        if (!font.loadFromFile("./Pretendard-Medium.otf")) {
            exit(1);
        }

        // 텍스트 요소 설정
        text.setFont(font);
        text.setCharacterSize(24);
        text.setFillColor(Color::White);
        text.setPosition(10, 10);

        // 커서 텍스트 설정
        cursor.setFont(font);
        cursor.setCharacterSize(24);
        cursor.setFillColor(Color::Cyan);

        // 스플래시 화면 텍스트 설정
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

        // 상태 표시줄 텍스트 설정
        statusBar.setFont(font);
        statusBar.setCharacterSize(24);
        statusBar.setFillColor(Color::White);
        statusBar.setPosition(10, 770);  // 화면 왼쪽 하단

        // 윈도우 생성
        window.create(VideoMode(1000, 800), "VimSmall");

        // 커서 초기 위치 설정
        cursorPosition = { 0, 0 };

        // 한 줄에 표시 가능한 최대 문자 수 계산
        maxCharsPerLine = (window.getSize().x - 20) / 15;

        // 초기 설정으로 빈 줄 추가
        lines.push_back("");

        // 상태 표시줄 업데이트
        updateStatusBar();
    }

    // 프로그램 실행 함수
    void run() {
        while (window.isOpen()) {
            processEvents();  // 이벤트 처리 함수 호출
            window.clear();   // 화면 초기화

            if (showSplashScreen) {
                // 처음 스플래시 화면 출력
                window.draw(splashText);
            }
            else {
                updateText();  // 텍스트, 커서 업데이트
                window.draw(text);
                window.draw(cursor);
                window.draw(statusBar);  // 상태 표시줄 표시
            }

            window.display();  // 화면 그리기
        }
    }

private:
    // 멤버 변수 선언
    RenderWindow window;
    Font font;
    Text text, cursor, splashText, statusBar;  // 텍스트 
    vector<string> lines;        // 입력된 텍스트 라인 저장
    string mode;                 // 현재 모드 ("INSERT" 또는 "NORMAL")
    Vector2i cursorPosition;     // 커서 위치( x, y )
    string command;              // 명령 모드에서 입력된 명령어 저장
    bool iMode;                  // 삽입 모드 상태
    bool nMode;                  // normal 모드 상태
    bool cmdMode;                // 명령 모드 상태
    int maxCharsPerLine;         // 한 줄에 표시 가능한 최대 문자 수
    bool showSplashScreen;       // 스플래시 화면 표시 여부
    bool iflag;                  // insert 모드 시 처음 입력되는 i 무시하기

    // 전체 이벤트 처리 함수
    void processEvents() {
        Event event;
        while (window.pollEvent(event)) {
            if (event.type == Event::Closed) {
                // 윈도우 닫기 이벤트 처리
                window.close();
            }
            if (event.type == Event::TextEntered) {
                if (!showSplashScreen) {
                    // 텍스트 입력 처리
                    handleTextInput(event);
                }
                else {
                    // 스플래시 화면 안 나오게 하기
                    showSplashScreen = false;
                }
            }
            if (event.type == Event::KeyPressed) {
                // 키 입력 처리
                handleKeyInput(event);
            }
            if (cmdMode && event.type == Event::TextEntered) {
                // 명령 모드에서 입력 처리
                if (event.text.unicode == '\b' && !command.empty()) {
                    command.pop_back();  // 백스페이스로 명령 삭제
                }
                else if (event.text.unicode == '\r') {
                    handleCommand();     // 엔터로 명령 실행
                    cmdMode = false;     // 명령 모드 종료
                }
                else if (event.text.unicode < 128) {
                    command += static_cast<char>(event.text.unicode);  // 명령 입력 추가
                }
            }

        }
    }

    // 모드 변경 함수
    void changeInsertMode(string newMode) {
        // insert -> normal
        if (newMode == "INSERT") {
            iMode = true;
            nMode = false;
            iflag = true;       // 첫 입력 i 비활성화
        }

        // normal -> insert
        else if (newMode == "NORMAL") {
            nMode = true;
            iMode = false;
            iflag = false;
        }
        mode = newMode;
        updateStatusBar();  // 상태 표시줄 업데이트
    }

    // 텍스트 입력 처리 함수
    void handleTextInput(const Event& event) {
        if (iMode) {
            // insert 모드로 변경 됐을 때, iflag가 true인 경우 처음 입력 무시하고 함수종료
            if (iflag) {
                iflag = false;  // 처음 입력 이후 입력은 처리하도록 iflag를 false로 설정
                return;
            }
            if (event.text.unicode == '\b') {
                // 백스페이스 처리
                if (cursorPosition.x > 0) {
                    // 내용이 있다면 현재 커서위치에서 왼쪽으로 문자 지우기
                    lines[cursorPosition.y].erase(cursorPosition.x - 1, 1);
                    cursorPosition.x--;     // 커서 x축 변경
                }
                else if (cursorPosition.y > 0) {
                    // 이전 줄과 병합
                    cursorPosition.x = lines[cursorPosition.y - 1].size();
                    lines[cursorPosition.y - 1] += lines[cursorPosition.y];
                    lines.erase(lines.begin() + cursorPosition.y);
                    cursorPosition.y--;
                }
            }
            else if (event.text.unicode == '\n' || event.text.unicode == '\r') {
                // 줄바꿈 처리
                //cout << "Key pressed: " << event.text.unicode << endl;
                string remainingText = lines[cursorPosition.y].substr(cursorPosition.x);        // 커서 이후의 텍스트 저장    
                lines[cursorPosition.y].erase(cursorPosition.x);                                // 커서 이후의 텍스트 제거
                lines.insert(lines.begin() + cursorPosition.y + 1, remainingText);              // 새 줄에 저장된 텍스트 나타내기
                cursorPosition.y++;     // 커서를 다음줄로 이동
                cursorPosition.x = 0;   // 새줄의 맨앞으로 커서 이동
            }
            else if (event.text.unicode < 128) {
                // 그 외 일반 문자 입력 처리
                if (cursorPosition.x < maxCharsPerLine) {
                    // 맞는 줄에 문자를 삽입
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
                handleCommand();   // enter로 명령 실행하기
                cmdMode = false; // 명령 모드 종료
                updateStatusBar(); // 상태 표시줄 업데이트
            }
            else if (event.text.unicode < 128) {
                // 일반 문자 추가
                command += static_cast<char>(event.text.unicode);
                updateStatusBar(); // 상태 표시줄 업데이트
            }
        }
    }

    // 키 입력 처리
    void handleKeyInput(const Event& event) {
        // 모드 변경
        if (nMode) {
            if (event.key.code == Keyboard::I) {
                // normal -> insert 모드로 전환
                changeInsertMode("INSERT");
            }
            else if (event.text.unicode == ':') {
                cmdMode = true;
                command.clear();
                updateStatusBar();
            }
            else if (event.key.code == Keyboard::W) {
                // 파일 저장
                saveToFile("output.txt");
            }
        }
        else if (iMode) {
            if (event.key.code == Keyboard::Escape) {
                // normal -> insert 모드로 전환
                changeInsertMode("NORMAL");
            }
        }

        // 커서 이동
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

    // 텍스트와 커서 업데이트 함수
    void updateText() {
        ostringstream oss;
        for (const auto& line : lines) {
            oss << line << "\n";
        }
        text.setString(oss.str());

        cursor.setString("|");
        cursor.setPosition(10 + cursorPosition.x * 12, 10 + cursorPosition.y * 30);
    }

    // 상태 표시줄 업데이트 함수
    void updateStatusBar() {
        if (cmdMode) {
            statusBar.setString(":" + command);
        }
        else statusBar.setString("-- " + mode + " --");
    }

    // 파일 저장 함수
    void saveToFile(const string& filename) {
        ofstream file(filename);
        if (file.is_open()) {
            for (const auto& line : lines) {
                file << line << "\n";
            }
            file.close();
        }
    }

    // 명령어 처리 함수
    // TODO : 파일 저장 에러 해결하기
    void handleCommand() {
        if (command == "wq") {
            saveToFile("output.txt");
        }
        command.clear();
    }
};

// 메인 함수
int main() {
    VimSmall editor;
    editor.run();
    return 0;
}
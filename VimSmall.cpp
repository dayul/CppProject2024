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
        splashText.setFillColor(Color::Cyan);
        splashText.setPosition(100, 290);

        // 상태 표시줄 텍스트 설정
        statusBar.setFont(font);
        statusBar.setCharacterSize(24);
        statusBar.setFillColor(Color::White);
        statusBar.setPosition(10, 770);

        // 윈도우 생성
        window.create(VideoMode(1000, 800), "VimSmall");

        // 커서 초기 위치 설정
        cursorPosition = { 0, 0 };

        // 한 줄에 표시 가능한 최대 문자 수 계산
        maxCharsPerLine = (window.getSize().x - 20) / 15;

        // 화면에 표시할 줄 수 계산
        visibleLines = (window.getSize().y - 40) / (text.getCharacterSize() + 5);

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
                window.draw(statusBar);
            }

            // 잘못된 명령어 입력 2초 후 normal 모드로 전환
            if (errorMode && cmdClock.getElapsedTime().asSeconds() > 1.5) {
                errorMode = false;
                cmdMode = false;
                changeMode("NORMAL");
            }

            window.display(); 
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
    Clock cmdClock;              // 잘못된 명령어 입력시 sleep
    int scrollOffset = 0;        // 현재 화면의 시작 줄
    int visibleLines;            // 화면에 표시할 수 있는 줄 수
    bool iMode;                  // 삽입 모드 상태
    bool nMode;                  // normal 모드 상태
    bool cmdMode;                // 명령 모드 상태
    bool errorMode;              // 명령 모드에서 잘못된 명령
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
                // 나머지 키 입력 처리
                handleKeyInput(event);
            }
        }
    }

    // 모드 변경 함수
    void changeMode(string newMode) {
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
        // 명령 모드일 때는 명령어만 처리
        if (cmdMode) {
            if (event.text.unicode == '\b' && !command.empty()) {
                command.pop_back();
                updateStatusBar();
            }
            else if (event.text.unicode == '\r') {
                handleCommand();   // enter로 명령 실행하기
                updateStatusBar(); 
            }
            else if (event.text.unicode < 128) {
                // 일반 문자 추가
                command += static_cast<char>(event.text.unicode);
                updateStatusBar(); 
            }
            return;     // 이외의 처리를 막음
        }
        if (iMode) {
            // insert 모드로 변경 됐을 때, iflag가 true인 경우 처음 입력(i) 무시하고 함수종료
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
                // 현재 줄에서 커서 위치 이후의 텍스트를 다음 줄로 이동
                string remainingText = lines[cursorPosition.y].substr(cursorPosition.x);
                lines[cursorPosition.y].erase(cursorPosition.x);  // 현재 줄의 커서 이후 텍스트 제거
                lines.insert(lines.begin() + cursorPosition.y + 1, remainingText); // 새 줄 생성 후 텍스트 추가

                // 커서 위치를 다음 줄로 이동
                cursorPosition.y++;
                cursorPosition.x = 0;

                // 화면에 표시할 줄 수 확인 후 스크롤 조정
                if (cursorPosition.y - scrollOffset >= visibleLines) {
                    scrollOffset++;
                }
            }
            else if (event.text.unicode < 128) {
                // 그 외 일반 문자 입력 처리
                // 맞는 줄(y)에 문자를 삽입
                lines[cursorPosition.y].insert(cursorPosition.x, 1, static_cast<char>(event.text.unicode));
                cursorPosition.x++;

                if (cursorPosition.x >= maxCharsPerLine) {
                    // 한 줄의 최대 문자를 초과한 경우 처리
                    string overflowText = lines[cursorPosition.y].substr(maxCharsPerLine); // 초과된 텍스트
                    lines[cursorPosition.y].erase(maxCharsPerLine);                        // 현재 줄 잘라내기
                    lines.insert(lines.begin() + cursorPosition.y + 1, overflowText);      // 다음 줄로 이동
                    cursorPosition.y++;
                    cursorPosition.x = 0;
                }
            }
        }
    }

    // 키 입력 처리
    void handleKeyInput(const Event& event) {
        // 모드 변경
        if (cmdMode) {      // 명령 모드일때 키 입력 무시
            return;
        }
        if (nMode) {
            if (event.key.code == Keyboard::I) {
                // normal -> insert 모드로 전환
                changeMode("INSERT");
            }
            // cmd 모드로 변경 : SFML에서는 Colon과 SemiColon을 동일하게 처리함
            else if (event.key.code == Keyboard::SemiColon) {
                cmdMode = true;
                command.clear();
                updateStatusBar();
            }
        }
        else if (iMode) {
            if (event.key.code == Keyboard::Escape) {
                // normal -> insert 모드로 전환
                changeMode("NORMAL");
            }
        }

        // H, J, K, L 별 커서 이동
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

        // 위, 아래 스크롤
        if (event.key.code == Keyboard::Up && scrollOffset > 0) {
            scrollOffset--;
        }
        else if (event.key.code == Keyboard::Down && scrollOffset + visibleLines < lines.size()) {
            scrollOffset++;
        }

    }

    // 텍스트와 커서 업데이트 함수
    void updateText() {
        ostringstream oss;

        // 화면에 표시할 텍스트 구성
        for (int i = scrollOffset; i < lines.size() && i < scrollOffset + visibleLines; i++) {
            oss << lines[i] << "\n";
        }

        text.setString(oss.str());
        cursor.setString("|");

        // 커서의 Y 좌표는 커서가 위치한 줄에서 `scrollOffset`을 뺀 값
        float cursorY = 10 + (cursorPosition.y - scrollOffset) * (text.getCharacterSize() + 5);

        // 커서의 X 좌표는 현재 줄의 커서 앞까지의 문자 폭 계산
        float cursorX = 10; // 시작 위치 (여백 포함)
        if (cursorPosition.x > 0) {
            string currentLine = lines[cursorPosition.y].substr(0, cursorPosition.x);
            for (char ch : currentLine) {
                cursorX += text.getFont()->getGlyph(ch, text.getCharacterSize(), false).advance;
            }
        }

        // 커서 좌표 적용
        cursor.setPosition(cursorX, cursorY);
    }

    // 상태 표시줄 업데이트 함수
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

    // 파일 저장 함수
    void saveToFile(const string& filename) {
        ofstream file(filename + ".txt");
        if (file.is_open()) {
            for (const auto& line : lines) {
                file << line << "\n";
            }
            file.close();
        }
        else {
            cout << filename << "파일 열기 실패" << endl;
        }
    }

    // 명령어 처리 함수
    void handleCommand() {

        if (command == ":wq") {
            cout << "wq 일때 : " << endl;
            saveToFile("output");
            window.close();
        }
        else if (command.find(":w ") == 0) {          // :w 명령어가 0 위치인지 확인
            // 이름이 주어지지 않았을 경우 기본적으로 output.txt로 이름 주기
            string filename = command.length() > 2 ? trim(command.substr(2)) : "output";
            saveToFile(filename);
            window.close();
        }
        else if (command == ":q!") {            // 저장 안 하고 바로 종료
            window.close();
        }
        else {
            errorMode = true;
            cmdClock.restart();     // 시간 재설정
        }

        command.clear();
        cmdMode = false;
    }

    // 명령어 전처리 함수
    string trim(const string& str) {
        // size_t : 부호없는 정수 타입
        size_t start = str.find_first_not_of(" \t\n\r"); // 앞쪽 공백, 탭, 줄바꿈이 아닌 위치 return
        size_t end = str.find_last_not_of(" \t\n\r");   // 뒤쪽 공백

        if (start == std::string::npos) {
            return "";       // 문자열이 모두 공백일 경우 빈 문자열 return
        }

        return str.substr(start, end - start + 1);      // 앞뒤 공백 제외 문자열 return
    }
};

// 메인 함수
int main() {
    VimSmall editor;
    editor.run();
    return 0;
}
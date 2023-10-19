#pragma comment(lib, "ws2_32.lib")
#define _CRT_SECURE_NO_WARNINGS // 혹은 localtime_s를 사용

//#pragma warning(disable: 4996)
#include <limits>
#include <WinSock2.h> //Winsock 헤더파일 include. WSADATA 들어있음.
#include <WS2tcpip.h>
#include <string>
#include <sstream>
#include <iostream>
#include <thread>
#include <mysql/jdbc.h>
#include <ctime>
#include <windows.h>
#include <cstdio> 
#include <cerrno> 
#include <vector>


#define MAX_SIZE 1024
WSADATA wsa;
using std::cout;
using std::cin;
using std::endl;
using std::string;

std::thread th2;

sql::mysql::MySQL_Driver* driver;
sql::Connection* con;
sql::Statement* stmt;
sql::PreparedStatement* pstmt;
sql::ResultSet* result;

const string server = "tcp://127.0.0.1:3306"; // 데이터베이스 주소
const string username = "root"; // 데이터베이스 사용자
const string db_password = "Aa103201579!"; // 데이터베이스 접속 비밀번호

int color_flag = 0;
int delete_return = 0;
const int special_word = 3;
const string new_password;
string id, password;
SOCKET client_sock;
string real_nickname;
SOCKADDR_IN client_addr = {};
SOCKADDR_IN new_client = {};
std::vector <string> bad_word = { "바보", "멍청이" };
string user;

time_t timer;
struct tm* t;

bool password_check(string);
int chat_recv();
void modify_user_info(string*, string*);
void user_delete(string*, string*);
void chat_history();
void MenuList(SOCKADDR_IN&, string*, string*);
void first_screen();
void current_user(string);
bool duplicate_current_user(string);
void delete_current_user(string);
void re_chat();
void output_chat(char buf[MAX_SIZE]);

namespace my_to_str{
    template < typename T > std::string to_string(const T& n)
    {
        std::ostringstream stm;
        stm << n;
        return stm.str();
    }
}
enum ConsolColor {
    CC_BLACK,
    CC_DARKBLUE,
    CC_DARKGREEN,
    CC_DARKCYAN,
    CC_DARKRED,
    CC_DARKMAGENTA,
    CC_DARKYELLOW,
    CC_GRAY,
    CC_DARKGRAY,
    CC_BLUE,
    CC_GREEN,
    CC_CYAN,
    CC_RED,
    CC_MAGENTA,
    CC_YELLOW,
    CC_WHITE,
    CC_LIGHTGRAY = 7,
    CC_ORIGINAL = 7,
    CC_ORIGINALFONT = 7,
    CC_ORIGINALBG = 0
}; 

typedef struct _Color {
    int num = 1;
    int color[10] = {CC_BLACK};
    string user_nickname[10] = {"."};
}Color;

Color c;

enum Modify {
    MODIFY_ID,
    MODIFY_PASSWORD,
    MODIFY_NICKNAME,
    MODIFY_EMAIL,
    MODIFY_BACK,
};

enum Delete {
    _DELETE,
    YES_DELETE,
    NO_DELETE,
};

enum Menu {
    _MENU,
    MENU_MODIFY,
    MENU_CHAT,
    MENU_DELETE,
    MENU_MAIN,
};

enum Main {
    _MAIN,
    MAIN_LOGIN,
    MAIN_MEMBERSHIP,
};

void to_clean() {
    system("cls");
}

void setFontColor(int color) {
    CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (info.wAttributes & 0xf0) | (color & 0xf));
}

void setColor(int color, int bgcolor){
    color &= 0xf;
    bgcolor &= 0xf;
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
        (bgcolor << 4) | color);
}

bool email_check(string email) {
    if (email.find("@") == string::npos) {
        return false;
    }
    else if (email.find(".com") == string::npos) {
        return false;
    }
    else if (email.find(".com") < email.find("@")) {
        return false;
    }
    else if (email.find("@") == 0) {
        return false;
    }
    else
        return true;
}

string check_timestamp() {
    timer = time(NULL); // 1970년 1월 1일 0시 0분 0초부터 시작하여 현재까지의 초
    t = localtime(&timer); // 포맷팅을 위해 구조체에 넣기
    string to_timestamp;

    to_timestamp = my_to_str::to_string(t->tm_year + 1900) + "-" + my_to_str::to_string(t->tm_mon + 1) + "-"
        + my_to_str::to_string(t->tm_mday) + "-" + my_to_str::to_string(t->tm_hour)
        + "-" + my_to_str::to_string(t->tm_min) + "-" + my_to_str::to_string(t->tm_sec);

    return to_timestamp;
}

bool login_possible(string possible_id, string possible_password) {
    string check_id, check_pw;

    pstmt = con->prepareStatement("SELECT * FROM user where user_id=? and password=?;");
    pstmt->setString(1, possible_id);
    pstmt->setString(2, possible_password);
    pstmt->execute();
    result = pstmt->executeQuery(); //데이터베이스에서 id 와 password 를 가져온다.
    while (result->next()) {
        check_id = result->getString(1).c_str();
        check_pw = result->getString(2).c_str();
        real_nickname = result->getString(3).c_str();
    }
    if (check_id == possible_id && check_pw == possible_password) {
        if (duplicate_current_user(possible_id)) {
            cout << "로그인 되었습니다." << endl;
            //current_user(possible_id);
            to_clean();
            return true;
        }
    }
    else {
        cout << "로그인에 실패했습니다." << endl;
        to_clean();
        return false;
    }
}

void chat_history() { // 유저 입장 시 SERVER로 부터 1을 받으면 채팅 전문을 데이터베이스로부터 받아옴
    pstmt = con->prepareStatement("select * from chatting");
    pstmt->execute();
    result = pstmt->executeQuery();

    while (result->next()) {
        cout << result->getString(2) << " [" + result->getString(3) + "] :" << result->getString(1) << '\n';
    }
}
//
//SOCKADDR_IN createSocket() {
//    int code = WSAStartup(MAKEWORD(2, 2), &wsa);
//
//    new_client.sin_family = AF_INET;
//    new_client.sin_port = htons(7777);
//    InetPton(AF_INET, TEXT("127.0.0.1"), &new_client.sin_addr);
//
//    return;
//}
void re_chat() {
    closesocket(client_sock);
    WSACleanup();

    int code = WSAStartup(MAKEWORD(2, 2), &wsa);

    if (!code) {
        client_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
        client_addr.sin_family = AF_INET;
        client_addr.sin_port = htons(7777);
        InetPton(AF_INET, TEXT("127.0.0.1"), &client_addr.sin_addr);
    }
}

void go_chatting(SOCKADDR_IN& client_addr) {     //채팅방 입장 함수
    while (1) {
        if (!connect(client_sock, (SOCKADDR*)&client_addr, sizeof(client_addr))) {
            cout << "Server Connect" << endl;
            send(client_sock, real_nickname.c_str(), real_nickname.length(), 0);
            break;
        }
        //perror("connect : ");
        cout << "Connecting..." << endl;

    }
    std::thread th2(chat_recv);

    while (1) {
        string text;
        std::getline(cin, text);
        const char* buffer = text.c_str(); // string형을 char* 타입으로 변환
       
        if (text == "") { continue; } // enter -> db 저장 x
        if (text == "/back") {
            send(client_sock, buffer, strlen(buffer), 0);
            //client_addr = createSocket(); 
            re_chat();
            MenuList(client_addr, &id, &password);
            break;
        }
        else {
            pstmt = con->prepareStatement("insert into chatting(message_content, send_time, user_nickname) values(?, ?, ?)");
            pstmt->setString(1, text);
            pstmt->setString(2, check_timestamp());
            pstmt->setString(3, real_nickname);
            pstmt->execute();
        }

        for (int i = 0; i < 2; i++) {
            if (text.find(bad_word[i]) != string::npos) {
                send(client_sock, buffer, strlen(buffer), 0);
                //client_addr = createSocket(); 
                re_chat();
                MenuList(client_addr, &id, &password);
                break;
            }
        }
        send(client_sock, buffer, strlen(buffer), 0);
    }
    th2.join();
    closesocket(client_sock);
}

int chat_recv() {
    char buf[MAX_SIZE] = { };
    string msg;
    //memset(&c, 0, sizeof(struct _Color));
    while (1) {
        ZeroMemory(&buf, MAX_SIZE);
        if (recv(client_sock, buf, MAX_SIZE, 0) > 0) {
            msg = buf;
            //color_flag = 0;
            std::stringstream ss(msg);  // 문자열을 스트림화
            //string user;
            ss >> user;// 스트림을 통해, 문자열을 공백 분리해 변수에 할당. 보낸 사람의 이름만 user에 저장됨.
            if (msg == "1") { //server로 부터 1을 받으면 db를 불러오는 함수 실행
                chat_history(); continue;
            }
            if (msg.substr(0, 6) == "/color") {
                std::stringstream s(msg);
                string nick, color;
                while (s >> nick >> color) {
                    if (nick != "/color") {
                        break;
                    }
                }
                c.color[c.num] = stoi(color);
                c.user_nickname[c.num] = nick;
                c.num++;
            }
                //for (int i = 0; i < c.num; i++) {
                //    if (user == c.user_nickname[i]) {
                //        setFontColor(c.color[i]);
                //        cout << buf << endl;
                //        setColor(CC_LIGHTGRAY, CC_BLACK); //기존 색상.
                //        
                //    }
                //}
                //    if (user != real_nickname) {
                //        setColor(CC_LIGHTGRAY, CC_BLACK);
                //        cout << buf << endl;
                //        
                //    }// 내가보낸 게 아닐 경우에만 출력하도록.
            std::thread t(output_chat, buf);
                
            t.join();
        }
        else {
                cout << "Server Off" << endl;
                return -1;
        }
    }
}

void output_chat(char buf[MAX_SIZE]) {
    for (int i = 0; i < c.num; i++) {
        if (user == c.user_nickname[i]) {
            setFontColor(c.color[i]);
            cout << buf << endl;
            setColor(CC_LIGHTGRAY, CC_BLACK); //기존 색상.
            return;
        }
    }
    if (user != real_nickname) {
        setColor(CC_LIGHTGRAY, CC_BLACK);
        cout << buf << endl;
        return;
    }// 내가보낸 게 아닐 경우에만 출력하도록.
}

void MenuList(SOCKADDR_IN& client_addr, string* menu_id, string* menu_password) {
    int num = 0;
    to_clean();
    cout << "☆★☆★☆★☆★☆★☆★☆★☆★☆★☆★☆★☆★☆★☆\n";
    cout << "★                                                  ★\n";
    cout << "☆                                                  ☆\n";
    cout << "★                                                  ★\n";
    cout << "☆                                                  ☆\n";
    cout << "★                                                  ★\n";
    cout << "☆                  1. 회원정보수정                 ☆\n";
    cout << "★                                                  ★\n";
    cout << "☆                                                  ☆\n";
    cout << "☆                  2. 채팅방 입장                  ☆\n";
    cout << "★                                                  ★\n";
    cout << "☆                                                  ☆\n";
    cout << "★                  3. 회원탈퇴                     ★\n";
    cout << "☆                                                  ☆\n";
    cout << "★                                                  ★\n";
    cout << "☆                  4. 로그아웃                     ☆\n";
    cout << "★                                                  ★\n";
    cout << "☆                                                  ☆\n";
    cout << "★                                                  ★\n";
    cout << "☆                                                  ☆\n";
    cout << "★                                                  ★\n";
    cout << "☆★☆★☆★☆★☆★☆★☆★☆★☆★☆★☆★☆★☆★☆\n";

    cin >> num;

    to_clean();
    switch (num) {
    case MENU_MODIFY:
        modify_user_info(menu_id, menu_password);
        MenuList(client_addr, menu_id, menu_password); // 회원정보 수정이 완료된 후 다시 메뉴로 이동
        break;
    case MENU_CHAT:
        go_chatting(client_addr);
        break;
    case MENU_DELETE:
        user_delete(menu_id, menu_password);
        if (delete_return) {
            break;
        }
        else {
            MenuList(client_addr, menu_id, menu_password);
            break;
        }
    case MENU_MAIN:
        delete_current_user(*menu_id);
        first_screen();
        break;
    }
}

void modify_user_info(string* modify_id, string* password) {
    string check_id, check_password;
    string new_info;
    int what_modify = 0;

    pstmt = con->prepareStatement("SELECT * FROM user where user_id = ? and password = ?");
    pstmt->setString(1, *modify_id);
    pstmt->setString(2, *password);
    pstmt->execute();
    result = pstmt->executeQuery();

    while (result->next()) {
        check_id = result->getString(1).c_str();
        check_password = result->getString(2).c_str();
    }
    if (check_id == *modify_id && check_password == *password) {
        cout << "1.비밀번호  2.닉네임  3.이메일  4.뒤로가기" << endl;
        cout << "수정을 원하시는 항목을 선택해주세요. >> ";
        cin >> what_modify;

        switch (what_modify) {
        case MODIFY_PASSWORD:
            cout << "새로운 비밀번호를 입력해주세요. (!, ?, # 중 1개 이상 포함) >> ";
            while (1) {
                cin >> new_info;
                if (password_check(new_info)) {
                    pstmt = con->prepareStatement("UPDATE user set password = ? where user_id = ?");
                    pstmt->setString(1, new_info);
                    pstmt->setString(2, *modify_id);
                    pstmt->execute();
                    *password = new_info;
                    cout << "성공적으로 정보가 수정되었습니다." << endl;
                    break;
                }
                else {
                    cout << "비밀번호 형식에 맞춰서 입력해주세요. >> ";
                    continue;
                }
            }break;
        case MODIFY_NICKNAME:
            cout << "새로운 닉네임을 입력해주세요. >> ";
            cin >> new_info;
            pstmt = con->prepareStatement("UPDATE user set nickname = ? where user_id = ?");
            pstmt->setString(1, new_info);
            pstmt->setString(2, *modify_id);
            pstmt->execute();
            cout << "성공적으로 정보가 수정되었습니다." << endl;
            real_nickname = new_info;
            break;
        case MODIFY_EMAIL:
            cout << "새로운 이메일을 입력해주세요. >> ";
            while (1) {
                cin >> new_info;
                if (email_check(new_info)) {
                    pstmt = con->prepareStatement("UPDATE user set email = ? where user_id = ?");
                    pstmt->setString(1, new_info);
                    pstmt->setString(2, *modify_id);
                    pstmt->execute();
                    cout << "성공적으로 정보가 수정되었습니다." << endl;
                    break;
                }
                else {
                    cout << "이메일 형식에 맞춰서 입력해주세요. >> ";
                    continue;
                }
            }
            break;
        case MODIFY_BACK:
            break;
        }
    }
    else {
        cout << "아이디 또는 비밀번호가 올바르지 않습니다." << endl;
    }
}

void user_delete(string* id, string* password) {
    int yes_or_no = 0;
    string delete_password;

    cout << "회원탈퇴를 할 경우 모든 대화 내용이 사라집니다." << endl
        << "계속 진행하시겠습니까? (1. 예  2. 아니오) >> ";
    cin >> yes_or_no;

    switch (yes_or_no) {
    case YES_DELETE:
        cout << "안전한 회원탈퇴를 위해, 비밀번호를 입력해주세요. >> ";
        cin >> delete_password;
        if (delete_password == *password) {
            pstmt = con->prepareStatement("DELETE FROM user where password = ?");
            pstmt->setString(1, *password);
            pstmt->execute();
            cout << "성공적으로 회원탈퇴가 되었습니다." << endl;
            delete_return = 1;
            break;
        }
        else {
            cout << "올바르지 않은 비밀번호를 입력하셨습니다." << endl;
            delete_return = 0;
            break;
        }
    case NO_DELETE:
        delete_return = 0;
        break;
    }
}

void current_user(string current_id) {
    pstmt = con->prepareStatement("INSERT INTO connecting_user(user_id) values(?)");
    pstmt->setString(1, current_id);
    pstmt->execute();
}

void delete_current_user(string delete_id) {
    pstmt = con->prepareStatement("DELETE FROM connecting_user where user_id = ?");
    pstmt->setString(1, delete_id);
    pstmt->execute();
}

bool duplicate_current_user(string duplicate_current_id) {
    string current_id;

    pstmt = con->prepareStatement("SELECT * FROM connecting_user");
    pstmt->execute();
    result = pstmt->executeQuery();

    while (result->next()) {
        current_id = result->getString(1).c_str();
    } //delete pstmt;
    if (current_id == duplicate_current_id) {
        return false;
    }
    else return true;
}

bool password_check(string a) {
    char special_char[special_word] = { '!', '?', '#' }; // !, ?, # 중 하나를 반드시 포함하여 비밀번호 구성
    for (char c : special_char) {
        if (a.find(c) != string::npos) {
            return true;
        }
    }
    return false;
}

void join_membership() {
    string membership_id, membership_password, nickname, email;
    string correct_id, correct_password;

    while (1) {
        cout << "사용할 아이디를 입력해주세요. >> ";
        cin >> membership_id;

        pstmt = con->prepareStatement("SELECT * FROM user where user_id=? ;"); //유저 id의 중복 체크를 위한 select문
        pstmt->setString(1, membership_id);
        pstmt->execute();
        result = pstmt->executeQuery();

        while (result->next()) {
            correct_id = result->getString(1).c_str();
        }
        if (correct_id == membership_id) {
            cout << "이미 사용중인 아이디입니다." << endl;
        }
        else if (correct_id != membership_id) {
            id = membership_id;
            while (1) {
                cout << "사용할 비밀번호를 입력해주세요. (!, ?, # 중 1개 이상 포함) >> ";
                cin >> membership_password;
                if (password_check(membership_password)) {
                    password = membership_password;
                    break;
                }
                continue;
            }
            cout << "닉네임을 입력해주세요 >> ";
            cin >> nickname;
            real_nickname = nickname;
            cout << "이메일을 입력해주세요. >> ";
            while (1) {
                cin >> email;
                if (email_check(email)) {
                    break;
                }
                else {
                    cout << "이메일 형식에 맞춰서 입력해주세요. >> ";
                    continue;
                }
            }
            break;
        }
    }
    //아이디 중복 체크가 끝나면 비밀번호 이름을 user테이블에 저장한다.
    pstmt = con->prepareStatement("insert into user(user_id, password, nickname, email) values(?,?,?,?)");
    pstmt->setString(1, membership_id);
    pstmt->setString(2, membership_password);
    pstmt->setString(3, nickname);
    pstmt->setString(4, email);
    pstmt->execute();
    cout << "회원가입이 완료되었습니다." << endl;
    cout << "client_sock membership : " << client_sock << endl;
    to_clean();
}

void first_screen() {
    int client_choose = 0;
    while (1) {
        cout << "☆★☆★☆★☆★☆★☆★☆★☆★☆★☆★☆★☆★☆★☆\n";
        cout << "★                                                  ★\n";
        cout << "☆                                                  ☆\n";
        cout << "★                                                  ★\n";
        cout << "☆                                                  ☆\n";
        cout << "★                                                  ★\n";
        cout << "☆                                                  ☆\n";
        cout << "★                                                  ★\n";
        cout << "☆                                                  ☆\n";
        cout << "☆     ★★★★★      ★      ★       ★  ★      ☆\n";
        cout << "★         ★        ★ ★     ★       ★★        ★\n";
        cout << "☆         ★       ★★★★   ★       ★ ★       ☆\n";
        cout << "★         ★     ★       ★  ★★★★ ★   ★     ★\n";
        cout << "☆                                                  ☆\n";
        cout << "★                                                  ★\n";
        cout << "☆              1. 로그인   2. 회원가입             ☆\n";
        cout << "★                                                  ★\n";
        cout << "☆                                                  ☆\n";
        cout << "★                                                  ★\n";
        cout << "☆                                                  ☆\n";
        cout << "★                                                  ★\n";
        cout << "☆                                                  ☆\n";
        cout << "★                                                  ★\n";
        cout << "☆★☆★☆★☆★☆★☆★☆★☆★☆★☆★☆★☆★☆★☆\n";

        cin >> client_choose;

        to_clean();
        if (client_choose == MAIN_LOGIN) { // 로그인
            cout << "ID:";
            cin >> id;
            cout << "PW:";
            cin >> password;
            bool check = login_possible(id, password);
            if (!check) continue; //로그인 실패 시 다시 main으로
            MenuList(client_addr, &id, &password); //로그인 성공 시 메뉴로
            continue; // 회원탈퇴 후 main으로 오도록
        }
        if (client_choose == MAIN_MEMBERSHIP) { //회원가입
            join_membership();
            continue;
        }
    }
}

int main() {
    // int client_choose = 0;
    bool login = true;

    int code = WSAStartup(MAKEWORD(2, 2), &wsa);
    // Winsock를 초기화하는 함수. MAKEWORD(2, 2)는 Winsock의 2.2 버전을 사용하겠다는 의미.
    // 실행에 성공하면 0을, 실패하면 그 이외의 값을 반환.
    // 0을 반환했다는 것은 Winsock을 사용할 준비가 되었다는 의미.

    try {
        driver = sql::mysql::get_mysql_driver_instance();
        con = driver->connect(server, username, db_password);
    }
    catch (sql::SQLException& e) {
        cout << "Could not connect to server. Error message: " << e.what() << endl;
        exit(1);
    }

    con->setSchema("chat");
    stmt = con->createStatement();
    stmt->execute("set names euckr");//한국어를 받기위한 설정문

    if (stmt) { delete stmt; stmt = nullptr; }
    stmt = con->createStatement();
    delete stmt;

    if (!code) {
        client_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);  // 연결할 서버 정보 설정 부분

        client_addr.sin_family = AF_INET;
        client_addr.sin_port = htons(7777);
        InetPton(AF_INET, TEXT("127.0.0.1"), &client_addr.sin_addr);

        first_screen();
    }
    WSACleanup();
    return 0;
}
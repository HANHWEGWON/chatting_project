#pragma comment(lib, "ws2_32.lib")
#define _CRT_SECURE_NO_WARNINGS // 혹은 localtime_s를 사용

#include <WinSock2.h> //Winsock 헤더파일 include. WSADATA 들어있음.
#include <WS2tcpip.h>
#include <string>
#include <sstream>
#include <iostream>
#include <thread>
#include <mysql/jdbc.h>
#include <ctime>

#define MAX_SIZE 1024

using std::cout;
using std::cin;
using std::endl;
using std::string;

sql::mysql::MySQL_Driver* driver;
sql::Connection* con;
sql::Statement* stmt;
sql::PreparedStatement* pstmt;
sql::ResultSet* result;

const string server = "tcp://127.0.0.1:3306"; // 데이터베이스 주소
const string username = "root"; // 데이터베이스 사용자
const string db_password = "Aa103201579!"; // 데이터베이스 접속 비밀번호

int delete_return = 0;
const int special_word = 3;
const string new_password;

SOCKET client_sock;
string real_nickname;

time_t timer;
struct tm* t;

int chat_recv();
void modify_user_info(string*, string*);
void user_delete(string*, string*);

namespace my_to_str
{
    template < typename T > std::string to_string(const T& n)
    {
        std::ostringstream stm;
        stm << n;
        return stm.str();
    }
}
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


//void make_room() {
//    string title;
//    string date = check_date();
//    cout << "채팅방 제목을 설정해주세요. >> ";
//    getline(cin, title);
//
//    pstmt = con->prepareStatement("insert into chatting_room(room_id, room_title, date) values(?,?,?)");
//    pstmt->setString(1, title);
//   
//    pstmt->setString(2, date);
//    pstmt->execute();
//    cout << "채팅방이 개설되었습니다." << endl;
//
//}
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
    else
        return true;
}

string check_date() {
    timer = time(NULL); // 1970년 1월 1일 0시 0분 0초부터 시작하여 현재까지의 초
    t = localtime(&timer); // 포맷팅을 위해 구조체에 넣기
    string to_date;
   
    to_date = my_to_str::to_string(t->tm_year + 1900) + "-" + my_to_str::to_string(t->tm_mon + 1) + "-"
        + my_to_str::to_string(t->tm_mday);

    return to_date;
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
// 2023_10_11 업데이트 (로그인 가능여부)
bool login_possible(string id, string password) {

    string check_id, check_pw;

    pstmt = con->prepareStatement("SELECT * FROM user where user_id=? and password=?;");
    pstmt->setString(1, id);
    pstmt->setString(2, password);
    pstmt->execute();
    result = pstmt->executeQuery(); //데이터베이스에서 id 와 password 를 가져온다.
    while (result->next()) {
        check_id = result->getString(1).c_str();
        check_pw = result->getString(2).c_str();
        real_nickname = result->getString(3).c_str();
    }
    if (check_id == id && check_pw == password) {
        cout << "로그인 되었습니다." << endl;
        return true;
    }
    else {
        cout << "로그인에 실패했습니다." << endl;
        return false;
    }
}
// 2023_10_11 업데이트 (메뉴리스트 - 채팅방 입장 함수)
void go_chatting(SOCKADDR_IN& client_addr) {        //챗팅방 입장 함수
    //로그인후 서버와 커넥트 한다.
    while (1) {
        if (!connect(client_sock, (SOCKADDR*)&client_addr, sizeof(client_addr))) {
            cout << "Server Connect" << endl;
            send(client_sock, real_nickname.c_str(), real_nickname.length(), 0);
            break;
        }
        cout << "Connecting..." << endl;
    }

    std::thread th2(chat_recv);
    //message_store(check_id);
    while (1) {
        string text;
        std::getline(cin, text);
        const char* buffer = text.c_str(); // string형을 char* 타입으로 변환
        send(client_sock, buffer, strlen(buffer), 0);
    }
    th2.join();
    closesocket(client_sock);
}

// 2023_10_11 업데이트 (로그인 성공시 메뉴)
void MenuList(SOCKADDR_IN& client_addr, string* id, string* password) {
    int num;
    
    cout << "1. 회원정보수정 2. 채팅방 입장 3. 회원탈퇴\n";
    cin >> num;
    
        switch (num) {
        case 1:
            modify_user_info(id, password);
            MenuList(client_addr, id, password); // 회원정보수정이 완료된 후 뒤로가기 느낌??
            break;
        case 2:
            go_chatting(client_addr);
            break;
        case 3:
            user_delete(id, password);
            if (delete_return) {
               // cout << "check";
                break;
            }
            else {
                MenuList(client_addr, id, password);
                break;
            }
            
        }
   
}
void modify_user_info(string* id, string* password) {
    string check_id, check_password;
    string new_info;
    int what_modify = 0;

    pstmt = con->prepareStatement("SELECT * FROM user where user_id = ? and password = ?");
    pstmt->setString(1, *id);
    pstmt->setString(2, *password);
    pstmt->execute();
    result = pstmt->executeQuery();

    while (result->next()) {
        check_id = result->getString(1).c_str();
        check_password = result->getString(2).c_str();
    }//왜 id랑 password 값 비교하는거지? 문제 -> 여긴 넘어간다고 쳐도 
    //회원정보 비밀번호 수정하고 회원탈퇴하려면 call by value라서 오류.
    if (check_id == *id && check_password == *password) {
        cout << "1.비밀번호  2.닉네임  3.이메일" << endl;
        cout << "수정을 원하시는 항목을 선택해주세요. >> ";
        cin >> what_modify;

        switch (what_modify) {
        case MODIFY_PASSWORD:
            cout << "새로운 비밀번호를 입력해주세요. >> ";
            cin >> new_info;
            pstmt = con->prepareStatement("UPDATE user set password = ? where user_id = ?");
            pstmt->setString(1, new_info);
            pstmt->setString(2, *id);
            pstmt->execute();
            *password = new_info;
            //delete pstmt;
            //delete result;
            cout << "성공적으로 정보가 수정되었습니다." << endl;
            break;
        case MODIFY_NICKNAME:
            cout << "새로운 닉네임을 입력해주세요. >> ";
            cin >> new_info;
            pstmt = con->prepareStatement("UPDATE user set nickname = ? where user_id = ?");
            pstmt->setString(1, new_info);
            pstmt->setString(2, *id);
            pstmt->execute();
            cout << "성공적으로 정보가 수정되었습니다." << endl;
            break;
        case MODIFY_EMAIL:
            cout << "새로운 이메일을 입력해주세요. >> ";
            cin >> new_info;
                pstmt = con->prepareStatement("UPDATE user set email = ? where user_id = ?");
                pstmt->setString(1, new_info);
                pstmt->setString(2, *id);
                pstmt->execute();
                cout << "성공적으로 정보가 수정되었습니다." << endl;
            
            break;
        case MODIFY_BACK:
            break;
        }
    }
    else {
        cout << "아이디 또는 비밀번호가 올바르지 않습니다." << endl;
    }
}

void user_delete(string* id, string* password) {//bool말고 여기 지역변수 선언해서 현재 return ㄱ밧대체
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
            //return true;
            break;
        }
        else {
            cout << "올바르지 않은 비밀번호를 입력하셨습니다." << endl;
            delete_return = 0;
            //return false;
            break;
        }
        //break;
    case NO_DELETE:
        delete_return = 0;
        //return false;
        break;
    }
    //return delete_return;
}



int chat_recv() {
    char buf[MAX_SIZE] = { };
    string msg;

    while (1) {
        ZeroMemory(&buf, MAX_SIZE);
        if (recv(client_sock, buf, MAX_SIZE, 0) > 0) {
            msg = buf;
            std::stringstream ss(msg);  // 문자열을 스트림화
            string user;
            ss >> user; // 스트림을 통해, 문자열을 공백 분리해 변수에 할당. 보낸 사람의 이름만 user에 저장됨.
            if (user != real_nickname) cout << buf << endl; // 내가 보낸 게 아닐 경우에만 출력하도록.
        }
        else {
            cout << "Server Off" << endl;
            return -1;
        }
    }
}
bool password_check(string a) {
    char special_char[special_word] = { '!', '?', '#' };
    for (char c : special_char) {
        if (a.find(c) != string::npos) {
            return true;
        }
    }
    return false;
}
void join_membership() {
    string id, password, nickname, email;
    string correct_id, correct_password;

    while (1) {
        cout << "사용할 아이디를 입력해주세요. >> ";
        cin >> id;

        pstmt = con->prepareStatement("SELECT * FROM user where user_id=? ;"); //유저 id의 중복 체크를 위한 select문
        pstmt->setString(1, id);
        
        pstmt->execute();

        result = pstmt->executeQuery();
        while (result->next()) {
            correct_id = result->getString(1).c_str();
        }
        if (correct_id == id) {
            cout << "이미 사용중인 아이디입니다." << endl;
        }
        else if (correct_id != id) {
            while (1) {
                cout << "사용할 비밀번호를 입력해주세요. (!, ?, # 중 1개 이상 포함) >> ";
                cin >> password;
                if (password_check(password)) {
                    break;
                }
                continue;
            }
            cout << "닉네임을 입력해주세요 >> ";
            cin >> nickname;
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
    pstmt->setString(1, id);
    pstmt->setString(2, password);
    pstmt->setString(3, nickname);
    pstmt->setString(4, email);
    pstmt->execute();
    cout << "회원가입이 완료되었습니다." << endl;
        
    }
//void message_store(string check_id) {
//
//    string store_id, store_msg; //client테이블에서 id랑,chatting내용을 담을 변수 
//
//    bool id = true;
//    cout << "저장된 내용" << endl;
//    con->setSchema("login");
//    pstmt = con->prepareStatement("SELECT * FROM chatting_massenger;");
//    result = pstmt->executeQuery();
//    while (result->next()) {
//        store_id = result->getString("id").c_str();
//        store_msg = result->getString("text").c_str();
//        //처음 입장해서 채팅내용이 없으면 전 내용을 못보게 설정
//        if (store_id == check_id)
//        {
//            id = false;
//        }
//        if (id == false)
//        {
//            cout << store_id << " : " << store_msg << endl;
//        }
//    }
//}


int main() {
    WSADATA wsa;
    int client_choose = 0;
    bool login = true;
    string id, password;
    // Winsock를 초기화하는 함수. MAKEWORD(2, 2)는 Winsock의 2.2 버전을 사용하겠다는 의미.
     // 실행에 성공하면 0을, 실패하면 그 이외의 값을 반환.
     // 0을 반환했다는 것은 Winsock을 사용할 준비가 되었다는 의미.
    int code = WSAStartup(MAKEWORD(2, 2), &wsa);

    try {
        driver = sql::mysql::get_mysql_driver_instance();
        con = driver->connect(server, username, db_password);
    }
    catch (sql::SQLException& e) {
        cout << "Could not connect to server. Error message: " << e.what() << endl;
        exit(1);
    }
    con->setSchema("chat");
    //한국어를 받기위한 설정문
    stmt = con->createStatement();
    stmt->execute("set names euckr");
    if (stmt) { delete stmt; stmt = nullptr; }
    stmt = con->createStatement();
    delete stmt;

    if (!code) {

        client_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP); // 

        // 연결할 서버 정보 설정 부분
        SOCKADDR_IN client_addr = {};
        client_addr.sin_family = AF_INET;
        client_addr.sin_port = htons(7777);
        InetPton(AF_INET, TEXT("127.0.0.1"), &client_addr.sin_addr);

        while (1) {
            cout << "1: 로그인하기 2: 회원가입하기" << endl;
            cin >> client_choose;
            // 로그인
            if (client_choose == 1) {
                //string id, password;
                cout << "ID:";
                cin >> id;
                cout << "PW:";
                cin >> password;
                bool check = login_possible(id, password);
                if (!check) continue;
                MenuList(client_addr, &id, &password);
                continue; //10_12 회원탈퇴 후 main으로
            }
            if (client_choose == 2)
            {
                join_membership();
                continue;
            }
            
        }
    }
    WSACleanup();
    return 0;
}
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
const string password = "Aa103201579!"; // 데이터베이스 접속 비밀번호

SOCKET client_sock;
string real_nickname;

time_t timer;
struct tm* t;

namespace my_to_str
{
    template < typename T > std::string to_string(const T& n)
    {
        std::ostringstream stm;
        stm << n;
        return stm.str();
    }
}

void make_room() {

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
            cout << "사용할 비밀번호를 입력해주세요 >> ";
            cin >> password;
            cout << "닉네임을 입력해주세요 >> ";
            cin >> nickname;
            cout << "이메일을 입력해주세요 >> ";
            cin >> email;
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
    // Winsock를 초기화하는 함수. MAKEWORD(2, 2)는 Winsock의 2.2 버전을 사용하겠다는 의미.
     // 실행에 성공하면 0을, 실패하면 그 이외의 값을 반환.
     // 0을 반환했다는 것은 Winsock을 사용할 준비가 되었다는 의미.
    int code = WSAStartup(MAKEWORD(2, 2), &wsa);

    try {
        driver = sql::mysql::get_mysql_driver_instance();
        con = driver->connect(server, username, password);
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
            cout << "1: 로그인하기 2: 회원가입하기 3: 비밀번호 수정 4:회원탈퇴" << endl;
            cin >> client_choose;
            // 로그인
            if (client_choose == 1) {
                string id;
                string password;
                string check_id, check_pw;
                cout << "ID:";
                cin >> id;
                cout << "PW:";
                cin >> password;
                pstmt = con->prepareStatement("SELECT * FROM user where user_id=? and password=?;");
                pstmt->setString(1, id);
                pstmt->setString(2, password);
                pstmt->execute();
                result = pstmt->executeQuery();
                while (result->next()) {
                    check_id = result->getString(1).c_str();
                    check_pw = result->getString(2).c_str();
                    real_nickname = result->getString(3).c_str();
                }
                if (check_id == id && check_pw == password) {
                    cout << "로그인 되었습니다." << endl;
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
                else {
                    cout << "로그인에 실패했습니다." << endl;
                    continue;
                }
            }
            if (client_choose == 2)
            {
                join_membership();
                continue;
            }
            if (client_choose == 3)
            {

                continue;
            }
            if (client_choose == 4) {

                continue;
            }
        }
    }
    WSACleanup();
    return 0;
}
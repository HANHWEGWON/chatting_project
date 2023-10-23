#pragma comment(lib, "ws2_32.lib") //명시적인 라이브러리의 링크. 윈속 라이브러리 참조

#include <WinSock2.h>
#include <string>
#include <iostream>
#include <thread>
#include <vector>
#include <mysql/jdbc.h>
#include <sstream>
#include<cstdlib>
#include<ctime>
#include<algorithm>

#define MAX_SIZE 1024
#define MAX_CLIENT 1

using std::cout;
using std::cin;
using std::endl;
using std::string;

sql::mysql::MySQL_Driver* driver;
sql::Connection* con;
sql::Statement* stmt;
sql::PreparedStatement* pstmt;
sql::ResultSet* result;

const string server = "tcp://127.0.0.1:3306"; //데이터베이스 주소
const string username = "root"; //데이터베이스 사용자
const string db_password = "Aa103201579!"; //데이터베이스 접속 비밀번호


struct SOCKET_INFO { //연결된 소켓 정보에 대한 틀 생성
    SOCKET sck;
    string user;
};
std::vector<SOCKET_INFO> sck_list; //연결된 클라이언트 소켓들을 저장할 배열 선언.
SOCKET_INFO server_sock; //서버 소켓에 대한 정보를 저장할 변수 선언.
int client_count = 0; //현재 접속해 있는 클라이언트를 count 할 변수 선언.
std::vector<int> c; //랜덤숫자 저장 벡터
int memo[10];
std::vector<int> p; //플레이어가 언급한 숫자
int game_end = 0;

void server_init();
void add_client();
void del_client(int idx);
void send_msg(const char* msg); //send() 함수 실행됨. 자세한 내용은 함수 구현부에서 확인.
void recv_msg(int idx); //recv() 함수 실행됨. 자세한 내용은 함수 구현부에서 확인.
void game_manager(int turn);

namespace my_to_str
{
    template < typename T > std::string to_string(const T& n)
    {
        std::ostringstream stm;
        stm << n;
        return stm.str();
    }
}

void baseball_init() {
    srand(time(NULL));
    c.clear(), p.clear();

    for (;;) {
        int r = rand() % 9 + 1;
        c.push_back(r);
        sort(c.begin(), c.end());
        c.erase(unique(c.begin(), c.end()), c.end());
        if (c.size() == 3) break;
    }

    cout << c[0] << ' ' << c[1] << ' ' << c[2] << '\n';
}

void server_init() {
    server_sock.sck = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    //Internet의 Stream 방식으로 소켓 생성
    //SOCKET_INFO의 소켓 객체에 socket 함수 반환값(디스크립터 저장)
    //인터넷 주소체계, 연결지향, TCP 프로토콜 쓸 것. 

    SOCKADDR_IN server_addr = {}; //소켓 주소 설정 변수
    //인터넷 소켓 주소체계 server_addr

    server_addr.sin_family = AF_INET; //소켓은 Internet 타입 
    server_addr.sin_port = htons(6666); //서버 포트 설정
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);  //서버이기 때문에 local 설정한다. 
    //Any인 경우는 호스트를 127.0.0.1로 잡아도 되고 localhost로 잡아도 되고 양쪽 다 허용(INADDR_ANY)하게 할 수 있음
    //ip 주소를 저장할 땐 server_addr.sin_addr.s_addr

    bind(server_sock.sck, (sockaddr*)&server_addr, sizeof(server_addr)); // 설정된 소켓 정보를 소켓에 바인딩
    listen(server_sock.sck, SOMAXCONN); // 소켓을 대기 상태로 기다림
    server_sock.user = "server";

    cout << "game Server On" << endl;
}

void add_client() {
    while (true) {
        SOCKADDR_IN addr = {};
        int addrsize = sizeof(addr);
        char buf[MAX_SIZE] = { };

        ZeroMemory(&addr, addrsize); //addr의 메모리 영역을 0으로 초기화

        SOCKET_INFO new_client = {};

        new_client.sck = accept(server_sock.sck, (sockaddr*)&addr, &addrsize);
        if (new_client.sck == -1) continue;

        recv(new_client.sck, buf, MAX_SIZE, 0);
        //Winsock2의 recv 함수. client가 보낸 닉네임을 받음
        new_client.user = string(buf);
        cout << "소켓연결이 확인되고 닉네임을 받고 난후..." << endl;

        string msg = "[공지] " + new_client.user + " 님이 입장했습니다.";
        cout << msg << endl;
        sck_list.push_back(new_client); //client 정보를 답는 sck_list 배열에 새로운 client 추가

        client_count++; //client 수 증가.
        cout << "[공지] 현재 접속자 수 : " << client_count << "명" << endl;
        send_msg(msg.c_str()); //c_str : string 타입을 const chqr* 타입으로 바꿔줌.

        if (client_count == 2) {
            msg = "[공지] 야구게임을 시작하겠습니다!";
            cout << msg << endl;
            send_msg(msg.c_str());
            baseball_init();

            while (true) {
                msg = "상대턴입니다! 기다려주세요.";
                send(sck_list[1].sck, msg.c_str(), msg.size(), 0);
                Sleep(1000);
                game_manager(0);
                if (game_end == 1) {
                    msg = "******** 축하합니다 [" + sck_list[0].user + "] 님이 승리하셨습니다!";
                    send_msg(msg.c_str());
                    Sleep(1000);
                    ZeroMemory(buf, MAX_SIZE);
                    if (recv(new_client.sck, buf, MAX_SIZE, 0) > 0) {
                        system("cls");
                        del_client(1);
                        del_client(0);
                        break;
                    }

                }
                send(sck_list[0].sck, msg.c_str(), msg.size(), 0);
                Sleep(1000);
                game_manager(1);
                if (game_end == 1) {
                    msg = "******** 축하합니다 [" + sck_list[1].user + "] 님이 승리하셨습니다!";
                    send_msg(msg.c_str());
                    Sleep(1000);
                    ZeroMemory(buf, MAX_SIZE);
                    if (recv(new_client.sck, buf, MAX_SIZE, 0) > 0) {
                        system("cls");
                        del_client(1);
                        del_client(0);
                        break;
                    }
                }
            }
        }
    }
}

void send_msg(const char* msg) { //접속해 있는 모든 client에게 메시지 전송
    for (int i = 0; i < client_count; i++) {
        send(sck_list[i].sck, msg, MAX_SIZE, 0);
    }
}

bool is_digit(string str) {
    return atoi(str.c_str()) != 0 || str.compare("0") == 0;
}

bool checkThreeNumber(string& msg, int* memo) {

    std::stringstream stream(msg);
    string sub;

    while (stream >> sub) {

        int x = atoi(sub.c_str());

        if (!is_digit(sub)) {
            msg = "숫자가 아닌것이 들어와있습니다. 다시 입력해주세요.***";
            return false;
        }
        else if (x < 1 || x > 9) {
            msg = "범위를 넘어선 숫자가 있습니다. 다시 입력해주세요.***";
            return false;
        }
        else if (memo[x] == 1) {
            msg = "숫자가 중복되어있습니다. 다시 입력해주세요.***";
            return false;
        }

        p.push_back(x);
        memo[x] = 1;
    }
    return true;
}

void game_manager(int turn) { //thread로 클라이언트에서 오는 데이터 받아오는 함수
    char buf[MAX_SIZE] = { };
    string msg = "";

    while (1) {
        msg = "1~9 사이의 숫자 3개를 입력 하세요. \n<Number><Spacebar><Number><Spacebar><Number>형식으로 입력";
        send(sck_list[turn].sck, msg.c_str(), msg.size(), 0); //먼저 들어온 사람에게 보냄
        ZeroMemory(buf, MAX_SIZE);
        if (recv(sck_list[turn].sck, buf, MAX_SIZE, 0) > 0) {

            int memo[10] = { 0 }; //클라이언트가 고른 숫자 3개 메모

            msg = buf;

            if (!checkThreeNumber(msg, memo)) { //규칙에 맞게 작성됐는 지 확인
                send(sck_list[turn].sck, msg.c_str(), msg.size(), 0);
                memo[10] = { 0, }; p.clear();
                continue;
            }

            msg = sck_list[turn].user + " : ";
            for (int i : p) msg += std::to_string(i) + " ";
            send_msg(msg.c_str()); //클라이언트가 부른 숫자를 모두에게 보냄
            msg = "";

            int strike = 0, ball = 0; //스트라이크, 볼 개수 알고리즘.

            for (int i = 0; i < 3; i++) {
                if (memo[c[i]] == 1 && p[i] == c[i]) strike++; //p 에 c 정수가 있고, p 정수의 위치와 c 정수의 위치가 같으면 strike
                else if (memo[c[i]] == 1 && p[i] != c[i]) ball++; //p 에 
            }
            if (strike == 3) { // 승패가 가려지게 되는 코드
                game_end = 1;
                return;
            }
            msg = "!! [" + sck_list[turn].user + "] : Strike : " + std::to_string(strike) + "\tBall : " + std::to_string(ball);
            send_msg(msg.c_str());
            memo[10] = { 0, }; p.clear();
            return;
        }
    }
}

void del_client(int idx) {
    closesocket(sck_list[idx].sck);
    sck_list.erase(sck_list.begin() + idx);
    client_count--;
}

int main() {
    WSADATA wsa;

    // Winsock를 초기화하는 함수. MAKEWORD(2, 2)는 Winsock의 2.2 버전을 사용하겠다는 의미
    // 실행에 성공하면 0을, 실패하면 그 이외의 값을 반환
    // 0을 반환했다는 것은 Winsock을 사용할 준비가 되었다는 의미
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
    stmt = con->createStatement();
    stmt->execute("set names euckr");
    if (stmt) { delete stmt; stmt = nullptr; }

    if (!code) {
        server_init();

        std::thread th1[MAX_CLIENT];
        for (int i = 0; i < MAX_CLIENT; i++) {
            // 인원 수 만큼 thread 생성해서 각각의 클라이언트가 동시에 소통할 수 있도록 함
            th1[i] = std::thread(add_client);
        }

        while (1) { // 무한 반복문을 사용하여 서버가 계속해서 채팅 보낼 수 있는 상태를 만들어 줌. 반복문을 사용하지 않으면 한 번만 보낼 수 있음
            string text, msg = "";

            std::getline(cin, text);

            const char* buf = text.c_str();
            msg = server_sock.user + " : " + buf;
            send_msg(msg.c_str());
        }
        for (int i = 0; i < MAX_CLIENT; i++) {
            th1[i].join();
            //join : 해당하는 thread 들이 실행을 종료하면 리턴하는 함수
            //join 함수가 없으면 main 함수가 먼저 종료되어서 thread가 소멸하게 됨
            //thread 작업이 끝날 때까지 main 함수가 끝나지 않도록 해줌
        }
        closesocket(server_sock.sck);
    }
    else {
        cout << "프로그램 종료. (Error code : " << code << ")";
    }
    WSACleanup();

    return 0;
}
#pragma comment(lib, "ws2_32.lib") //명시적인 라이브러리의 링크. 윈속 라이브러리 참조
//링크하는거래 winsocket은 이미 깔려있음.

#include <WinSock2.h>
#include <string>
#include <sstream>
#include <iostream>
#include <thread>
#include <vector>
#include <mysql/jdbc.h>
#include <algorithm>

#define MAX_SIZE 1024 //데이터 전송할 때 임시로 저장하는 곳
#define MAX_CLIENT 3 //client 명수 제한

using std::cout;
using std::cin;
using std::endl;
using std::string;

sql::mysql::MySQL_Driver* driver;
sql::Connection* con;
sql::Statement* stmt;
sql::PreparedStatement* pstmt;
sql::ResultSet* result;

const string db_server = "tcp://127.0.0.1:3306"; // 데이터베이스 주소
const string db_username = "root"; // 데이터베이스 사용자
const string db_password = "Aa103201579!"; // 데이터베이스 접속 비밀번호

struct SOCKET_INFO { // 연결된 소켓 정보에 대한 틀 생성
    SOCKET sck; //클라이언트 각각의 소켓 저장 위함.
    string user;
};

//std::vector<SOCKET_INFO> sck_list; // 연결된 클라이언트 소켓들을 저장할 배열 선언.
std::list<SOCKET_INFO> sck_list;
SOCKET_INFO server_sock; // 서버 소켓에 대한 정보를 저장할 변수 선언.


int client_count = 0; // 현재 접속해 있는 클라이언트를 count 할 변수 선언.

void server_init(); // socket 초기화 함수. socket(), bind(), listen() 함수 실행됨. 자세한 내용은 함수 구현부에서 확인.
void add_client(); // 소켓에 연결을 시도하는 client를 추가(accept)하는 함수. client accept() 함수 실행됨. 자세한 내용은 함수 구현부에서 확인.
void send_msg(const char* msg); // send() 함수 실행됨. 자세한 내용은 함수 구현부에서 확인.
void recv_msg(int idx); // recv() 함수 실행됨. 자세한 내용은 함수 구현부에서 확인.
void del_client(int idx); // 소켓에 연결되어 있는 client를 제거하는 함수. closesocket() 실행됨. 자세한 내용은 함수 구현부에서 확인.
void direct_msg(string nickname, string msg);

// i = 0;

int main() {
    WSADATA wsa;

    // Winsock를 초기화하는 함수. MAKEWORD(2, 2)는 Winsock의 2.2 버전을 사용하겠다는 의미.
    // 실행에 성공하면 0을, 실패하면 그 이외의 값을 반환.
    // 0을 반환했다는 것은 Winsock을 사용할 준비가 되었다는 의미.
    int code = WSAStartup(MAKEWORD(2, 2), &wsa);

    try {
        driver = sql::mysql::get_mysql_driver_instance();
        con = driver->connect(db_server, db_username, db_password);
    }
    catch (sql::SQLException& e) {
        std::cout << "Could not connect to server. Error message: " << e.what() << endl;
        exit(1);
    }

    // 데이터베이스 선택
    con->setSchema("chat");

    // db 한글 저장을 위한 셋팅 
    stmt = con->createStatement();
    stmt->execute("set names euckr");
    if (stmt) { delete stmt; stmt = nullptr; }

    if (!code) {
        server_init();

        std::thread th1[MAX_CLIENT];
        for (int i = 0; i < MAX_CLIENT; i++) {
            // 인원 수 만큼 thread 생성해서 각각의 클라이언트가 동시에 소통할 수 있도록 함.
            th1[i] = std::thread(add_client); //new 안쓰고 overload 객체 생성?
        }
        
        //std::thread th1(add_client); // 이렇게 하면 하나의 client만 받아짐...

        while (1) { // 무한 반복문을 사용하여 서버가 계속해서 채팅 보낼 수 있는 상태를 만들어 줌. 반복문을 사용하지 않으면 한 번만 보낼 수 있음.
            string text, msg = "";

            std::getline(cin, text);
            const char* buf = text.c_str();
            msg = server_sock.user + " : " + buf;
            send_msg(msg.c_str()); //다른 client에게 메세지 전송
        }

        for (int i = 0; i < MAX_CLIENT; i++) {
            th1[i].join();
            //join : 해당하는 thread 들이 실행을 종료하면 리턴하는 함수.
            //join 함수가 없으면 main 함수가 먼저 종료되어서 thread가 소멸하게 됨.
            //thread 작업이 끝날 때까지 main 함수가 끝나지 않도록 해줌.
        }
        //th1.join();

        closesocket(server_sock.sck);
    }
    else {
        cout << "프로그램 종료. (Error code : " << code << ")";
    }

    WSACleanup();

    return 0;
}

void server_init() { //서버 초기화
    server_sock.sck = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP); //tcp 사용 시 sock_stream사용?
    // Internet의 Stream 방식으로 소켓 생성
   // SOCKET_INFO의 소켓 객체에 socket 함수 반환값(디스크립터 저장)
   // 인터넷 주소체계, 연결지향, TCP 프로토콜 쓸 것. 

    SOCKADDR_IN server_addr = {}; // 소켓 주소 설정 변수
    // 인터넷 소켓 주소체계 server_addr

    server_addr.sin_family = AF_INET; // 소켓은 Internet 타입 
    server_addr.sin_port = htons(7777); // 서버 포트 설정 -> 포트는 내가 하기 나름. 바꿔도 됨
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);  // 서버이기 때문에 local 설정한다. 
    //Any인 경우는 호스트를 127.0.0.1로 잡아도 되고 localhost로 잡아도 되고 양쪽 다 허용하게 할 수 있다. 그것이 INADDR_ANY이다.
    //ip 주소를 저장할 땐 server_addr.sin_addr.s_addr -- 정해진 모양?

    //소켓의 흐름. socket -> bind

    bind(server_sock.sck, (sockaddr*)&server_addr, sizeof(server_addr)); // 설정된 소켓 정보를 소켓에 바인딩한다.
    listen(server_sock.sck, SOMAXCONN); // 소켓을 대기 상태로 기다린다.*
    server_sock.user = "server";

    cout << "Server On" << endl;
}//여기까지 서버 초기화

void add_client() {
    SOCKADDR_IN addr = {};
    int addrsize = sizeof(addr);
    char buf[MAX_SIZE] = { };

    ZeroMemory(&addr, addrsize); // addr의 메모리 영역을 0으로 초기화

    SOCKET_INFO new_client = {};

    new_client.sck = accept(server_sock.sck, (sockaddr*)&addr, &addrsize);
    recv(new_client.sck, buf, MAX_SIZE, 0); //client에 요기1로 표시함. 연결되면 임의로 바로 닉네임을 서버에 전송.
    // Winsock2의 recv 함수. client가 보낸 닉네임을 받음.
    new_client.user = string(buf);
    
    cout << new_client.sck << endl;
    string msg = "[공지] " + new_client.user + " 님이 입장했습니다.";
    cout << msg << endl;
    sck_list.push_back(new_client); // client 정보를 답는 sck_list 배열에 새로운 client 추가
 /*   cout << sck_list[0].sck << endl;
    cout << sck_list[1].sck << endl;
    cout << client_count << endl;
    */

    std::thread th(recv_msg, client_count); //스레드 내에 또 다른 스레드 생성.

    // 다른 사람들로부터 오는 메시지를 계속해서 받을 수 있는 상태로 만들어 두기.
    //client가 2명이면 서버thread가 4개가 돈다? client에선 각각 1개씩? 

    //std::thread tth(dm_recv_msg, client_count); dm 보낼 때 thread.

    client_count++; // client 수 증가.원래자리
    cout << "[공지] 현재 접속자 수 : " << client_count << "명" << endl;
    send_msg(msg.c_str()); // c_str : string 타입을 const chqr* 타입으로 바꿔줌.
    
    string first = "1";
    send(new_client.sck, first.c_str(), first.size(), 0);
    
    th.join();
    //thh.join();//
}

void send_msg(const char* msg) {
    for (std::list<SOCKET_INFO>::iterator it = sck_list.begin(); it!=sck_list.end(); it++) { // 접속해 있는 모든 client에게 메시지 전송
        send(it->sck, msg, MAX_SIZE, 0);
    }
}

void direct_msg(string nickname, string msg) { //다이렉트 메세지
    std::stringstream stream;
    stream.str(msg);
    string sub, send_msg;
    while (stream >> sub) {
        if (sub != "/dm") {
            break;
        }
    }
    pstmt = con->prepareStatement("select * from user");
    pstmt->execute();
    result = pstmt->executeQuery();

    int loc = msg.find(sub) + sub.length();
    int db_flag = 0, cur_flag = 0;
  
    while (result->next()) {
        if (result->getString(3).compare(sub) == 0) { //만약 dm 보낼 사람의 아이디가 DB에 저장된것과 같다면
            db_flag = 1;
            send_msg = "귓속말 [" + nickname + "] " + ":" + msg.substr(loc);
            for (std::list<SOCKET_INFO>::iterator it = sck_list.begin(); it != sck_list.end(); it++) {    //소켓 리스트에서 찾는다. 현재 들어와 있는지.
                if (it->user == sub) {
                    send(it->sck, send_msg.c_str(), MAX_SIZE, 0);
                    cur_flag = 1; break;
                }
            }


            if (cur_flag == 0) {
                send_msg = "현재 " + sub + "는 들어와있지 않습니다. \n";
                for (std::list<SOCKET_INFO>::iterator it = sck_list.begin(); it != sck_list.end(); it++) {    //소켓 리스트에서 찾는다.
                    if (it->user == nickname) {
                        send(it->sck, send_msg.c_str(), MAX_SIZE, 0);
                        break;
                    }
                }
            }
            break;
        }
    }

    if (db_flag == 0) {
        send_msg = "채팅 가입자가 아닙니다.\n";
        for (std::list<SOCKET_INFO>::iterator it = sck_list.begin(); it != sck_list.end(); it++) {    //소켓 리스트에서 찾는다.
            if (it->user == nickname) {
                send(it->sck, send_msg.c_str(), MAX_SIZE, 0);
                break;
            }
        }
    }

}

//
//void all_chat() {
//    string all_msg = "";
//    pstmt = con->prepareStatement("select * from chatting");
//    pstmt->execute();
//    result = pstmt->executeQuery();
//
//    /*result->next();
//    all_msg = result->getString(2) + " " + result->getString(3) + "\n";
//    send(sck_list[client_count - 1].sck, all_msg.c_str(), MAX_SIZE, 0);*/
//
//    while (result->next()) {
//        all_msg = result->getString(2) + " " + result->getString(3) + "\n";
//        send(sck_list[client_count - 1].sck, all_msg.c_str(), MAX_SIZE, 0);//새로 입장하는 client에게만 이전 채팅내용.
//    }
//}


void recv_msg(int idx) {
    std::list<SOCKET_INFO>::iterator it=sck_list.begin();
    int cnt = 0;
    for (; it != sck_list.end(); ++it) {
        if (cnt == idx) break;
        cnt++;
    }
    char buf[MAX_SIZE] = { };
    string msg = "";
   // cout << sck_list[idx].sck << endl;

    while (1) {
        ZeroMemory(&buf, MAX_SIZE);
        if (recv(it->sck, buf, MAX_SIZE, 0) > 0) { // 오류가 발생하지 않으면 recv는 수신된 바이트 수를 반환. 0보다 크다는 것은 메시지가 왔다는 것.
            if (string(buf).substr(0, 4) == "/dm ") {
                direct_msg(it->user, buf);
                continue;
            }
            if (string(buf).substr(0, 5) == "/back") {
                del_client(idx);
                return;
            }
            msg = it->user + " : " + buf;
            cout << msg << endl;
            send_msg(msg.c_str());
           // store_chat(msg, check_timestamp(), sck_list[idx].user); //10_11 
        }
        else { //그렇지 않을 경우 퇴장에 대한 신호로 생각하여 퇴장 메시지 전송
            msg = "[공지] " + it->user + " 님이 퇴장했습니다.";
            cout << msg << endl;
            send_msg(msg.c_str());
            del_client(idx); // 클라이언트 삭제
            return;
        }
    }
}


void del_client(int idx) {
    std::list<SOCKET_INFO>::iterator it = sck_list.begin();
    int cnt = 0;
    for (; it != sck_list.end(); ++it) {
        if (cnt == idx) break;
        cnt++;
    }
    closesocket(it->sck);
    sck_list.erase(it); // 배열에서 클라이언트를 삭제하게 될 경우 index가 달라지면서 런타임 오류 발생....ㅎ
    client_count--;
   /* pstmt = con->prepareStatement("DELETE from dm where client_sock = ?");
    pstmt->setInt(1, sck_list[idx].sck);
    pstmt->execute();
    delete pstmt;*/
}
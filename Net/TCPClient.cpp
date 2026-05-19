#include"TCPClient.h"
#include"../Mediator/TCPClientMediator.h"
#include"def.h"

bool TCPClient::initNet() {
	//加载库
    WORD version = MAKEWORD(_DEF_VERSION_LOW, _DEF_VERSION_HIGH);
	WSADATA data = {};
	int err = WSAStartup(version, &data);
	if (0 != err) {
		cout << "WSAStartup fail" << endl;
		return false;
	}
    if (LOBYTE(data.wVersion) == _DEF_VERSION_LOW && HIBYTE(data.wVersion) == _DEF_VERSION_HIGH) {
		cout << "WSAStartup success" << endl;
	}
	else {
		cout << "WSAStartup version error" << endl;
		return false;
	}
	//创建套接字
	m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == m_sock) {
		cout << "socket error:" << WSAGetLastError() << endl;
		return false;
	}
	else {
		cout << "socket success" << endl;
	}
	//请求建立连接
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(_DEF_SERVER_PORT);
    serverAddr.sin_addr.S_un.S_addr = inet_addr(_DEF_SERVER_IP);
    err = connect(m_sock, (sockaddr*)&serverAddr, sizeof(serverAddr));
	if (SOCKET_ERROR == err) {
		cout << "connect error:" << WSAGetLastError() << endl;
		return false;
	}
	else {
		cout << "connect success" << endl;
	}
	//此时已经可以开始接受数据了
	//创建一个新的线程, 用来接收数据
	m_hand = (HANDLE)_beginthreadex(nullptr, 0, &RecvThread, this, 0, nullptr);
	return true;
}
bool TCPClient::sendData(char* data, int len, long to) {
	if (!data || len < 0) {
		return false;
	}
    int nSendNum = send(m_sock, (const char*)&len, sizeof(int), 0);
    if (SOCKET_ERROR == nSendNum) {
		cout << "send packLen error:" << WSAGetLastError() << endl;
		return false;
	}
	nSendNum = send(m_sock, data, len, 0);
	if (SOCKET_ERROR == nSendNum) {
		cout << "send package error:" << WSAGetLastError() << endl;
		return false;
	}
	return true;
}
void TCPClient::recvData() {
    cout << __func__ << endl;
	while (m_isRunning) {
		//先接包大小
        int packLen = 0;
        int nRecvNum = recv(m_sock, (char*)&packLen, sizeof(int), 0);
        if (SOCKET_ERROR == nRecvNum) {
			cout << "recv packLen error:" << WSAGetLastError() << endl;
			break;
		}
        char* recvBuf = new char[packLen];
		//再接包内容
		int offset = 0;
        while (packLen > 0) {
            nRecvNum = recv(m_sock, recvBuf + offset, packLen, 0);
			if (nRecvNum > 0) {
				offset += nRecvNum;
                packLen -= nRecvNum;
			}
			else {
				cout << "recv error:" << WSAGetLastError() << endl;
				return;
			}
		}
		//成功接收, 把数据转发到中介者类
        m_INetMedia->transmitData(recvBuf, offset, (long)m_sock);
        delete[] recvBuf; //立即释放
	}
}
void TCPClient::uninitNet() {
	m_isRunning = 0;
	if (m_hand) {
		if (WAIT_TIMEOUT == WaitForSingleObject(m_hand, 500)) {
			TerminateThread(m_hand, -1);
		}
		CloseHandle(m_hand);
	}
    if (m_sock && m_sock != INVALID_SOCKET) {
		closesocket(m_sock);
	}
	WSACleanup();
}

unsigned __stdcall TCPClient::RecvThread(void* arg) {
    cout << __func__ << endl;
	((TCPClient*)arg)->recvData();
	return -1;
}

TCPClient::TCPClient(INetMediator* pThis) {
	m_INetMedia = pThis;
}
TCPClient::~TCPClient() {
	uninitNet();
}

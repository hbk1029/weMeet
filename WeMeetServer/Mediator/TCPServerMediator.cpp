#include"TCPServerMediator.h"
#include"../Net/TCPServer.h"
#include "../Kernel/Kernel.h"

TCPServerMediator::TCPServerMediator() {
    m_INet = new TCPServer(this);
}
TCPServerMediator::~TCPServerMediator() {

}
bool TCPServerMediator::openNet() {
    return m_INet->initNet();
}
void TCPServerMediator::run()
{
    m_INet->run();
}
void TCPServerMediator::closeNet() {
    m_INet->uninitNet();
}
bool TCPServerMediator::sendData(char* data, int len, int sockfd) {
    return m_INet->sendData(data, len, sockfd);
}
void TCPServerMediator::transmitData(char* data, int len, int sockfd) {
    Kernel::m_pKernel->dealData(data, len, sockfd);
}
void TCPServerMediator::onClientDisconnect(int sockfd) {
    Kernel::m_pKernel->handleDisconnect(sockfd);
}

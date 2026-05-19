#include"common.h"
#include"./Kernel/Kernel.h"

using namespace std;

int main()
{
    setvbuf(stdout, NULL, _IONBF, 0);
    srand(time(NULL));
    Kernel* m_pKernel = Kernel::getInstance();
    if (!m_pKernel->startServer()) {
        cout << "打开服务器失败" << endl;
        return -1;
    }
    m_pKernel->runServer();
    return 0;
}

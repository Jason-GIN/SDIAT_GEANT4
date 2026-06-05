#ifndef MYSESSION_HH
#define MYSESSION_HH

#include <fstream>
#include "G4UIsession.hh"
#include "G4UImanager.hh"

namespace Test02
{
// 1. 自定义会话类：将输出写入日志文件
class MySession : public G4UIsession {
public:
 MySession() {}
    virtual ~MySession() {}

    // 重载G4cout输出
    virtual G4UIsession* SessionStart() { return this; }
    virtual G4int ReceiveG4cout(const G4String& coutString) {
        std::cerr << coutString << std::flush;
        return 0;
    }
    virtual G4int ReceiveG4cerr(const G4String& cerrString) {
        std::cerr << cerrString << std::flush;
        return 0;
    }
};

}

#endif